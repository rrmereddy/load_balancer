/**
 * @file WebServer.h
 * @brief Defines the WebServer class interface.
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
     * @brief Get the current request.
     * @return Current request.
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
     *        If the request is completed, clear the request.
     * @return True if the request is completed, false otherwise.
     */
    bool handleRequest();

private:
    int id_;
    Request currentRequest_;
};
