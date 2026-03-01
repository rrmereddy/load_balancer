/**
 * @file WebServer.h
 * @brief Declares the WebServer class.
 */

#pragma once

#include "Request.h"
class Logger;
/**
 * @brief Represents a single web server in the pool.
 */
class WebServer {
public:
    /**
     * @brief Construct a new WebServer.
     * @param id Unique identifier for the server.
     */
    explicit WebServer(int id);

    /**
     * @brief Get the server identifier.
     * @return Server id.
     */
    int getId() const;

    /**
     * @brief Gets the request currently assigned to the server.
     * @return Copy of the current request state.
     */
    Request getCurrentRequest() const;

    /**
     * @brief Check if the server is currently processing a request.
     * @return True if busy, false otherwise.
     */
    bool isBusy() const;

    /**
     * @brief Assign a request to this server.
     * @param request The request to process.
     */
    void assignRequest(const Request& request);

    /**
     * @brief Clear the current request.
     */
    void clearRequest();

    /**
     * @brief Handle the current request. Process the request for one cycle.
     * @return True when the request has completed this cycle; otherwise false.
     */
    bool handleRequest();

private:
    /** Unique server identifier. */
    int id_;
    /** Request currently being processed, if any. */
    Request currentRequest_;
};
