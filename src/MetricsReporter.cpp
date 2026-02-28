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
    metrics.totalRejectedRequests = 0;
    metrics.minServersObserved = balancer.getServerCount();
    metrics.maxServersObserved = balancer.getServerCount();
    return metrics;
}

void updateMetrics(
    LoadBalancerMetrics& metrics,
    const LoadBalancer& balancer,
    int acceptedRequests,
    int rejectedRequests) {
    metrics.totalIncomingRequests += acceptedRequests;
    metrics.totalRejectedRequests += rejectedRequests;
    const std::size_t queueSize = balancer.getQueueSize();
    metrics.peakQueueSize = std::max(metrics.peakQueueSize, queueSize);
    metrics.queueSizeAccumulator += static_cast<long long>(queueSize);
    ++metrics.queueSamples;
    metrics.minServersObserved = std::min(metrics.minServersObserved, balancer.getServerCount());
    metrics.maxServersObserved = std::max(metrics.maxServersObserved, balancer.getServerCount());
}

void logSimulationStartSnapshot(
    Logger& logger,
    const std::string& label,
    const LoadBalancer& balancer,
    int initialServers,
    int runCycles,
    int initialRequestCount) {
    const std::string prefix = "[" + label + "] ";
    logger.log(prefix + "=== Simulation Start Snapshot ===");
    logger.log(prefix + "Configured run: initial servers=" + std::to_string(initialServers) +
               ", run cycles=" + std::to_string(runCycles));
    logger.log(prefix + "Requests at start of log: " + std::to_string(initialRequestCount));
    logger.log(prefix + "Starting queue size: " + std::to_string(balancer.getQueueSize()));
    logger.log(prefix + "Task time range (cycles): " + std::to_string(kTaskTimeMinCycles) + "-" +
               std::to_string(kTaskTimeMaxCycles));
    logQueueSnapshot(logger, balancer, prefix + "Start-of-log", kQueueSampleSize);
}


// a bunch of metrics to log at the end of the simulation
void logSimulationEndSummary(
    Logger& logger,
    const std::string& label,
    const LoadBalancer& balancer,
    const LoadBalancerMetrics& metrics,
    int initialServers,
    int cyclesRun) {
    const std::string prefix = "[" + label + "] ";
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
    const int totalGeneratedDuringRun = metrics.totalIncomingRequests + metrics.totalRejectedRequests;
    const int totalGeneratedIncludingInitial = totalRequests + metrics.totalRejectedRequests;
    const double rejectionRatio = totalGeneratedIncludingInitial > 0
                                      ? (100.0 * static_cast<double>(metrics.totalRejectedRequests)) /
                                            static_cast<double>(totalGeneratedIncludingInitial)
                                      : 0.0;

    // log the metrics
    logger.log(prefix + "=== Simulation End Summary ===");
    logger.log(prefix + "Cycles run: " + std::to_string(cyclesRun));
    logger.log(prefix + "Starting queue size: " + std::to_string(metrics.startingQueueSize));
    logger.log(prefix + "Ending queue size: " + std::to_string(endingQueueSize));
    logger.log(prefix + "Task time range (cycles): " + std::to_string(kTaskTimeMinCycles) + "-" +
               std::to_string(kTaskTimeMaxCycles));
    logger.log(prefix + "Total requests accepted/enqueued: " + std::to_string(totalRequests));
    logger.log(prefix + "Incoming requests accepted during run: " + std::to_string(metrics.totalIncomingRequests));
    logger.log(prefix + "Total generated during run (accepted + rejected): " + std::to_string(totalGeneratedDuringRun));
    logger.log(prefix + "Total requests handled/completed: " + std::to_string(requestsHandled));
    logger.log(prefix + "Requests remaining at end: " + std::to_string(requestsRemaining));
    logger.log(prefix + "Peak queue size observed: " + std::to_string(metrics.peakQueueSize));
    logger.log(prefix + "Average queue size observed: " + std::to_string(averageQueueSize));
    logger.log(prefix + "Server count start/end: " + std::to_string(initialServers) + " -> " +
               std::to_string(balancer.getServerCount()));
    logger.log(prefix + "Server min/max observed: " + std::to_string(metrics.minServersObserved) + " / " +
               std::to_string(metrics.maxServersObserved));
    logger.log(prefix + "End-status active/inactive servers: " + std::to_string(balancer.getActiveServerCount()) + " / " +
               std::to_string(balancer.getIdleServerCount()));
    logger.log(prefix + "Servers added/removed by scaling: " + std::to_string(balancer.getTotalServersAddedCount()) + " / " +
               std::to_string(balancer.getTotalServersRemovedCount()));
    logger.log(prefix + "Throughput (handled per cycle): " + std::to_string(throughputPerCycle));
    logger.log(prefix + "Completion ratio: " + std::to_string(completionRatio) + "%");

    logger.log(prefix + "Rejected/discarded requests: " + std::to_string(metrics.totalRejectedRequests));
    logger.log(prefix + "Rejection ratio: " + std::to_string(rejectionRatio) + "%");

    // log the and server snapshots
    logServerSnapshot(logger, balancer, prefix + "End-of-run", kServerSampleSize);
}
