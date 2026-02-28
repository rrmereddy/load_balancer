/**
 * @file LoadBalancer.cpp
 * @brief Implements the LoadBalancer class.
 */

#include "LoadBalancer.h"
#include "Logger.h"
#include <iterator>
#include <sstream>

LoadBalancer::LoadBalancer(int initialServers, const std::string& balancerLabel)
: requestQueue_(),
  servers_(),
  clock_(0),
  nextServerId_(1),
  totalRequests_(0),
  totalRequestsHandled_(0),
  pendingScaleDown_(0),
  totalServersAdded_(0),
  totalServersRemoved_(0),
  balancerLabel_(balancerLabel.empty() ? "LoadBalancer" : balancerLabel) {
    if (initialServers < 0) {
        initialServers = 0;
    }
    for (int i = 0; i < initialServers; ++i) {
        servers_.push_back(WebServer(nextServerId_++));
    }
}

int LoadBalancer::getServerCount() const {
    return static_cast<int>(servers_.size());
}

std::size_t LoadBalancer::getQueueSize() const {
    return requestQueue_.size();
}

std::vector<Request> LoadBalancer::getQueueSnapshot(std::size_t maxItems) const {
    std::vector<Request> snapshot;
    if (maxItems == 0 || requestQueue_.empty()) {
        return snapshot;
    }

    std::queue<Request> queueCopy = requestQueue_;
    while (!queueCopy.empty() && snapshot.size() < maxItems) {
        snapshot.push_back(queueCopy.front());
        queueCopy.pop();
    }
    return snapshot;
}

std::vector<ServerSnapshot> LoadBalancer::getServerSnapshots() const {
    std::vector<ServerSnapshot> snapshot;
    snapshot.reserve(servers_.size());
    for (const auto& server : servers_) {
        ServerSnapshot item;
        item.id = server.getId();
        item.busy = server.isBusy();
        item.request = item.busy ? server.getCurrentRequest() : Request();
        snapshot.push_back(item);
    }
    return snapshot;
}

int LoadBalancer::getActiveServerCount() const {
    int active = 0;
    for (const auto& server : servers_) {
        if (server.isBusy()) {
            ++active;
        }
    }
    return active;
}

int LoadBalancer::getIdleServerCount() const {
    return static_cast<int>(servers_.size()) - getActiveServerCount();
}

void LoadBalancer::enqueueRequest(const Request& request) {
    requestQueue_.push(request);
    ++totalRequests_;
}

bool LoadBalancer::hasPendingRequests() const {
    // if the request queue is not empty, return true
    if (!requestQueue_.empty()) {
        return true;
    }
    // if any server is busy, return true
    for (const auto& server : servers_) {
        if (server.isBusy()) {
            return true;
        }
    }

    return false;
}

bool LoadBalancer::dispatchToServers(Logger& logger) {
    bool dispatched = false;

    for (auto& server : servers_) {
        //if the server is busy, handle the request
        if (server.isBusy()) {
            // tick the server, this will decrement the time cycles of the request if it is busy and clear the request if it is done
            if (server.handleRequest()) {
                // log the request
                std::ostringstream line;
                line << "Clock " << clock_ << ": " << formatServerTag(server)
                     << " completed request from " << server.getCurrentRequest().ipIn << " to " << server.getCurrentRequest().ipOut;
                logger.log(line.str());
                // clear the request
                server.clearRequest();
                ++totalRequestsHandled_;
                continue;
            }
            // log the request
            std::ostringstream line;
            line << "Clock " << clock_ << ": " << formatServerTag(server)
                 << " working on request from " << server.getCurrentRequest().ipIn << " to " << server.getCurrentRequest().ipOut
                 << " finishing in " << server.getCurrentRequest().timeCycles << " cycles";
            logger.log(line.str());
            continue;
        }

        if (requestQueue_.empty()) {
            continue;
        }

        Request request = requestQueue_.front();
        requestQueue_.pop();
        server.assignRequest(request);

        // log the request
        std::ostringstream line;
        line << "Clock " << clock_ << ": " << formatServerTag(server)
             << " assigned request from " << request.ipIn << " to " << request.ipOut;
        logger.log(line.str());

        dispatched = true;
    }

    // process the deferred scale down
    processDeferredScaleDown(logger);

    return dispatched;
}

void LoadBalancer::tick() {
    ++clock_;
}

void LoadBalancer::addServer() {
    servers_.push_back(WebServer(nextServerId_++));
}

void LoadBalancer::removeServer() {
    for (auto it = servers_.rbegin(); it != servers_.rend(); ++it) {
        if (!it->isBusy()) {
            servers_.erase(std::next(it).base());
            return;
        }
    }
}

int LoadBalancer::getClock() const {
    return clock_;
}

void LoadBalancer::evaluateScaling(int minQueuePerServer, int maxQueuePerServer, Logger& logger) {
    const std::size_t queueSize = requestQueue_.size();
    const int serverCount = static_cast<int>(servers_.size());

    // if there are no servers, add one
    // Want to make sure we have at least one server to handle the requests
    if (serverCount == 0) {
        if (queueSize > 0) {
            addServer();
            ++totalServersAdded_;
            std::ostringstream line;
            line << "Clock " << clock_ << ": [" << balancerLabel_
                 << "] Scale up -> added server, total servers: " << servers_.size();
            logger.log(line.str());
        }
        return;
    }

    // if the queue size is greater than the max queue per server, add servers until threshold is met
    double queuePerServer = static_cast<double>(queueSize) / static_cast<double>(serverCount);
    if (queuePerServer > static_cast<double>(maxQueuePerServer)) {
        int addedServers = 0;

        // safeguard against invalid configuration that could loop forever
        if (maxQueuePerServer <= 0) {
            addServer();
            ++addedServers;
        } else {
            // add servers until the threshold is met
            while (!servers_.empty() && queuePerServer > static_cast<double>(maxQueuePerServer)) {
                addServer();
                ++addedServers;
                // recalculate the queue per server
                queuePerServer =static_cast<double>(queueSize) / static_cast<double>(servers_.size());
            }
        }

        // log the scale up
        std::ostringstream line;
        line << "Clock " << clock_ << ": [" << balancerLabel_ << "] Scale up -> added " << addedServers
             << " server(s), average queue per server is " << queuePerServer
             << ", total servers: " << servers_.size();
        logger.log(line.str());
        if (addedServers > 0) {
            totalServersAdded_ += addedServers;
        }
        return;
    }
    // if the queue size is less than the min queue per server, remove a server if possible
    if (queuePerServer < static_cast<double>(minQueuePerServer)) {
        // here we check whther the server can immediately be removed, else we defer the scale down
        if (!removeOneIdleServer(logger)) {
            ++pendingScaleDown_; // defer the scale down

            // log the deferred scale down
            std::ostringstream line;
            line << "Clock " << clock_ << ": [" << balancerLabel_
                 << "] Scale down deferred -> no idle server available, pending removals: "
                 << pendingScaleDown_;
            logger.log(line.str());
        }
    }
}

/**
 * @details Remove one idle server if any and if requested. Finished by AI
 * @param logger Logger for removal events.
 * @return True if a server was removed, false otherwise.
 */
bool LoadBalancer::removeOneIdleServer(Logger& logger) {
    for (auto it = servers_.rbegin(); it != servers_.rend(); ++it) {
        if (!it->isBusy()) {
            const int removedServerId = it->getId();
            servers_.erase(std::next(it).base());

            std::ostringstream line;
            line << "Clock " << clock_ << ": [" << balancerLabel_ << "] Scale down -> removed idle server "
                 << removedServerId
                 << ", total servers: " << servers_.size();
            logger.log(line.str());
            ++totalServersRemoved_;
            return true;
        }
    }
    return false;
}

void LoadBalancer::processDeferredScaleDown(Logger& logger) {
    while (pendingScaleDown_ > 0) {
        if (!removeOneIdleServer(logger)) {
            return;
        }
        --pendingScaleDown_;
    }
}

int LoadBalancer::getTotalRequestsCount() const {
    return totalRequests_;
}

int LoadBalancer::getTotalRequestsHandledCount() const {
    return totalRequestsHandled_;
}

/**
 * @details Get the total number of requests remaining.
 * @return Total requests remaining.
 */
int LoadBalancer::getTotalRequestsRemainingCount() const {
    return totalRequests_ - totalRequestsHandled_;
}

int LoadBalancer::getTotalServersAddedCount() const {
    return totalServersAdded_;
}

int LoadBalancer::getTotalServersRemovedCount() const {
    return totalServersRemoved_;
}

std::string LoadBalancer::formatServerTag(const WebServer& server) const {
    return "[" + balancerLabel_ + "] Server " + std::to_string(server.getId());
}