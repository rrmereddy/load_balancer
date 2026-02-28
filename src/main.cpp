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
#include "Switch.h"
#include <iostream>
#include <random>
#include <string>

/**
 * @brief Randomly enqueue new requests to simulate live incoming traffic.
 * @return Accepted and rejected request counts for this cycle.
 */
struct IncomingTrafficResult {
    int acceptedProcessingRequests;
    int acceptedStreamingRequests;
    int rejectedProcessingRequests;
    int rejectedStreamingRequests;
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
    Switch& switchRouter,
    const IpBlocker& ipBlocker,
    double blockedTrafficRate,
    int currentClock,
    Logger& logger) {
    static std::mt19937 engine(std::random_device{}());
    std::bernoulli_distribution burstChance(0.70);
    std::uniform_int_distribution<int> newRequestDist(1, 10);
    IncomingTrafficResult result = {0, 0, 0, 0};

    if (!burstChance(engine)) {
        return result;
    }

    const int newRequests = newRequestDist(engine);
    std::string firstRejectedIp;
    for (int i = 0; i < newRequests; ++i) {
        const Request request = makeBiasedRequest(ipBlocker, blockedTrafficRate);
        if (!ipBlocker.isAllowed(request.ipIn)) {
            if (request.jobType == JobType::Streaming) {
                ++result.rejectedStreamingRequests;
            } else {
                ++result.rejectedProcessingRequests;
            }
            if (firstRejectedIp.empty()) {
                firstRejectedIp = request.ipIn;
            }
            continue;
        }
        switchRouter.routeRequest(request);
        if (request.jobType == JobType::Streaming) {
            ++result.acceptedStreamingRequests;
        } else {
            ++result.acceptedProcessingRequests;
        }
    }

    const int totalAccepted = result.acceptedProcessingRequests + result.acceptedStreamingRequests;
    const int totalRejected = result.rejectedProcessingRequests + result.rejectedStreamingRequests;

    if (totalAccepted > 0) {
        logger.log("Clock " + std::to_string(currentClock) +
                   ": Incoming traffic -> accepted " + std::to_string(totalAccepted) +
                   " request(s), processing=" + std::to_string(result.acceptedProcessingRequests) +
                   ", streaming=" + std::to_string(result.acceptedStreamingRequests));
    }

    if (totalRejected > 0) {
        logger.log("Clock " + std::to_string(currentClock) +
                   ": IP range blocker rejected " + std::to_string(totalRejected) +
                   " request(s), processing=" + std::to_string(result.rejectedProcessingRequests) +
                   ", streaming=" + std::to_string(result.rejectedStreamingRequests) +
                   " request(s)");
        if (!firstRejectedIp.empty()) {
            logger.log("Clock " + std::to_string(currentClock) +
                       ": Sample rejected source IP: " + firstRejectedIp);
        }
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

    // split initial capacity across two balancers with minimal config changes
    const int processingInitialServers = initialServers / 2;
    const int streamingInitialServers = initialServers - processingInitialServers;

    LoadBalancer processingBalancer(processingInitialServers, "Processing");
    LoadBalancer streamingBalancer(streamingInitialServers, "Streaming");
    Switch switchRouter(processingBalancer, streamingBalancer);

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
    int initialAcceptedProcessingRequests = 0;
    int initialAcceptedStreamingRequests = 0;
    int initialRejectedProcessingRequests = 0;
    int initialRejectedStreamingRequests = 0;
    std::string firstInitialRejectedIp;
    for (int i = 0; i < initialRequestTarget; ++i) {
        const Request request = makeBiasedRequest(ipBlocker, config.blockedTrafficSimulationRate);
        if (!ipBlocker.isAllowed(request.ipIn)) {
            if (request.jobType == JobType::Streaming) {
                ++initialRejectedStreamingRequests;
            } else {
                ++initialRejectedProcessingRequests;
            }
            if (firstInitialRejectedIp.empty()) {
                firstInitialRejectedIp = request.ipIn;
            }
            continue;
        }
        switchRouter.routeRequest(request);
        if (request.jobType == JobType::Streaming) {
            ++initialAcceptedStreamingRequests;
        } else {
            ++initialAcceptedProcessingRequests;
        }
    }
    const int initialAcceptedRequests = initialAcceptedProcessingRequests + initialAcceptedStreamingRequests;
    const int initialRejectedRequests = initialRejectedProcessingRequests + initialRejectedStreamingRequests;
    if (initialRejectedRequests > 0) {
        logger.log("Initial request load -> accepted " + std::to_string(initialAcceptedRequests) +
                   " (processing=" + std::to_string(initialAcceptedProcessingRequests) +
                   ", streaming=" + std::to_string(initialAcceptedStreamingRequests) + ")" +
                   ", rejected " + std::to_string(initialRejectedRequests) +
                   " (processing=" + std::to_string(initialRejectedProcessingRequests) +
                   ", streaming=" + std::to_string(initialRejectedStreamingRequests) + ")" +
                   ", sample source IP: " + firstInitialRejectedIp);
    }

    LoadBalancerMetrics processingMetrics = initializeMetrics(processingBalancer);
    LoadBalancerMetrics streamingMetrics = initializeMetrics(streamingBalancer);

    logSimulationStartSnapshot(
        logger,
        "Processing",
        processingBalancer,
        processingInitialServers,
        runCycles,
        processingBalancer.getTotalRequestsCount());
    logSimulationStartSnapshot(
        logger,
        "Streaming",
        streamingBalancer,
        streamingInitialServers,
        runCycles,
        streamingBalancer.getTotalRequestsCount());
    
    // start the simulation run
    logger.log("=== Simulation Run ===");
    int cyclesRun = 0;
    for (int i = 0; i < runCycles; ++i) {
        const IncomingTrafficResult newRequests =
            simulateIncomingRequests(
                switchRouter,
                ipBlocker,
                config.blockedTrafficSimulationRate,
                processingBalancer.getClock(),
                logger);

        // dispatch the requests to the servers
        processingBalancer.dispatchToServers(logger);
        streamingBalancer.dispatchToServers(logger);
        // tick the clock
        processingBalancer.tick();
        streamingBalancer.tick();
        // increment the number of cycles run
        ++cyclesRun;

        // check whether scaling is required for the servers to help handle the requests
        if (config.scalingCheckInterval > 0 && (cyclesRun % config.scalingCheckInterval) == 0) {
            processingBalancer.evaluateScaling(config.minQueuePerServer, config.maxQueuePerServer, logger);
            streamingBalancer.evaluateScaling(config.minQueuePerServer, config.maxQueuePerServer, logger);
        }

        updateMetrics(
            processingMetrics,
            processingBalancer,
            newRequests.acceptedProcessingRequests,
            newRequests.rejectedProcessingRequests);
        updateMetrics(
            streamingMetrics,
            streamingBalancer,
            newRequests.acceptedStreamingRequests,
            newRequests.rejectedStreamingRequests);
    }
    processingMetrics.totalRejectedRequests += initialRejectedProcessingRequests;
    streamingMetrics.totalRejectedRequests += initialRejectedStreamingRequests;

    logSimulationEndSummary(
        logger,
        "Processing",
        processingBalancer,
        processingMetrics,
        processingInitialServers,
        cyclesRun);
    logSimulationEndSummary(
        logger,
        "Streaming",
        streamingBalancer,
        streamingMetrics,
        streamingInitialServers,
        cyclesRun);

    const int totalServers = processingBalancer.getServerCount() + streamingBalancer.getServerCount();
    const int totalRequests = processingBalancer.getTotalRequestsCount() + streamingBalancer.getTotalRequestsCount();
    const int totalHandled =
        processingBalancer.getTotalRequestsHandledCount() + streamingBalancer.getTotalRequestsHandledCount();
    const int totalRemaining =
        processingBalancer.getTotalRequestsRemainingCount() + streamingBalancer.getTotalRequestsRemainingCount();

    std::cout << "Simulation complete. Servers: " << totalServers
              << ", Requests: " << totalRequests
              << ", Cycles run: " << cyclesRun
              << ", Requests handled: " << totalHandled
              << ", Requests remaining: " << totalRemaining
              << '\n';

    return 0;
}
