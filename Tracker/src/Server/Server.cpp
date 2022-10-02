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

        Protocol::Message m;
        m.getIncomingMessage(sock, clientAddress);

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
            std::cout << "Client: " << inet_ntoa(clientAddress.sin_addr) << " sent malformed [register] request" << std::endl;
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to register with is less than the required
            break;
        }

        std::string handle = message.argList.at(0);
        std::string ipv4Addr = message.argList.at(1);

        std::vector<int> ports;
        for (auto i = message.argList.begin() + 2; i != message.argList.end(); i++)
        {
            std::cout << *i << std::endl;
            ports.push_back(
                atoi(i->c_str())); // Converts string port to int and adds it to the vector
        }

        if (handle.length() > 15)
        {
            std::cout << "Client: " << inet_ntoa(clientAddress.sin_addr) << " attempted to [register] a handler greater than 15 characters (" << message.argList.at(0) << ")" << std::endl;
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        if (handleLookupTable.find(handle) != handleLookupTable.end())
        { // A duplicate handle was already registered
            std::cout << "Client: " << inet_ntoa(clientAddress.sin_addr) << " attempted to [register] a handler that already exists (" << message.argList.at(0) << ")" << std::endl;
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        handleLookupTable.insert(
            {handle, {ipv4Addr, ports}});
    }
    break;

    default:
        break;
    }
}