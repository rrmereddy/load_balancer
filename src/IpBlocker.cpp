/**
 * @file IpBlocker.cpp
 * @brief Implements IP blocking checks.
 */

#include "IpBlocker.h"
#include <cstdio>
#include <utility>

namespace {

// helper function to parse an IPv4 address. Done by AI
bool parseIpv4(const std::string& ip, std::uint32_t& out) {
    unsigned int a = 0U;
    unsigned int b = 0U;
    unsigned int c = 0U;
    unsigned int d = 0U;
    char trailing = '\0';

    if (std::sscanf(ip.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &trailing) != 4) {
        return false;
    }

    if (a > 255U || b > 255U || c > 255U || d > 255U) {
        return false;
    }

    out = (static_cast<std::uint32_t>(a) << 24U) |
          (static_cast<std::uint32_t>(b) << 16U) |
          (static_cast<std::uint32_t>(c) << 8U) |
          static_cast<std::uint32_t>(d);
    return true;
}

} // namespace

// adds an allowed IP range to the blocker. This is just taken from the config file and makes sure that no invalid ranges are added
bool IpBlocker::addAllowedRange(const IpRange& range) {
    std::uint32_t start = 0;
    std::uint32_t end = 0;
    if (!parseIpv4(range.start, start) || !parseIpv4(range.end, end)) {
        return false;
    }

    if (start > end) {
        std::swap(start, end);
    }

    allowedRanges_.push_back({start, end});
    return true;
}

// checks if an IP is allowed by the blocker
bool IpBlocker::isAllowed(const std::string& ip) const {
    std::uint32_t value = 0;
    if (!parseIpv4(ip, value)) {
        return false;
    }

    for (const auto& range : allowedRanges_) {
        if (value >= range.first && value <= range.second) {
            return true;
        }
    }
    return false;
}
