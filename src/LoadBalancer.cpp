/**
 * @file LoadBalancer.cpp
 * @brief Implements the LoadBalancer class.
 */

#include "LoadBalancer.h"
#include "Logger.h"
#include <sstream>

LoadBalancer::LoadBalancer(int initialServers)
: requestQueue_(), servers_(), clock_(0), nextServerId_(1) {
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

void LoadBalancer::enqueueRequest(const Request& request) {
    requestQueue_.push(request);
}

bool LoadBalancer::hasPendingRequests() const {
    return !requestQueue_.empty();
}

bool LoadBalancer::dispatchToServers(Logger& logger) {
    bool dispatched = false;

    for (auto& server : servers_) {
        // if there are no requests, break
        if (requestQueue_.empty()) {
            break;
        }

        // if the server is busy, skip it
        if (server.isBusy()) {
            // tick the server, this will decrement the time cycles of the request if it is busy and clear the request if it is done
            if (server.handleRequest()) {
                // log the request
                std::ostringstream line;
                line << "Clock " << clock_ << ": Server " << server.getId()
                     << " completed request from " << server.getCurrentRequest().ipIn << " to " << server.getCurrentRequest().ipOut;
                logger.log(line.str());
                // clear the request
                server.clearRequest();
                ++totalRequestsHandled_;
                continue;
            }
            // log the request
            std::ostringstream line;
            line << "Clock " << clock_ << ": Server " << server.getId()
                 << " working on request from " << server.getCurrentRequest().ipIn << " to " << server.getCurrentRequest().ipOut
                 << " finishing in " << server.getCurrentRequest().timeCycles << " cycles";
            logger.log(line.str());
            continue;
        }

        // the server is not busy, so assign a new request
        // get the request from the queue
        Request request = requestQueue_.front();
        requestQueue_.pop();
        server.assignRequest(request);

        // log the request
        std::ostringstream line;
        line << "Clock " << clock_ << ": Server " << server.getId()
             << " assigned request from " << request.ipIn << " to " << request.ipOut;
        logger.log(line.str());

        dispatched = true;
    }

    return dispatched;
}

void LoadBalancer::tick() {
    ++clock_;
}

void LoadBalancer::addServer() {
    servers_.push_back(WebServer(nextServerId_++));
}

void LoadBalancer::removeServer() {
    if (!servers_.empty()) {
        servers_.pop_back();
    }
}

int LoadBalancer::getClock() const {
    return clock_;
}
