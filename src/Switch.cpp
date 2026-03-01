/**
 * @file Switch.cpp
 * @brief Implements request routing switch behavior.
 */

#include "Switch.h"

/**
 * @brief Constructs a switch with references to both balancers.
 * @param processingBalancer Balancer for processing jobs.
 * @param streamingBalancer Balancer for streaming jobs.
 */
Switch::Switch(LoadBalancer& processingBalancer, LoadBalancer& streamingBalancer)
    : processingBalancer_(processingBalancer), streamingBalancer_(streamingBalancer) {
}

/**
 * @brief Routes a request to the appropriate balancer.
 * @param request Request to route.
 */
void Switch::routeRequest(const Request& request) {
    if (request.jobType == JobType::Streaming) {
        streamingBalancer_.enqueueRequest(request);
        return;
    }
    processingBalancer_.enqueueRequest(request);
}
