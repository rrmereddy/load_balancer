/**
 * @file LoadBalancer.cpp
 * @brief Implements the LoadBalancer class.
 */

#include "LoadBalancer.h"
#include "Logger.h"
#include <iterator>
#include <sstream>

/**
 * @brief Constructs a load balancer with an initial server pool.
 * @param initialServers Number of servers to create initially.
 * @param balancerLabel Human-readable balancer label used in logs.
 */
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

/**
 * @brief Gets the number of queued requests.
 * @return Current queue length.
 */
std::size_t LoadBalancer::getQueueSize() const {
    return requestQueue_.size();
}

/**
 * @brief Gets a copy of the first queued requests.
 * @param maxItems Maximum number of requests to include.
 * @return Snapshot vector of queued requests.
 */
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

/**
 * @brief Gets the current snapshot of all servers.
 * @return Vector with one snapshot entry per server.
 */
std::vector<ServerSnapshot> LoadBalancer::getServerSnapshots() const {
    // get the server snapshots
    std::vector<ServerSnapshot> snapshot;

    // reserve the space for the server snapshots
    snapshot.reserve(servers_.size());

    // for each server, add the server snapshot to the vector
    for (const auto& server : servers_) {
        ServerSnapshot item;
        item.id = server.getId();
        item.busy = server.isBusy();
        item.request = item.busy ? server.getCurrentRequest() : Request();
        snapshot.push_back(item);
    }
    return snapshot;
}

/**
 * @brief Gets number of busy servers.
 * @return Count of active servers.
 */
int LoadBalancer::getActiveServerCount() const {
    int active = 0;
    for (const auto& server : servers_) {
        if (server.isBusy()) {
            ++active;
        }
    }
    return active;
}

/**
 * @brief Gets number of idle servers.
 * @return Count of inactive servers.
 */
int LoadBalancer::getIdleServerCount() const {
    return static_cast<int>(servers_.size()) - getActiveServerCount();
}

/**
 * @brief Adds a request to the queue and increments totals.
 * @param request Request to enqueue.
 */
void LoadBalancer::enqueueRequest(const Request& request) {
    requestQueue_.push(request);
    ++totalRequests_;
}

/**
 * @brief Checks whether any work remains to be processed.
 * @return True when queue is non-empty or a server is busy.
 */
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

/**
 * @brief Dispatches queued work and advances active server requests.
 * @param logger Logger used for dispatch/progress/completion messages.
 * @return True when at least one queued request was assigned this cycle.
 */
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

/**
 * @brief Advances the balancer clock by one cycle.
 */
void LoadBalancer::tick() {
    ++clock_;
}

/**
 * @brief Adds one new server to the pool.
 */
void LoadBalancer::addServer() {
    servers_.push_back(WebServer(nextServerId_++));
}

/**
 * @brief Removes one idle server, preferring most recently added.
 */
void LoadBalancer::removeServer() {
    for (auto it = servers_.rbegin(); it != servers_.rend(); ++it) {
        if (!it->isBusy()) {
            servers_.erase(std::next(it).base());
            return;
        }
    }
}

/**
 * @brief Gets the current clock value.
 * @return Simulation clock.
 */
int LoadBalancer::getClock() const {
    return clock_;
}

/**
 * @brief Applies auto-scaling rules based on queue-per-server ratio.
 * @param minQueuePerServer Threshold below which scale-down is considered.
 * @param maxQueuePerServer Threshold above which scale-up is triggered.
 * @param logger Logger used for scaling event messages.
 */
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
 * @brief Removes one idle server if available.
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

/**
 * @brief Processes pending scale-down requests once servers become idle.
 * @param logger Logger for scale-down events.
 */
void LoadBalancer::processDeferredScaleDown(Logger& logger) {
    while (pendingScaleDown_ > 0) {
        if (!removeOneIdleServer(logger)) {
            return;
        }
        --pendingScaleDown_;
    }
}

/**
 * @brief Gets total accepted request count.
 * @return Number of accepted requests.
 */
int LoadBalancer::getTotalRequestsCount() const {
    return totalRequests_;
}

/**
 * @brief Gets total completed request count.
 * @return Number of handled requests.
 */
int LoadBalancer::getTotalRequestsHandledCount() const {
    return totalRequestsHandled_;
}

/**
 * @brief Gets total number of requests remaining.
 * @return Total requests remaining.
 */
int LoadBalancer::getTotalRequestsRemainingCount() const {
    return totalRequests_ - totalRequestsHandled_;
}

/**
 * @brief Gets total number of servers added by scaling.
 * @return Number of added servers.
 */
int LoadBalancer::getTotalServersAddedCount() const {
    return totalServersAdded_;
}

/**
 * @brief Gets total number of servers removed by scaling.
 * @return Number of removed servers.
 */
int LoadBalancer::getTotalServersRemovedCount() const {
    return totalServersRemoved_;
}

/**
 * @brief Formats a server tag for consistent logging output.
 * @param server Server to describe.
 * @return String tag with balancer label and server id.
 */
std::string LoadBalancer::formatServerTag(const WebServer& server) const {
    return "[" + balancerLabel_ + "] Server " + std::to_string(server.getId());
}