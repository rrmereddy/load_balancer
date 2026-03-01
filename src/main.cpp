/**
 * @file main.cpp
 * @brief Entry point and simulation orchestration.
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
 * @brief Tracks accepted/rejected incoming request counts per job type.
 */
struct IncomingTrafficResult {
    /** Accepted processing requests. */
    int acceptedProcessingRequests;
    /** Accepted streaming requests. */
    int acceptedStreamingRequests;
    /** Rejected processing requests. */
    int rejectedProcessingRequests;
    /** Rejected streaming requests. */
    int rejectedStreamingRequests;
};


/**
 * @brief Generates a request biased toward blocked or allowed traffic.
 * @param ipBlocker Configured allowlist checker.
 * @param blockedTrafficRate Target probability of generating blocked traffic.
 * @return Generated request matching target distribution when possible.
 */
static Request makeBiasedRequest(const IpBlocker& ipBlocker, double blockedTrafficRate) {

    // create a random engine for the request
    static std::mt19937 engine(std::random_device{}());
    // clamp the blocked traffic rate to be between 0 and 1
    double clampedRate = blockedTrafficRate;
    if (clampedRate < 0.0) {
        clampedRate = 0.0;
    } else if (clampedRate > 1.0) {
        clampedRate = 1.0;
    }

    // generate a random boolean based on the blocked traffic rate
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

/**
 * @brief Simulates one cycle of incoming traffic generation and routing.
 * @param switchRouter Switch used to route accepted requests.
 * @param ipBlocker Allowlist checker for source IPs.
 * @param blockedTrafficRate Target blocked-traffic ratio for generation.
 * @param currentClock Current simulation cycle for logging.
 * @param logger Logger used for traffic event output.
 * @return Accepted/rejected counts per job type for this cycle.
 */
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

/**
 * @brief Runs the full load-balancing simulation.
 * @return Exit code (`0` on success).
 */
int main() {
    // load the configuration for the load balancer, this will be used to configure the load balancer
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
    
    // create the load balancers for the processing and streaming jobs
    LoadBalancer processingBalancer(processingInitialServers, "Processing");
    LoadBalancer streamingBalancer(streamingInitialServers, "Streaming");

    // create the switch router for the load balancers
    Switch switchRouter(processingBalancer, streamingBalancer);

    // create the logger for the load balancer, this will be used to log the load balancer
    Logger logger(config.logFilePath);

    // configure allowed source ranges for the blocker
    // makes sure that the allowed IP ranges are valid
    IpBlocker ipBlocker;
    for (const auto& range : config.allowedIpRanges) {
        if (!ipBlocker.addAllowedRange({range.start, range.end})) {
            logger.log("Invalid allowed IP range ignored: " + range.start + " - " + range.end);
        }
    }
    logger.log("Blocked traffic simulation rate: " + std::to_string(config.blockedTrafficSimulationRate));

    // create the initial requests to enqueue for the load balancers
    const int initialRequestTarget = initialServers * 100;
    int initialAcceptedProcessingRequests = 0;
    int initialAcceptedStreamingRequests = 0;
    int initialRejectedProcessingRequests = 0;
    int initialRejectedStreamingRequests = 0;

    // initialize the initial requests for the load balancers
    for (int i = 0; i < initialRequestTarget; ++i) {
        const Request request = makeBiasedRequest(ipBlocker, config.blockedTrafficSimulationRate);
        // check if the request is blocked
        if (!ipBlocker.isAllowed(request.ipIn)) {
            // if the request is blocked, increment the rejected requests for the load balancers
            if (request.jobType == JobType::Streaming) {
                ++initialRejectedStreamingRequests;
            } else {
                ++initialRejectedProcessingRequests;
            }
            continue;
        }
        // route the request to the load balancer
        switchRouter.routeRequest(request);

        // increment the accepted requests for the load balancers
        if (request.jobType == JobType::Streaming) {
            ++initialAcceptedStreamingRequests;
        } else {
            ++initialAcceptedProcessingRequests;
        }
    }

    // log the initial requests and summary for the load balancers
    const int initialAcceptedRequests = initialAcceptedProcessingRequests + initialAcceptedStreamingRequests;
    const int initialRejectedRequests = initialRejectedProcessingRequests + initialRejectedStreamingRequests;
    if (initialRejectedRequests > 0) {
        logger.log("Initial request load -> accepted " + std::to_string(initialAcceptedRequests) +
                   " (processing=" + std::to_string(initialAcceptedProcessingRequests) +
                   ", streaming=" + std::to_string(initialAcceptedStreamingRequests) + ")" +
                   ", rejected " + std::to_string(initialRejectedRequests) +
                   " (processing=" + std::to_string(initialRejectedProcessingRequests) +
                   ", streaming=" + std::to_string(initialRejectedStreamingRequests) + ")");
    }

    // initialize the metrics for the load balancers
    LoadBalancerMetrics processingMetrics = initializeMetrics(processingBalancer);
    LoadBalancerMetrics streamingMetrics = initializeMetrics(streamingBalancer);

    // initial snapshots for the state of the load balancers
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

        // update the metrics for the load balancers
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

    // update the metrics for the load balancers with the initial requests
    processingMetrics.totalRejectedRequests += initialRejectedProcessingRequests;
    streamingMetrics.totalRejectedRequests += initialRejectedStreamingRequests;

    // log the end of the simulation summary for the load balancers
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
