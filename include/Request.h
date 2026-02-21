/**
 * @file Request.h
 * @brief Defines the Request struct and job type.
 */

#pragma once

#include <string>

/**
 * @details Types of jobs that a request can represent.
 */
enum class JobType {
    Processing,
    Streaming
};

/**
 * @details Represents a single web request.
 */
struct Request {
    std::string ipIn;
    std::string ipOut;
    int timeCycles;
    JobType jobType;
};

/**
 * @details Convert JobType to a single character label.
 * @param type The job type.
 * @return 'P' for Processing, 'S' for Streaming.
 */
char jobTypeToChar(JobType type);

/**
 * @details Convert a single character label to JobType.
 * @param typeChar The job type character.
 * @return JobType enum value.
 */
JobType jobTypeFromChar(char typeChar);

/**
 * @details Convert JobType to a human-readable string.
 * @param type The job type.
 * @return String label for the job type.
 */
std::string jobTypeToString(JobType type);
