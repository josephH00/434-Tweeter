#include "Server.h"

Server::Server(int serverPort)
{

    this->serverPort = serverPort;

    // Create socket for sending/receiving datagrams
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        dieWithError("server: socket() failed");

    // Construct local address structure
    memset(&serverAddress, 0, sizeof(serverAddress));  // Zero out structure
    serverAddress.sin_family = AF_INET;                // Internet address family
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
    serverAddress.sin_port = htons(serverPort);        // Local port

    // Bind to the local address
    if (bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        dieWithError("server: bind() failed");

    std::cout << "Server is listening on port " << serverPort << std::endl;
}

void Server::run()
{
    while (true)
    {
        sockaddr_in clientAddress;

        std::string serializedMessage = getIncomingMessage(clientAddress);

        Protocol::Message m;
        m.parseIncoming(serializedMessage);

        std::cout << "Recieved command [" << Protocol::TrackerToStrMap.at(m.command) << "] from client @" << inet_ntoa(clientAddress.sin_addr) << ":\n\tArgs: ";
        for (const auto &i : m.argList)
            std::cout << i << " ";
        std::cout << std::endl;

        parseClientMessage(m, clientAddress); // Process the newly recievied information
    }
}

void Server::dieWithError(const char *errorMessage) // External error handling function
{
    perror(errorMessage);
    std::exit(1);
}

uint32_t Server::getIncomingMessageSize(sockaddr_in &clientAddress)
{
    uint clientAddressLen = sizeof(clientAddress);
    uint32_t incomingMessageSize = 0;

    auto recievedMsgSize = recvfrom(
        sock,
        &incomingMessageSize,
        Protocol::maxBufferSizeAnnouncementLength,
        0,
        (struct sockaddr *)&clientAddress,
        &clientAddressLen);

    return ntohl(incomingMessageSize); // Convert byte-order to original intended value
}

std::string Server::getIncomingMessage(sockaddr_in &clientAddress)
{
    uint clientAddressLen = sizeof(clientAddress);

    auto msgSize = getIncomingMessageSize(clientAddress); // The message could have a variable length of arguements we get how long it should be before it arrives
    char *cStrIncomingMessage = new char[msgSize];        // Create enough space for the message buffer

    auto recievedMsgSize = recvfrom(
        sock,
        cStrIncomingMessage,
        msgSize,
        0,
        (struct sockaddr *)&clientAddress,
        &clientAddressLen);

    cStrIncomingMessage[msgSize] = '\0';

    std::string strIncomingMessage = std::string(cStrIncomingMessage); // Can't delete before it's used
    delete[] cStrIncomingMessage;

    return strIncomingMessage;
}

void Server::sendReturnCode(sockaddr_in &clientAddress, Protocol::ReturnCode code, std::string additionalData = std::string())
{

    std::vector<std::string> args = {
        Protocol::ReturnCodeToStrMap.at(code)                                       // Serializes the enum to a string
    };                                                                              // Default nothing else is sent except for the code type
    if (code == Protocol::ReturnCode::ARBITRARY && additionalData != std::string()) // Additional data is present
        args.push_back(additionalData);                                             // Add it to the list

    // Set up return code struct
    Protocol::Message returnCode = {
        .command = Protocol::TrackerClientCommands::ReturnCode,
        .argList = args};

    returnCode.sendMessage(
        sock,
        clientAddress);
}

void Server::parseClientMessage(Protocol::Message message, sockaddr_in &clientAddress)
{
    switch (message.command)
    {
    case Protocol::TrackerClientCommands::Register:
    {
        // Register arguments list: [@handle] [IPv4 Address] [Ports used by User]
        // Returns: _SUCCESS_ if new unique handle, _FAILURE_ otherwise

        if (message.argList.size() < 3)
        {
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to register with is less than the required
            std::cout << "Client";
        }
    }
    break;

    default:
        break;
    }
}