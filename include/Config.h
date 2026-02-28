/**
 * @file Config.h
 * @details Defines configuration data for the load balancer.
 */

#pragma once

#include <string>
#include <vector>

/**
 * @details Inclusive IPv4 range used by config-driven allowlist checks.
 */
struct ConfigIpRange {
    std::string start;
    std::string end;
};

/**
 * @details Holds configuration values for the simulation.
 */
struct Config {
    int initialServers;
    int runCycles;
    int minQueuePerServer;
    int maxQueuePerServer;
    int scalingCheckInterval;
    std::string logFilePath;
    std::vector<ConfigIpRange> allowedIpRanges;
    double blockedTrafficSimulationRate;
};

/**
 * @details Load the configuration given by the project requirements.
 * @return Loaded configuration.
 */
Config loadConfig();
