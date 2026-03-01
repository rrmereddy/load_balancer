/**
 * @file MetricsReporter.h
 * @brief Declares metrics aggregation and reporting helpers.
 */

#pragma once

#include "LoadBalancer.h"
#include <string>

class Logger;

/**
 * @brief Aggregated metrics collected during a simulation run.
 */
struct LoadBalancerMetrics {
    /** Queue size observed at the beginning of the run. */
    std::size_t startingQueueSize;
    /** Maximum queue size observed during the run. */
    std::size_t peakQueueSize;
    /** Sum of sampled queue sizes for average calculation. */
    long long queueSizeAccumulator;
    /** Number of queue samples recorded. */
    int queueSamples;
    /** Accepted requests generated during the run. */
    int totalIncomingRequests;
    /** Rejected requests observed during the run. */
    int totalRejectedRequests;
    /** Minimum server count observed during the run. */
    int minServersObserved;
    /** Maximum server count observed during the run. */
    int maxServersObserved;
};

/**
 * @brief Initializes run metrics from current load balancer state.
 * @param balancer Current load balancer.
 * @return Initialized metrics object.
 */
LoadBalancerMetrics initializeMetrics(const LoadBalancer& balancer);

/**
 * @brief Updates run metrics after one cycle.
 * @param metrics Metrics to mutate.
 * @param balancer Current load balancer.
 * @param acceptedRequests Number of accepted requests in this cycle.
 * @param rejectedRequests Number of rejected requests in this cycle.
 */
void updateMetrics(
    LoadBalancerMetrics& metrics,
    const LoadBalancer& balancer,
    int acceptedRequests,
    int rejectedRequests);

/**
 * @brief Emits the start-of-run snapshot section.
 * @param logger Output logger.
 * @param label Workload label associated with the balancer.
 * @param balancer Current load balancer.
 * @param initialServers Initial server count.
 * @param runCycles Requested cycle count.
 * @param initialRequestCount Initial requests seeded before cycle 0.
 */
void logSimulationStartSnapshot(
    Logger& logger,
    const std::string& label,
    const LoadBalancer& balancer,
    int initialServers,
    int runCycles,
    int initialRequestCount);

/**
 * @brief Emits the end-of-run summary section.
 * @param logger Output logger.
 * @param label Workload label associated with the balancer.
 * @param balancer Current load balancer.
 * @param metrics Aggregated run metrics.
 * @param initialServers Initial server count.
 * @param cyclesRun Actual cycles completed.
 */
void logSimulationEndSummary(
    Logger& logger,
    const std::string& label,
    const LoadBalancer& balancer,
    const LoadBalancerMetrics& metrics,
    int initialServers,
    int cyclesRun);

/**
 * @brief Prints a concise end-of-run summary to the console.
 * @param processingBalancer Processing load balancer.
 * @param streamingBalancer Streaming load balancer.
 * @param cyclesRun Actual cycles completed.
 */
void printSimulationConsoleSummary(
    const LoadBalancer& processingBalancer,
    const LoadBalancer& streamingBalancer,
    int cyclesRun);
