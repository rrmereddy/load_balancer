/**
 * @file Config.cpp
 * @brief Implements configuration loading.
 */

#include "Config.h"

/**
 * @brief Loads the default simulation configuration.
 * @return Configuration populated with project default values.
 */
Config loadConfig() {
    Config config;
    config.initialServers = 10;
    config.runCycles = 10000;
    config.minQueuePerServer = 50;
    config.maxQueuePerServer = 80;
    config.scalingCheckInterval = 100;
    config.logFilePath = "load_balancer.txt";
    config.allowedIpRanges = {
        {"64.0.0.0", "191.255.255.255"}
    };
    config.blockedTrafficSimulationRate = 0.10;
    return config;
}
