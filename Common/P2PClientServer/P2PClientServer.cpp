#include "P2PClientServer.h"

P2PClientServer::P2PClientServer(const char *targetIPAddress, int targetPort, int P2PServerPort)
{
    this->targetIP = targetIPAddress; // First arg: server IP address (dotted decimal)
    this->targetPort = targetPort;    // Second arg: Use given port

    server = std::make_unique<SocketServer::Server>(P2PServerPort);

    // Create a datagram/UDP socket
    if ((socket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        dieWithError("client: socket() failed");

    // Construct the server address structure
    memset(&targetAddress, 0, sizeof(targetAddress));    // Zero out structure
    targetAddress.sin_family = AF_INET;                  // Use internet addr family
    targetAddress.sin_addr.s_addr = inet_addr(targetIP); // Set server's IP address
    targetAddress.sin_port = htons(targetPort);          // Set server's port
}

void P2PClientServer::dieWithError(const char *errorMessage) // External error handling function
{
    perror(errorMessage);
    std::exit(1);
}