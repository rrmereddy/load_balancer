/**
 * @file main.cpp
 * @brief Entry point for the load balancer.
 */

#include "Config.h"
#include "LoadBalancer.h"
#include "Logger.h"
#include <iostream>

int main() {
    // load the configuration for the load balancer, this will be used to configure the load balancer
    // TODO: Maybe add a way to load the configuration from a file?
    Config config = loadConfig();

    // create the load balancer with the initial server count
    LoadBalancer balancer(config.initialServers);

    // create the logger for the load balancer, this will be used to log the load balancer
    Logger logger(config.logFilePath);

    logger.log("Load balancer initialized.");
    std::cout << "Setup complete. Servers: " << balancer.getServerCount()
              << ", Run cycles: " << config.runCycles << '\n';

    return 0;
}
