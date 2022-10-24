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
#include "Common/P2PClientServer/P2PClientServer.h"

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
        std::vector<int> P2PServerPorts;
        std::vector<std::string> followers; // Sorted list of who follows what handle
        bool tweetInProgress;       // The User has sent a tweet command and the Tracker has not received an end-tweet command back
    };
    std::map<std::string, UserEntry> handleLookupTable; // This stores the key-value pairs of all the registered users by [@handle, {Registration information}]
};