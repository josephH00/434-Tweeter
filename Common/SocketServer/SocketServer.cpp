#include "SocketServer.h"

namespace SocketServer
{

    Server::Server(int serverPort)
    {
        this->serverPort = serverPort;

        // Create socket for sending/receiving datagrams
        if ((socket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            dieWithError("server: socket() failed");

        // Construct local address structure
        memset(&serverAddress, 0, sizeof(serverAddress));  // Zero out structure
        serverAddress.sin_family = AF_INET;                // Internet address family
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
        serverAddress.sin_port = htons(serverPort);        // Local port

        // Bind to the local address
        if (bind(socket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
            dieWithError("server: bind() failed");

        std::cout << "Server is listening on port " << serverPort << std::endl;
    }

    void Server::dieWithError(const char *errorMessage) // External error handling function
    {
        perror(errorMessage);
        std::exit(1);
    }

    Response Server::getIncomingBlockingMessage()
    {
        sockaddr_in clientAddress;

        Protocol::Message m;
        m.getIncomingMessage(socket, clientAddress);

        return {
            .clientAddress = clientAddress,
            .msg = m};
    }

    void Server::sendReturnCode(sockaddr_in &clientAddress, Protocol::ReturnCode code, std::vector<std::string> additionalData)
    {
        std::vector<std::string> args = {
            Protocol::ReturnCodeToStrMap.at(code) // Serializes the enum to a string
        };
        // Default nothing else is sent except for the code type
        if (code == Protocol::ReturnCode::ARBITRARY && additionalData != std::vector<std::string>()) // Additional data is present
            args.insert(args.end(), additionalData.begin(), additionalData.end());                   // Insert additional vector array after return code type

        // Set up return code struct
        Protocol::Message returnCode = {
            .command = Protocol::TrackerClientCommands::ReturnCode,
            .argList = args};

        // Inform console
        std::cout << "-> Sending return code: ";
        for (const auto &i : args)
            std::cout << i << " ";
        std::cout << std::endl;

        returnCode.sendMessage(
            socket,
            clientAddress);
    }

}