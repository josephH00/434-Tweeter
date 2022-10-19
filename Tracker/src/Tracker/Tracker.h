#pragma once
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>
#include <memory>

#include "Common/Protocol.h"
#include "Common/SocketServer/SocketServer.h"

class Tracker
{
public:
    Tracker(int serverPort);
    void run();

private:
    std::unique_ptr<SocketServer::Server> socketServer;

    void parseClientMessage(SocketServer::Response);

    struct UserEntry // The associated user information needed to register a new user
    {
        std::string ipv4Addr;
        std::vector<int> communicationPorts;
        std::vector<std::string> followers; // Sorted list of who follows what handle
    };
    std::map<std::string, UserEntry> handleLookupTable; // This stores the key-value pairs of all the registered users by [@handle, {Registration information}]
};