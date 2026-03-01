/**
 * @file IpBlocker.cpp
 * @brief Implements IPv4 allowlist checks.
 */

#include "IpBlocker.h"
#include <cstdio>
#include <utility>

namespace {

/**
 * @brief Parses an IPv4 string into a 32-bit integer.
 * @param ip Dotted-decimal IPv4 string.
 * @param out Output integer value when parsing succeeds.
 * @return True when parsing succeeds and the address is valid.
 */
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

/**
 * @brief Adds an allowed IPv4 range to the blocker.
 * @param range Inclusive string range to add.
 * @return True when both addresses parse successfully.
 */
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

/**
 * @brief Checks whether an IPv4 address is allowed.
 * @param ip Source IPv4 string.
 * @return True when the address falls in any configured range.
 */
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
