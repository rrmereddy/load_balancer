/**
 * @file Switch.h
 * @brief Declares the request routing switch.
 */

#pragma once

#include "LoadBalancer.h"
#include "Request.h"

/**
 * @brief Routes incoming requests to a load balancer based on job type.
 */
class Switch {
public:
    /**
     * @brief Constructs a switch that routes to dedicated balancers.
     * @param processingBalancer Target balancer for Processing jobs.
     * @param streamingBalancer Target balancer for Streaming jobs.
     */
    Switch(LoadBalancer& processingBalancer, LoadBalancer& streamingBalancer);

    /**
     * @brief Routes one request to the proper balancer by job type.
     * @param request Request to route.
     */
    void routeRequest(const Request& request);

private:
    /** Balancer handling Processing requests. */
    LoadBalancer& processingBalancer_;
    /** Balancer handling Streaming requests. */
    LoadBalancer& streamingBalancer_;
};
