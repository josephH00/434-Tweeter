#pragma once
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>

#include "Common/Protocol.h"

/**
 * @brief The response (message + additional data) when the server receives data from a client
 * 
 */
struct SocketServerResponse {
    sockaddr_in clientIPAddress;
    Protocol::Message msg;
};

/**
 * @brief This wraps the instantiation and the required C functions needed to operate a socket based server
 * @details This was created to reduce unnecessary boilerplate across both the client and tracker processes
 */
class SocketServer
{
public:
    SocketServer(int serverPort);
    ~SocketServer() { close(socket); }

    SocketServerResponse getIncomingBlockingMessage();

    void sendMessage(Protocol::Message msg);
    void sendReturnCode(sockaddr_in &clientAddress, Protocol::ReturnCode code, std::vector<std::string> additionalData); // Send a return code with optional additional data

private:
    int socket;
    sockaddr_in serverAddress;
    unsigned short serverPort;

    void dieWithError(const char *errorMessage);
};