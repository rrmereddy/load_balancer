/**
 * @file IpBlocker.h
 * @brief Defines IP range blocking interface.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * @details Represents an IP range with start and end (inclusive).
 */
struct IpRange {
    std::string start;
    std::string end;
};

/**
 * @details IPv4 allowlist blocker that rejects out-of-range sources.
 */
class IpBlocker {
public:
    /**
     * @details Add an allowed IPv4 range (inclusive).
     * @param range The range to allow.
     * @return True if range is valid and added, false otherwise.
     */
    bool addAllowedRange(const IpRange& range);

    /**
     * @details Check if an IPv4 address is allowed by configured ranges.
     * @param ip IP address string.
     * @return True if allowed, false otherwise.
     */
    bool isAllowed(const std::string& ip) const;

private:
    std::vector<std::pair<std::uint32_t, std::uint32_t>> allowedRanges_;
};
