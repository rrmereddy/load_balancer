/**
 * @file Logger.cpp
 * @brief Implements the Logger class.
 */

#include "Logger.h"

Logger::Logger(const std::string& filePath) : file_(filePath, std::ios::app) {}

void Logger::log(const std::string& message) {
    // log the message to the file
    if (file_.is_open()) {
        file_ << message << '\n';
    }
}
