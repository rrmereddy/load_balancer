/**
 * @file IpBlocker.h
 * @brief Declares IPv4 allowlist filtering utilities.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * @brief Represents an inclusive IPv4 range.
 */
struct IpRange {
    /** Start of the inclusive range. */
    std::string start;
    /** End of the inclusive range. */
    std::string end;
};

/**
 * @brief IPv4 allowlist blocker that rejects out-of-range sources.
 */
class IpBlocker {
public:
    /**
     * @brief Adds an allowed IPv4 range.
     * @param range The range to allow.
     * @return True if range is valid and added, false otherwise.
     */
    bool addAllowedRange(const IpRange& range);

    /**
     * @brief Checks whether an IPv4 address is within allowed ranges.
     * @param ip IP address string.
     * @return True if allowed, false otherwise.
     */
    bool isAllowed(const std::string& ip) const;

private:
    /** Stored allowed IPv4 ranges in 32-bit numeric form. */
    std::vector<std::pair<std::uint32_t, std::uint32_t>> allowedRanges_;
};
