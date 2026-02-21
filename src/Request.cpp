/**
 * @file Request.cpp
 * @brief Implements Request utilities.
 */

#include "Request.h"

/**
 * This is used to convert the job type to a character for logging.
 */
char jobTypeToChar(JobType type) {
    switch (type) {
    case JobType::Processing:
        return 'P';
    case JobType::Streaming:
        return 'S';
    default:
        return 'P';
    }
}

/**
 * This is used to convert the job type from a character to a job type enum.
 */
JobType jobTypeFromChar(char typeChar) {
    switch (typeChar) {
    case 'S':
    case 's':
        return JobType::Streaming;
    case 'P':
    case 'p':
    default:
        return JobType::Processing;
    }
}

/**
 * This is used to convert the job type to a string for logging.
 */
std::string jobTypeToString(JobType type) {
    switch (type) {
    case JobType::Processing:
        return "Processing";
    case JobType::Streaming:
        return "Streaming";
    default:
        return "Processing";
    }
}
