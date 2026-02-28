/**
 * @file main.cpp
 * @brief Entry point for the load balancer.
 */

#include "Config.h"
#include "IpBlocker.h"
#include "LoadBalancer.h"
#include "Logger.h"
#include "MetricsReporter.h"
#include "Request.h"
#include <iostream>
#include <random>
#include <string>

/**
 * @brief Randomly enqueue new requests to simulate live incoming traffic.
 * @return Accepted and rejected request counts for this cycle.
 */
struct IncomingTrafficResult {
    int acceptedRequests;
    int rejectedRequests;
};

static Request makeBiasedRequest(const IpBlocker& ipBlocker, double blockedTrafficRate) {
    static std::mt19937 engine(std::random_device{}());
    double clampedRate = blockedTrafficRate;
    if (clampedRate < 0.0) {
        clampedRate = 0.0;
    } else if (clampedRate > 1.0) {
        clampedRate = 1.0;
    }

    const bool shouldBeBlocked = std::bernoulli_distribution(clampedRate)(engine);
    Request fallback = makeRandomRequest();
    bool fallbackBlocked = !ipBlocker.isAllowed(fallback.ipIn);
    if (fallbackBlocked == shouldBeBlocked) {
        return fallback;
    }

    // Keep generation simple while biasing toward the requested blocked/allowed ratio.
    for (int i = 0; i < 32; ++i) {
        Request candidate = makeRandomRequest();
        const bool candidateBlocked = !ipBlocker.isAllowed(candidate.ipIn);
        if (candidateBlocked == shouldBeBlocked) {
            return candidate;
        }
    }
    return fallback;
}

static IncomingTrafficResult simulateIncomingRequests(
    LoadBalancer& balancer,
    const IpBlocker& ipBlocker,
    double blockedTrafficRate,
    Logger& logger) {
    static std::mt19937 engine(std::random_device{}());
    std::bernoulli_distribution burstChance(0.70);
    std::uniform_int_distribution<int> newRequestDist(1, 10);
    IncomingTrafficResult result = {0, 0};

    if (!burstChance(engine)) {
        return result;
    }

    const int newRequests = newRequestDist(engine);
    std::string firstRejectedIp;
    for (int i = 0; i < newRequests; ++i) {
        const Request request = makeBiasedRequest(ipBlocker, blockedTrafficRate);
        if (!ipBlocker.isAllowed(request.ipIn)) {
            ++result.rejectedRequests;
            if (firstRejectedIp.empty()) {
                firstRejectedIp = request.ipIn;
            }
            continue;
        }
        balancer.enqueueRequest(request);
        ++result.acceptedRequests;
    }

    if (result.acceptedRequests > 0) {
        logger.log("Clock " + std::to_string(balancer.getClock()) +
                   ": Incoming traffic -> accepted " + std::to_string(result.acceptedRequests) +
                   " request(s)");
    }

    if (result.rejectedRequests > 0) {
        logger.log("Clock " + std::to_string(balancer.getClock()) +
                   ": IP range blocker rejected " + std::to_string(result.rejectedRequests) +
                   " request(s), sample source IP: " + firstRejectedIp);
    }
    return result;
}

int main() {
    // load the configuration for the load balancer, this will be used to configure the load balancer
    // TODO: Maybe add a way to load the configuration from a file?
    Config config = loadConfig();

    // take user input for the number of servers to start with
    int initialServers;
    std::cout << "Enter the number of servers to start with: ";
    std::cin >> initialServers;

    // take user input for the number of cycles to run
    int runCycles;
    std::cout << "Enter the number of cycles to run: ";
    std::cin >> runCycles;

    // create the load balancer with the initial server count
    LoadBalancer balancer(initialServers);

    // create the logger for the load balancer, this will be used to log the load balancer
    Logger logger(config.logFilePath);

    // configure allowed source ranges for the blocker
    IpBlocker ipBlocker;
    for (const auto& range : config.allowedIpRanges) {
        if (!ipBlocker.addAllowedRange({range.start, range.end})) {
            logger.log("Invalid allowed IP range ignored: " + range.start + " - " + range.end);
        }
    }
    logger.log("Blocked traffic simulation rate: " + std::to_string(config.blockedTrafficSimulationRate));

    // create the initial requests to enqueue
    const int initialRequestTarget = initialServers * 100;
    int initialAcceptedRequests = 0;
    int initialRejectedRequests = 0;
    std::string firstInitialRejectedIp;
    for (int i = 0; i < initialRequestTarget; ++i) {
        const Request request = makeBiasedRequest(ipBlocker, config.blockedTrafficSimulationRate);
        if (!ipBlocker.isAllowed(request.ipIn)) {
            ++initialRejectedRequests;
            if (firstInitialRejectedIp.empty()) {
                firstInitialRejectedIp = request.ipIn;
            }
            continue;
        }
        balancer.enqueueRequest(request);
        ++initialAcceptedRequests;
    }
    if (initialRejectedRequests > 0) {
        logger.log("Initial request load -> accepted " + std::to_string(initialAcceptedRequests) +
                   ", rejected " + std::to_string(initialRejectedRequests) +
                   ", sample source IP: " + firstInitialRejectedIp);
    }

    LoadBalancerMetrics metrics = initializeMetrics(balancer);
    metrics.totalRejectedRequests += initialRejectedRequests;
    logSimulationStartSnapshot(logger, balancer, initialServers, runCycles, initialAcceptedRequests);

    int cyclesRun = 0;
    for (int i = 0; i < runCycles; ++i) {
        const IncomingTrafficResult newRequests =
            simulateIncomingRequests(balancer, ipBlocker, config.blockedTrafficSimulationRate, logger);

        // dispatch the requests to the servers
        balancer.dispatchToServers(logger);
        // tick the clock
        balancer.tick();
        // increment the number of cycles run
        ++cyclesRun;

        // check whether scaling is required for the servers to help handle the requests
        if (config.scalingCheckInterval > 0 && (cyclesRun % config.scalingCheckInterval) == 0) {
            balancer.evaluateScaling(config.minQueuePerServer, config.maxQueuePerServer, logger);
        }

        updateMetrics(metrics, balancer, newRequests.acceptedRequests, newRequests.rejectedRequests);
    }
    logSimulationEndSummary(logger, balancer, metrics, initialServers, cyclesRun);

    std::cout << "Simulation complete. Servers: " << balancer.getServerCount()
              << ", Requests: " << balancer.getTotalRequestsCount()
              << ", Cycles run: " << cyclesRun
              << ", Requests handled: " << balancer.getTotalRequestsHandledCount()
              << ", Requests remaining: " << balancer.getTotalRequestsRemainingCount()
              << '\n';

    return 0;
}
