#pragma once
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>

#include "Common/Protocol.h"

class User
{
public:
    User(const char *serverIPAddress, int serverPort);
    ~User() { close(sock); }
    void run();

private:
    int sock; // Socket descriptor

    struct sockaddr_in serverAddress; // Echo server address
    struct sockaddr_in fromAddr;      // Source address of echo

    unsigned short serverPort; // Echo server port
    const char *serverIP;      // IP address of server

    void dieWithError(const char *errorMessage);
};