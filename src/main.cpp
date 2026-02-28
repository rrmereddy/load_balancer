/**
 * @file main.cpp
 * @brief Entry point for the load balancer.
 */

#include "Config.h"
#include "LoadBalancer.h"
#include "Logger.h"
#include "MetricsReporter.h"
#include <iostream>
#include <random>
#include <string>

/**
 * @brief Randomly enqueue new requests to simulate live incoming traffic.
 * @return Number of new requests added this cycle.
 */
static int simulateIncomingRequests(LoadBalancer& balancer) {
    static std::mt19937 engine(std::random_device{}());
    std::bernoulli_distribution burstChance(0.70);
    std::uniform_int_distribution<int> newRequestDist(1, 10);

    if (!burstChance(engine)) {
        return 0;
    }

    const int newRequests = newRequestDist(engine);
    for (int i = 0; i < newRequests; ++i) {
        balancer.enqueueRequest(makeRandomRequest());
    }
    return newRequests;
}

int main() {
    // load the configuration for the load balancer, this will be used to configure the load balancer
    // TODO: Maybe add a way to load the configuration from a file?
    Config config = loadConfig();

    // take user input for the number of servers to start with
    int initialServers;
    std::cout << "Enter the number of servers to start with: ";
    std::cin >> initialServers;

    // take user input for the number of cycles to run
    int runCycles;
    std::cout << "Enter the number of cycles to run: ";
    std::cin >> runCycles;

    // create the load balancer with the initial server count
    LoadBalancer balancer(initialServers);

    // create the logger for the load balancer, this will be used to log the load balancer
    Logger logger(config.logFilePath);

    // create the initial requests to enqueue
    const int initialRequestCount = initialServers * 100;
    for (int i = 0; i < initialRequestCount; ++i) {
        balancer.enqueueRequest(makeRandomRequest());
    }

    LoadBalancerMetrics metrics = initializeMetrics(balancer);
    logSimulationStartSnapshot(logger, balancer, initialServers, runCycles, initialRequestCount);

    int cyclesRun = 0;
    for (int i = 0; i < runCycles; ++i) {
        const int newRequests = simulateIncomingRequests(balancer);
        if (newRequests > 0) {
            logger.log("Clock " + std::to_string(balancer.getClock()) +
                       ": Incoming traffic -> enqueued " + std::to_string(newRequests) +
                       " new request(s)");
        }

        // dispatch the requests to the servers
        balancer.dispatchToServers(logger);
        // tick the clock
        balancer.tick();
        // increment the number of cycles run
        ++cyclesRun;

        // check whether scaling is required for the servers to help handle the requests
        if (config.scalingCheckInterval > 0 && (cyclesRun % config.scalingCheckInterval) == 0) {
            balancer.evaluateScaling(config.minQueuePerServer, config.maxQueuePerServer, logger);
        }

        updateMetrics(metrics, balancer, newRequests);
    }
    logSimulationEndSummary(logger, balancer, metrics, initialServers, cyclesRun);

    std::cout << "Simulation complete. Servers: " << balancer.getServerCount()
              << ", Requests: " << balancer.getTotalRequestsCount()
              << ", Cycles run: " << cyclesRun
              << ", Requests handled: " << balancer.getTotalRequestsHandledCount()
              << ", Requests remaining: " << balancer.getTotalRequestsRemainingCount()
              << '\n';

    return 0;
}
