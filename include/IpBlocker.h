/**
 * @file IpBlocker.h
 * @brief Defines IP range blocking interface.
 */

#pragma once

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
 * @details Simple IP blocker placeholder for firewall rules.
 */
class IpBlocker {
public:
    /**
     * @details Add a blocked IP range.
     * @param range The range to block.
     */
    void addBlockedRange(const IpRange& range);

    /**
     * @details Check if an IP is blocked.
     * @param ip IP address string.
     * @return True if blocked, false otherwise.
     */
    bool isBlocked(const std::string& ip) const;

private:
    std::vector<IpRange> blockedRanges_;
};
