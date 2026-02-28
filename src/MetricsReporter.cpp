#include "MetricsReporter.h"

#include "Logger.h"
#include "Request.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace {

const int kTaskTimeMinCycles = 1;
const int kTaskTimeMaxCycles = 10;
const std::size_t kQueueSampleSize = 8;
const std::size_t kServerSampleSize = 8;

// helper function to format the request for logging
/**
 * @details Format the request for logging.
 * @param request The request to format.
 * @return The formatted request.
 */
std::string formatRequestForLog(const Request& request) {
    std::ostringstream line;
    line << request.ipIn << " -> " << request.ipOut
         << " | type=" << jobTypeToString(request.jobType)
         << " | time=" << request.timeCycles;
    return line.str();
}

/**
 * @details Log the queue snapshot.
 * @param logger The logger to use.
 * @param balancer The balancer to use.
 * @param title The title of the snapshot.
 * @param maxItems The maximum number of items to log.
 */
void logQueueSnapshot(Logger& logger, const LoadBalancer& balancer, const std::string& title, std::size_t maxItems) {
    // log the queue size
    logger.log(title + " queue size: " + std::to_string(balancer.getQueueSize()));
    const std::vector<Request> snapshot = balancer.getQueueSnapshot(maxItems);

    // make sure the snapshot is not empty
    if (snapshot.empty()) {
        logger.log(title + " queue sample: [empty]");
        return;
    }

    logger.log(title + " queue sample (first " + std::to_string(snapshot.size()) + " request(s)):");
    for (std::size_t i = 0; i < snapshot.size(); ++i) {
        logger.log("  [" + std::to_string(i + 1) + "] " + formatRequestForLog(snapshot[i]));
    }
}

// helps to log a server snapshot
void logServerSnapshot(Logger& logger, const LoadBalancer& balancer, const std::string& title, std::size_t maxItems) {
    const std::vector<ServerSnapshot> snapshots = balancer.getServerSnapshots();
    logger.log(title + " server snapshot: active=" + std::to_string(balancer.getActiveServerCount()) +
               ", inactive=" + std::to_string(balancer.getIdleServerCount()) +
               ", total=" + std::to_string(balancer.getServerCount()));

    // make sure the snapshot is not empty
    if (snapshots.empty()) {
        logger.log("  [no servers available]");
        return;
    }

    // log the server snapshot
    const std::size_t sampleCount = std::min(maxItems, snapshots.size());
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const ServerSnapshot& server = snapshots[i];
        if (!server.busy) {
            logger.log("  Server " + std::to_string(server.id) + ": idle");
            continue;
        }
        logger.log("  Server " + std::to_string(server.id) + ": busy -> " + formatRequestForLog(server.request));
    }
}

} // namespace

LoadBalancerMetrics initializeMetrics(const LoadBalancer& balancer) {
    const std::size_t startingQueueSize = balancer.getQueueSize();
    LoadBalancerMetrics metrics;
    metrics.startingQueueSize = startingQueueSize;
    metrics.peakQueueSize = startingQueueSize;
    metrics.queueSizeAccumulator = static_cast<long long>(startingQueueSize);
    metrics.queueSamples = 1;
    metrics.totalIncomingRequests = 0;
    metrics.minServersObserved = balancer.getServerCount();
    metrics.maxServersObserved = balancer.getServerCount();
    return metrics;
}

void updateMetrics(LoadBalancerMetrics& metrics, const LoadBalancer& balancer, int newRequests) {
    metrics.totalIncomingRequests += newRequests;
    const std::size_t queueSize = balancer.getQueueSize();
    metrics.peakQueueSize = std::max(metrics.peakQueueSize, queueSize);
    metrics.queueSizeAccumulator += static_cast<long long>(queueSize);
    ++metrics.queueSamples;
    metrics.minServersObserved = std::min(metrics.minServersObserved, balancer.getServerCount());
    metrics.maxServersObserved = std::max(metrics.maxServersObserved, balancer.getServerCount());
}

void logSimulationStartSnapshot(
    Logger& logger,
    const LoadBalancer& balancer,
    int initialServers,
    int runCycles,
    int initialRequestCount) {
    logger.log("=== Simulation Start Snapshot ===");
    logger.log("Configured run: initial servers=" + std::to_string(initialServers) +
               ", run cycles=" + std::to_string(runCycles));
    logger.log("Requests at start of log: " + std::to_string(initialRequestCount));
    logger.log("Starting queue size: " + std::to_string(balancer.getQueueSize()));
    logger.log("Task time range (cycles): " + std::to_string(kTaskTimeMinCycles) + "-" +
               std::to_string(kTaskTimeMaxCycles));
    logQueueSnapshot(logger, balancer, "Start-of-log", kQueueSampleSize);
    logger.log("=== Cycle Activity ===");
}


// a bunch of metrics to log at the end of the simulation
void logSimulationEndSummary(
    Logger& logger,
    const LoadBalancer& balancer,
    const LoadBalancerMetrics& metrics,
    int initialServers,
    int cyclesRun) {
    const std::size_t endingQueueSize = balancer.getQueueSize();
    const int totalRequests = balancer.getTotalRequestsCount();
    const int requestsHandled = balancer.getTotalRequestsHandledCount();
    const int requestsRemaining = balancer.getTotalRequestsRemainingCount();

    // calculate the metrics using the information collected during the simulation
    const double averageQueueSize = metrics.queueSamples > 0
                                        ? static_cast<double>(metrics.queueSizeAccumulator) /
                                              static_cast<double>(metrics.queueSamples)
                                        : 0.0;
    const double throughputPerCycle = cyclesRun > 0
                                          ? static_cast<double>(requestsHandled) / static_cast<double>(cyclesRun)
                                          : 0.0;
    const double completionRatio = totalRequests > 0
                                       ? (100.0 * static_cast<double>(requestsHandled)) /
                                             static_cast<double>(totalRequests)
                                       : 0.0;

    // log the metrics
    logger.log("=== Simulation End Summary ===");
    logger.log("Cycles run: " + std::to_string(cyclesRun));
    logger.log("Starting queue size: " + std::to_string(metrics.startingQueueSize));
    logger.log("Ending queue size: " + std::to_string(endingQueueSize));
    logger.log("Task time range (cycles): " + std::to_string(kTaskTimeMinCycles) + "-" +
               std::to_string(kTaskTimeMaxCycles));
    logger.log("Total requests enqueued/generated: " + std::to_string(totalRequests));
    logger.log("Incoming requests generated during run: " + std::to_string(metrics.totalIncomingRequests));
    logger.log("Total requests handled/completed: " + std::to_string(requestsHandled));
    logger.log("Requests remaining at end: " + std::to_string(requestsRemaining));
    logger.log("Peak queue size observed: " + std::to_string(metrics.peakQueueSize));
    logger.log("Average queue size observed: " + std::to_string(averageQueueSize));
    logger.log("Server count start/end: " + std::to_string(initialServers) + " -> " +
               std::to_string(balancer.getServerCount()));
    logger.log("Server min/max observed: " + std::to_string(metrics.minServersObserved) + " / " +
               std::to_string(metrics.maxServersObserved));
    logger.log("End-status active/inactive servers: " + std::to_string(balancer.getActiveServerCount()) + " / " +
               std::to_string(balancer.getIdleServerCount()));
    logger.log("Servers added/removed by scaling: " + std::to_string(balancer.getTotalServersAddedCount()) + " / " +
               std::to_string(balancer.getTotalServersRemovedCount()));
    logger.log("Throughput (handled per cycle): " + std::to_string(throughputPerCycle));
    logger.log("Completion ratio: " + std::to_string(completionRatio) + "%");

    // Not yet implemented, will make an IP blocker
    // simulate a DDoS attack by blocking IPs
    logger.log("Rejected/discarded requests: 0 (no rejection path implemented)");

    // log the and server snapshots
    logServerSnapshot(logger, balancer, "End-of-run", kServerSampleSize);
}
