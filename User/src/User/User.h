#pragma once
#include <stdio.h>      // for printf() and fprintf()
#include <sys/socket.h> // for socket() and bind()
#include <arpa/inet.h>  // for sockaddr_in and inet_ntoa()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()
#include <cstdlib>

#include <algorithm>
#include "Common/Structures.h"

class User {
    public:
    User(const char *serverIPAddress, int serverPort);
    ~User() { close( sock ); }
    void run();

private:
    
    int sock;                        // Socket descriptor

    struct sockaddr_in echoServAddr; // Echo server address
    struct sockaddr_in fromAddr;     // Source address of echo

    unsigned short serverPort;     // Echo server port
    const char *serverIP;                    // IP address of server

        void dieWithError(const char *errorMessage);
}