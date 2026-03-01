/**
 * @file Config.h
 * @brief Defines configuration values used by the simulation.
 */

#pragma once

#include <string>
#include <vector>

/**
 * @brief Inclusive IPv4 range used by allowlist checks.
 */
struct ConfigIpRange {
    /** Start of the inclusive range. */
    std::string start;
    /** End of the inclusive range. */
    std::string end;
};

/**
 * @brief Holds run configuration values for the simulation.
 */
struct Config {
    /** Number of initial servers requested by configuration defaults. */
    int initialServers;
    /** Number of simulation cycles requested by configuration defaults. */
    int runCycles;
    /** Lower queue-per-server threshold used for scale-down checks. */
    int minQueuePerServer;
    /** Upper queue-per-server threshold used for scale-up checks. */
    int maxQueuePerServer;
    /** Number of cycles between scaling evaluations. */
    int scalingCheckInterval;
    /** Output path used by the logger. */
    std::string logFilePath;
    /** Allowed source IPv4 ranges for incoming traffic. */
    std::vector<ConfigIpRange> allowedIpRanges;
    /** Target ratio of blocked traffic to simulate. */
    double blockedTrafficSimulationRate;
};

/**
 * @brief Loads default configuration values for the project.
 * @return Populated configuration object.
 */
Config loadConfig();
