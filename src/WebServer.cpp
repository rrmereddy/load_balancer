/**
 * @file WebServer.cpp
 * @brief Implements the WebServer class.
 */

#include "WebServer.h"

WebServer::WebServer(int id) : id_(id), currentRequest_() {
}

/**
 * @brief Get the server identifier.
 * @return Server id.
 */
int WebServer::getId() const {
    return id_; // return the server id
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