#pragma once
#include <stdio.h>      // for printf() and fprintf()
#include <sys/socket.h> // for socket() and bind()
#include <arpa/inet.h>  // for sockaddr_in and inet_ntoa()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()
#include <cstdlib>

#include <algorithm>
#include "Common/Structures.h"

class Server
{
    public:
        Server(int serverPort);
        void run();

    private:
        int sock; // Socket

        sockaddr_in serverAddress; //Local address of server
        
        unsigned short serverPort; // Server port

        void dieWithError(const char *errorMessage);

        void sendReturnCode(sockaddr_in &clientAddress, Protocol::ReturnCode code, std::string additionalData); //Send a return code with optional additional data
        
        void parseClientMessage(Protocol::Message message, sockaddr_in& clientAddress);
        

        struct UserEntry { //The associated user information needed to register a new user
            std::string ipv4Addr;
            std::vector<int> communicationPorts;
        };
        std::map<std::string, UserEntry> handleLookupTable; //This stores the key-value pairs of all the registered users by [@handle, {Registration information}]
};