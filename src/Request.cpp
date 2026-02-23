/**
 * @file Request.cpp
 * @brief Implements Request utilities.
 */

#include "Request.h"
#include <random>
#include <string>

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


/**
 * This is used to make a random IP address for the request.
 * Funstions implemented by AI using the header file provided.
 */
static std::string makeRandomIp(std::mt19937& engine) {
    std::uniform_int_distribution<int> octetDist(0, 255);
    return std::to_string(octetDist(engine)) + "." + std::to_string(octetDist(engine)) +
           "." + std::to_string(octetDist(engine)) + "." + std::to_string(octetDist(engine));
}

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
