/**
 * @file Request.cpp
 * @brief Implements request helper utilities.
 */

#include "Request.h"
#include <random>
#include <string>

/**
 * @brief Converts job type to a compact single-character label.
 * @param type Job type to convert.
 * @return `P` for processing jobs and `S` for streaming jobs.
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
 * @brief Converts a single-character label to a job type.
 * @param typeChar Character representation of a job type.
 * @return Matching job type; defaults to Processing for unknown values.
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
 * @brief Converts job type to a descriptive string.
 * @param type Job type to convert.
 * @return Human-readable job type name.
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

/**
 * @brief Generates a random IPv4 address string.
 * @param engine Random engine used for octet generation.
 * @return Randomly generated IPv4 address.
 */
static std::string makeRandomIp(std::mt19937& engine) {
    std::uniform_int_distribution<int> octetDist(0, 255);
    return std::to_string(octetDist(engine)) + "." + std::to_string(octetDist(engine)) +
           "." + std::to_string(octetDist(engine)) + "." + std::to_string(octetDist(engine));
}

/**
 * @brief Creates a random request for simulation.
 * @return Random request with source/destination IP, job type, and duration.
 */
Request makeRandomRequest() {
    static std::mt19937 engine(std::random_device{}());
    std::uniform_int_distribution<int> timeDist(1, 10);

    Request request;
    // create a random IP address for the request
    request.ipIn = makeRandomIp(engine);
    request.ipOut = makeRandomIp(engine);
    request.timeCycles = timeDist(engine);

    // a radnom number is generated to determine the job type
    const int jobType = timeDist(engine) % 2;
    request.jobType = jobType == 0 ? JobType::Processing : JobType::Streaming;
    
    // return the request
    return request;
}
