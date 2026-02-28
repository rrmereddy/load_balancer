#pragma once

#include "LoadBalancer.h"

class Logger;

/**
 * @details Aggregated metrics collected during a simulation run.
 */
struct LoadBalancerMetrics {
    std::size_t startingQueueSize;
    std::size_t peakQueueSize;
    long long queueSizeAccumulator;
    int queueSamples;
    int totalIncomingRequests;
    int minServersObserved;
    int maxServersObserved;
};

/**
 * @details Initialize run metrics from the current load balancer state.
 * @param balancer Current load balancer.
 * @return Initialized metrics object.
 */
LoadBalancerMetrics initializeMetrics(const LoadBalancer& balancer);

/**
 * @details Update run metrics after one cycle.
 * @param metrics Metrics to mutate.
 * @param balancer Current load balancer.
 * @param newRequests Number of incoming requests generated this cycle.
 */
void updateMetrics(LoadBalancerMetrics& metrics, const LoadBalancer& balancer, int newRequests);

/**
 * @details Emit the start-of-run snapshot section.
 * @param logger Output logger.
 * @param balancer Current load balancer.
 * @param initialServers Initial server count.
 * @param runCycles Requested cycle count.
 * @param initialRequestCount Initial requests seeded before cycle 0.
 */
void logSimulationStartSnapshot(
    Logger& logger,
    const LoadBalancer& balancer,
    int initialServers,
    int runCycles,
    int initialRequestCount);

/**
 * @details Emit the end-of-run summary section.
 * @param logger Output logger.
 * @param balancer Current load balancer.
 * @param metrics Aggregated run metrics.
 * @param initialServers Initial server count.
 * @param cyclesRun Actual cycles completed.
 */
void logSimulationEndSummary(
    Logger& logger,
    const LoadBalancer& balancer,
    const LoadBalancerMetrics& metrics,
    int initialServers,
    int cyclesRun);
