#pragma once
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>
#include <fcntl.h>

#include "Common/Protocol.h"

namespace SocketServer
{
    /**
     * @brief The response (message + additional data) when the server receives data from a client
     *
     */
    struct Response
    {
        sockaddr_in clientAddress;
        Protocol::Message msg;
    };

    /**
     * @brief This wraps the instantiation and the required C functions needed to operate a socket based server
     * @details This was created to reduce unnecessary boilerplate across both the client and tracker processes
     */
    class Server
    {
    public:
        Server(int serverPort, bool blockingMode);
        ~Server() { close(socket); }

        Response getIncomingBlockingMessage();

        void sendMessage(Protocol::Message msg);
        void sendReturnCode(sockaddr_in &clientAddress, Protocol::ReturnCode code, std::vector<std::string> additionalData = std::vector<std::string>()); // Send a return code with optional additional data

    private:
        int socket;
        sockaddr_in serverAddress;
        unsigned short serverPort;

        void dieWithError(const char *errorMessage);
    };

};