#include "User.h"

User::User(const char *serverIPAddress, int serverPort)
{

    this->serverIP = serverIPAddress; // First arg: server IP address (dotted decimal)
    this->serverPort = serverPort;    // Second arg: Use given port

    // Create a datagram/UDP socket
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        dieWithError("client: socket() failed");

    // Construct the server address structure
    memset(&serverAddress, 0, sizeof(serverAddress));     // Zero out structure
    serverAddress.sin_family = AF_INET;                  // Use internet addr family
    serverAddress.sin_addr.s_addr = inet_addr(serverIP); // Set server's IP address
    serverAddress.sin_port = htons(serverPort);          // Set server's port

    std::cout << "Client is running with server on: " << serverIP << ":" << serverPort << std::endl;
}

void User::dieWithError(const char *errorMessage) // External error handling function
{
    perror(errorMessage);
    std::exit(1);
}

void User::run()
{
    while(true) {
    std::cout << "Command> ";

    std::string userInput;
    std::getline(std ::cin, userInput);

    Protocol::Message m;
    m.parseIncoming(userInput, ' ');
    m.sendMessage(sock, serverAddress);

    
    }
}