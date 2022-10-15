#pragma once
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>

#include <algorithm>
#include "Common/Structures.h"

class Server
{
public:
    Server(int serverPort);
    ~Server() { close(sock); }
    void run();

private:
    int sock; // Socket

    sockaddr_in serverAddress; // Local address of server

    unsigned short serverPort; // Server port

    void dieWithError(const char *errorMessage);

    void sendReturnCode(sockaddr_in &clientAddress, Protocol::ReturnCode code, std::vector<std::string> additionalData); // Send a return code with optional additional data

    void parseClientMessage(Protocol::Message message, sockaddr_in &clientAddress);

    struct UserEntry
    { // The associated user information needed to register a new user
        std::string ipv4Addr;
        std::vector<int> communicationPorts;
        std::vector<std::string> followers; // Sorted list of who follows what handle
    };
    std::map<std::string, UserEntry> handleLookupTable; // This stores the key-value pairs of all the registered users by [@handle, {Registration information}]
};