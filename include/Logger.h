/**
 * @file Logger.h
 * @brief Defines a basic logging interface.
 */

#pragma once

#include <fstream>
#include <string>

/**
 * @details Handles logging events and summaries.
 */
class Logger {
public:
    /**
     * @details Construct a new Logger.
     * @param filePath Log file path.
     */
    explicit Logger(const std::string& filePath);

    /**
     * @details Log a single line message.
     * @param message Text to log.
     */
    void log(const std::string& message);

private:
    std::ofstream file_;
};
