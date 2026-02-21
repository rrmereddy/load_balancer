/**
 * @file Config.h
 * @brief Defines configuration data for the load balancer.
 */

#pragma once

#include <string>

/**
 * @details Holds configuration values for the simulation.
 */
struct Config {
    int initialServers;
    int runCycles;
    int minQueuePerServer;
    int maxQueuePerServer;
    std::string logFilePath;
};

/**
 * @details Load the configuration given by the project requirements.
 * @return Loaded configuration.
 */
Config loadConfig();
