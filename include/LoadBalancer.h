/**
 * @file LoadBalancer.h
 * @details Defines the LoadBalancer class interface.
 */

#pragma once

#include "Request.h"
#include "WebServer.h"
#include <queue>
#include <vector>

class Logger;

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
     * @details Check if there are any pending requests.
     * @return True if queue has items, false otherwise.
     */
    bool hasPendingRequests() const;

    /**
     * @details Dispatch at most one request per server and log it.
     * @param logger Logger to record handled requests.
     * @return True if any request was dispatched, false otherwise.
     */
    bool dispatchToServers(Logger& logger);

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

    /**
     * @details Evaluate scaling thresholds and add/remove capacity.
     * @param minQueuePerServer Minimum queue-per-server threshold.
     * @param maxQueuePerServer Maximum queue-per-server threshold.
     * @param logger Logger for scaling events.
     */
    void evaluateScaling(int minQueuePerServer, int maxQueuePerServer, Logger& logger);

    /**
     * @details Get the total number of requests.
     * @return Total requests.
     */
    int getTotalRequestsCount() const;

    /**
     * @details Get the total number of requests handled.
     * @return Total requests handled.
     */
    int getTotalRequestsHandledCount() const;

    /**
     * @details Get the total number of requests remaining.
     * @return Total requests remaining.
     */
    int getTotalRequestsRemainingCount() const;

private:
    /**
     * @details Remove one idle server if any and if requested.
     * @param logger Logger for removal events.
     * @return True if a server was removed.
     */
    bool removeOneIdleServer(Logger& logger);

    /**
     * @details Process deferred scale-down operations.
     * @param logger Logger for deferred removal events.
     */
    void processDeferredScaleDown(Logger& logger);

    std::queue<Request> requestQueue_;
    std::vector<WebServer> servers_;
    int clock_;
    int nextServerId_;
    int totalRequests_;
    int totalRequestsHandled_;
    int pendingScaleDown_;
};
