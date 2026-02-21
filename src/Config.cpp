/**
 * @file Config.cpp
 * @brief Implements config loading.
 */

#include "Config.h"

/**
 * The config file helps with the configuration of the load balancer.
 * Confiurations are taken from the project requirements and can be changed as needed/
 */

Config loadConfig() {
    Config config;
    config.initialServers = 2;
    config.runCycles = 1000;
    config.minQueuePerServer = 50;
    config.maxQueuePerServer = 80;
    config.scaleCheckDelay = 10;
    config.logFilePath = "load_balancer.txt";
    return config;
}
