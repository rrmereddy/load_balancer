/**
 * @file main.cpp
 * @brief Entry point for the load balancer.
 */

#include "Config.h"
#include "LoadBalancer.h"
#include "Logger.h"
#include <iostream>
#include <string>

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

    logger.log("Load balancer initialized.");
    logger.log("Initial requests enqueued: " + std::to_string(initialRequestCount));

    int cyclesRun = 0;
    for (int i = 0; i < runCycles; ++i) {
        // dispatch the requests to the servers
        balancer.dispatchToServers(logger);
        // tick the clock
        balancer.tick();
        // increment the number of cycles run
        ++cyclesRun;

        if (!balancer.hasPendingRequests()) {
            break;
        }
    }

    std::cout << "Simulation complete. Servers: " << balancer.getServerCount()
              << ", Requests: " << initialRequestCount
              << ", Cycles run: " << cyclesRun << '\n';

    return 0;
}
