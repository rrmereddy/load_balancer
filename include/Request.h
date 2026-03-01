/**
 * @file Request.h
 * @brief Defines request data structures and utility helpers.
 */

#pragma once

#include <string>

/**
 * @brief Supported request job categories.
 */
enum class JobType {
    /** CPU-oriented workload. */
    Processing,
    /** Bandwidth-oriented workload. */
    Streaming
};

/**
 * @brief Represents a single web request in the simulation.
 */
struct Request {
    /** Source IPv4 address for the request. */
    std::string ipIn;
    /** Destination IPv4 address for the request. */
    std::string ipOut;
    /** Remaining processing time in simulation cycles. */
    int timeCycles;
    /** Workload category used for routing. */
    JobType jobType;
};

/**
 * @brief Convert JobType to a single character label.
 * @param type The job type.
 * @return 'P' for Processing, 'S' for Streaming.
 */
char jobTypeToChar(JobType type);

/**
 * @brief Convert a single character label to JobType.
 * @param typeChar The job type character.
 * @return JobType enum value.
 */
JobType jobTypeFromChar(char typeChar);

/**
 * @brief Convert JobType to a human-readable string.
 * @param type The job type.
 * @return String label for the job type.
 */
std::string jobTypeToString(JobType type);

/**
 * @brief Create a random request for simulation.
 * @return Randomly generated Request.
 */
Request makeRandomRequest();
