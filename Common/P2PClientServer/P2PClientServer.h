#pragma once
#include <memory>

#include "Common/SocketServer/SocketServer.h"
#include "Common/Protocol.h"

class P2PClientServer
{
public:
    P2PClientServer(const char *targetIPAddress, int targetPort, int P2PServerPort);

private:
    int socket;
    struct sockaddr_in targetAddress;
    unsigned short targetPort;
    const char *targetIP;
    void dieWithError(const char *errorMessage);

    std::unique_ptr<SocketServer::Server> server;
};
