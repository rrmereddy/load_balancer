/**
 * @file IpBlocker.cpp
 * @brief Implements IP blocking checks.
 */

#include "IpBlocker.h"

void IpBlocker::addBlockedRange(const IpRange& range) {
    blockedRanges_.push_back(range);
}

bool IpBlocker::isBlocked(const std::string& ip) const {
    for (const auto& range : blockedRanges_) {
        if (ip >= range.start && ip <= range.end) {
            return true;
        }
    }
    return false;
}
