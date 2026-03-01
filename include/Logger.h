/**
 * @file Logger.h
 * @brief Declares a simple file logger.
 */

#pragma once

#include <fstream>
#include <string>

/**
 * @brief Handles logging events and summaries.
 */
class Logger {
public:
    /**
     * @brief Constructs a logger that appends to a file.
     * @param filePath Log file path.
     */
    explicit Logger(const std::string& filePath);

    /**
     * @brief Logs a single line message.
     * @param message Text to log.
     */
    void log(const std::string& message);

private:
    /** Output file stream for append-only logging. */
    std::ofstream file_;
};
