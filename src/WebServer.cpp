/**
 * @file WebServer.cpp
 * @brief Implements the WebServer class.
 */

#include "WebServer.h"
#include <sstream>

WebServer::WebServer(int id) : id_(id), currentRequest_() {
}

/**
 * @brief Get the server identifier.
 * @return Server id.
 */
int WebServer::getId() const {
    return id_; // return the server id
}

Request WebServer::getCurrentRequest() const {
    return currentRequest_;
}

/**
 * @brief Check if the server is currently processing a request.
 * @return True if busy, false otherwise.
 */
bool WebServer::isBusy() const {
    return currentRequest_.timeCycles > 0;
}

/**
 * @brief Assign a request to this server.
 * @param request The request to assign.
 */
void WebServer::assignRequest(const Request& request) {
    currentRequest_ = request;
}

/**
 * @brief Clear the current request.
 */
void WebServer::clearRequest() {
    currentRequest_ = Request();
}

/**
 * @brief Handle the current request. Process the request for one cycle.
 *        If the request is completed, return true.
 * @return True if the request is completed, false otherwise.
 */
bool WebServer::handleRequest() {
    // if the server is not busy, return false
    if (!isBusy()) {
        return true; // just a failsafe, but this shouldn't trigger
    }
    // decrement the time cycles of the request
    --currentRequest_.timeCycles;
    // if the request is completed, return true
    if (currentRequest_.timeCycles == 0) {
        return true;
    }
    return false;
}