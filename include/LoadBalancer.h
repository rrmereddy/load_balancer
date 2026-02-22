/**
 * @file LoadBalancer.h
 * @details Defines the LoadBalancer class interface.
 */

#pragma once

#include "Request.h"
#include "WebServer.h"
#include <queue>
#include <vector>

/**
 * @details Manages a queue of requests and a pool of web servers.
 */
class LoadBalancer {
public:
    /**
     * @details Construct a new LoadBalancer.
     * @param initialServers Number of servers to start with.
     */
    explicit LoadBalancer(int initialServers);

    /**
     * @details Get the number of servers in the pool.
     * @return Server count.
     */
    int getServerCount() const;

    /**
     * @details Get the number of requests in the queue.
     * @return Queue size.
     */
    std::size_t getQueueSize() const;

    /**
     * @details Enqueue a new request.
     * @param request Request to add.
     */
    void enqueueRequest(const Request& request);

    /**
     * @details Advance the internal clock by one cycle.
     */
    void tick();

    /**
     * @details Add a server to the pool.
     */
    void addServer();

    /**
     * @details Remove a server from the pool if possible.
     */
    void removeServer();

    /**
     * @details Get the current clock cycle.
     * @return Clock cycle count.
     */
    int getClock() const;

private:
    std::queue<Request> requestQueue_;
    std::vector<WebServer> servers_;
    int clock_;
    int nextServerId_;
};
