/**
 * @file LoadBalancer.h
 * @brief Declares the LoadBalancer class and related snapshots.
 */

#pragma once

#include "Request.h"
#include "WebServer.h"
#include <queue>
#include <string>
#include <vector>

class Logger;

/**
 * @brief Represents one server state at a point in time.
 */
struct ServerSnapshot {
    /** Server identifier. */
    int id;
    /** True when the server is currently processing a request. */
    bool busy;
    /** Current request snapshot (default request when idle). */
    Request request;
};

/**
 * @brief Manages request queues and a pool of web servers.
 */
class LoadBalancer {
public:
    /**
     * @brief Constructs a new load balancer.
     * @param initialServers Number of servers to start with.
     * @param balancerLabel Human-readable label for logs.
     */
    explicit LoadBalancer(int initialServers, const std::string& balancerLabel = "LoadBalancer");

    /**
     * @brief Gets the number of servers in the pool.
     * @return Server count.
     */
    int getServerCount() const;

    /**
     * @brief Gets the number of requests in the queue.
     * @return Queue size.
     */
    std::size_t getQueueSize() const;

    /**
     * @brief Gets a non-destructive snapshot of queued requests.
     * @param maxItems Maximum number of queued requests to include.
     * @return Requests from the front of the queue.
     */
    std::vector<Request> getQueueSnapshot(std::size_t maxItems) const;

    /**
     * @brief Gets a snapshot of every server and its current state.
     * @return Vector of server snapshots.
     */
    std::vector<ServerSnapshot> getServerSnapshots() const;

    /**
     * @brief Gets the number of currently active (busy) servers.
     * @return Active server count.
     */
    int getActiveServerCount() const;

    /**
     * @brief Gets the number of currently inactive (idle) servers.
     * @return Idle server count.
     */
    int getIdleServerCount() const;

    /**
     * @brief Enqueues a new request.
     * @param request Request to add.
     */
    void enqueueRequest(const Request& request);

    /**
     * @brief Checks whether work remains in queue or servers.
     * @return True if queue has items, false otherwise.
     */
    bool hasPendingRequests() const;

    /**
     * @brief Dispatches queued requests and advances active work.
     * @param logger Logger to record handled requests.
     * @return True if any request was dispatched, false otherwise.
     */
    bool dispatchToServers(Logger& logger);

    /**
     * @brief Advances the internal clock by one cycle.
     */
    void tick();

    /**
     * @brief Adds one server to the pool.
     */
    void addServer();

    /**
     * @brief Removes one idle server from the pool if possible.
     */
    void removeServer();

    /**
     * @brief Gets the current clock cycle.
     * @return Clock cycle count.
     */
    int getClock() const;

    /**
     * @brief Evaluates scaling thresholds and adjusts capacity.
     * @param minQueuePerServer Minimum queue-per-server threshold.
     * @param maxQueuePerServer Maximum queue-per-server threshold.
     * @param logger Logger for scaling events.
     */
    void evaluateScaling(int minQueuePerServer, int maxQueuePerServer, Logger& logger);

    /**
     * @brief Gets the total accepted requests count.
     * @return Total requests.
     */
    int getTotalRequestsCount() const;

    /**
     * @brief Gets the total completed requests count.
     * @return Total requests handled.
     */
    int getTotalRequestsHandledCount() const;

    /**
     * @brief Gets the total requests not yet completed.
     * @return Total requests remaining.
     */
    int getTotalRequestsRemainingCount() const;

    /**
     * @brief Gets the total number of servers added by scaling.
     * @return Servers added.
     */
    int getTotalServersAddedCount() const;

    /**
     * @brief Gets the total number of servers removed by scaling.
     * @return Servers removed.
     */
    int getTotalServersRemovedCount() const;

private:
    /**
     * @brief Formats a labeled server tag for logs.
     * @param server Server whose tag should be generated.
     * @return Formatted server label.
     */
    std::string formatServerTag(const WebServer& server) const;

    /**
     * @brief Removes one idle server if available.
     * @param logger Logger for removal events.
     * @return True if a server was removed.
     */
    bool removeOneIdleServer(Logger& logger);

    /**
     * @brief Processes deferred scale-down operations.
     * @param logger Logger for deferred removal events.
     */
    void processDeferredScaleDown(Logger& logger);

    /** FIFO queue of pending requests. */
    std::queue<Request> requestQueue_;
    /** Pool of simulated servers. */
    std::vector<WebServer> servers_;
    /** Current simulation clock. */
    int clock_;
    /** Identifier to assign to the next created server. */
    int nextServerId_;
    /** Total accepted/enqueued requests. */
    int totalRequests_;
    /** Total completed requests. */
    int totalRequestsHandled_;
    /** Requested scale-down removals waiting for idle capacity. */
    int pendingScaleDown_;
    /** Total servers added via scaling. */
    int totalServersAdded_;
    /** Total servers removed via scaling. */
    int totalServersRemoved_;
    /** Label used in log output to identify this balancer. */
    std::string balancerLabel_;
};
