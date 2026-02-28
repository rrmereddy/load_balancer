#pragma once

#include "LoadBalancer.h"
#include "Request.h"

/**
 * @details Routes incoming requests to a load balancer based on job type.
 */
class Switch {
public:
    /**
     * @details Construct a switch that routes to dedicated balancers.
     * @param processingBalancer Target balancer for Processing jobs.
     * @param streamingBalancer Target balancer for Streaming jobs.
     */
    Switch(LoadBalancer& processingBalancer, LoadBalancer& streamingBalancer);

    /**
     * @details Route one request to the proper balancer by job type.
     * @param request Request to route.
     */
    void routeRequest(const Request& request);

private:
    LoadBalancer& processingBalancer_;
    LoadBalancer& streamingBalancer_;
};
