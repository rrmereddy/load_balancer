/**
 * @file Logger.cpp
 * @brief Implements the Logger class.
 */

#include "Logger.h"

/**
 * @brief Constructs a logger that appends to a file.
 * @param filePath Path to log file.
 */
Logger::Logger(const std::string& filePath) : file_(filePath, std::ios::app) {}

/**
 * @brief Writes one line to the configured log file.
 * @param message Message text to append.
 */
void Logger::log(const std::string& message) {
    // log the message to the file
    if (file_.is_open()) {
        file_ << message << '\n';
    }
}
