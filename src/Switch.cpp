#include "Switch.h"

Switch::Switch(LoadBalancer& processingBalancer, LoadBalancer& streamingBalancer)
    : processingBalancer_(processingBalancer), streamingBalancer_(streamingBalancer) {
}

void Switch::routeRequest(const Request& request) {
    if (request.jobType == JobType::Streaming) {
        streamingBalancer_.enqueueRequest(request);
        return;
    }
    processingBalancer_.enqueueRequest(request);
}
