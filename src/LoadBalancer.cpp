/**
 * @file LoadBalancer.cpp
 * @brief Implements the LoadBalancer class.
 */

#include "LoadBalancer.h"

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
