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

        std::cout << std::endl
                  << std::endl; // Spread out messages on the console
    }
}

void Server::dieWithError(const char *errorMessage) // External error handling function
{
    perror(errorMessage);
    std::exit(1);
}

void Server::sendReturnCode(sockaddr_in &clientAddress, Protocol::ReturnCode code, std::vector<std::string> additionalData = std::vector<std::string>())
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

        // If a client with the same ipv4 address arg registers it faults, todo: Fix
        handleLookupTable.insert(
            {handle, {ipv4Addr, ports, {}}}); // All else no errors, register client

        sendReturnCode(clientAddress, Protocol::ReturnCode::SUCCESS);
    }
    break;

    case Protocol::TrackerClientCommands::QueryHandles:
    {
        // Querys handles registered with tracker
        // Query Handles argument list: None
        // Query Handles return list: [# of handles] [handle 1] [handle 2] [handle 3] [handle #]
        //   Alt return list for 0 handles: [0]

        std::vector<std::string> registeredHandles;
        registeredHandles.push_back(std::to_string(
            handleLookupTable.size() // First arg is how many handles are registered
            ));

        sendReturnCode(clientAddress, Protocol::ReturnCode::ARBITRARY, registeredHandles);
    }
    break;

    case Protocol::TrackerClientCommands::Follow:
    {
        // Follow arg list: [original handle] [handle to follow]
        // Return: SUCCESS if possible, FAILURE if not registered

        if (message.argList.size() < 2)
        {
            std::cout << "Client: " << inet_ntoa(clientAddress.sin_addr) << " sent malformed [follow] request" << std::endl;
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to follow with is less than the required
            break;
        }

        std::string handle = message.argList.at(0);
        std::string handleToFollow = message.argList.at(1);

        if (handleLookupTable.find(handle) == handleLookupTable.end()) // Handle was not registered (iterator reaches the end of the map)
        {
            std::cout << "Client: " << inet_ntoa(clientAddress.sin_addr) << " attempted to [follow] with a handle that was not already registered (" << handle << ")" << std::endl;
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        if (handleLookupTable.find(handleToFollow) == handleLookupTable.end()) // Handle to follow was not registered
        {
            std::cout << "Client: " << inet_ntoa(clientAddress.sin_addr) << " attempted to [follow] a handle that was not already registered (" << handleToFollow << ")" << std::endl;
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        auto &handleUserInformation = handleLookupTable[handle]; // Directly get the string vector
        auto it = std::upper_bound(std::begin(handleUserInformation.followers), std::end(handleUserInformation.followers), handleToFollow,
                                   [](auto const &a, auto const &b)
                                   {
                                       return a.compare(b) < 0; // Find the position where inserting would be in alphabetical order
                                   });
        handleUserInformation.followers.insert(it, handleToFollow); // Insert into the sorted vector

        // Inform console
        std::cout << "Sucessfully added follower (@" << handleToFollow << ") to @" << handle << std::endl;
        std::cout << "Current followers of @" << handle << ": ";
        for (const auto i : handleLookupTable.at(handle).followers)
        {
            std::cout << i << ' ';
        }
        std::cout << std::endl;

        sendReturnCode(clientAddress, Protocol::ReturnCode::SUCCESS);
    }
    break;

    case Protocol::TrackerClientCommands::Drop: {
        //Removes a handle from the follower list

        // Follow arg list: [original handle] [handle to drop]
        // Return: SUCCESS if possible, FAILURE if not registered

        if (message.argList.size() < 2)
        {
            std::cout << "Client: " << inet_ntoa(clientAddress.sin_addr) << " sent malformed [drop] request" << std::endl;
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to follow with is less than the required
            break;
        }

        std::string handle = message.argList.at(0);
        std::string handleToDrop = message.argList.at(1);

        if (handleLookupTable.find(handle) == handleLookupTable.end()) // Handle was not registered (iterator reaches the end of the map)
        {
            std::cout << "Client: @" << handle << " (" <<inet_ntoa(clientAddress.sin_addr) << ") " << "attempted to [drop] with a handle that was not already registered (" << handle << ")" << std::endl;
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        if (handleLookupTable.find(handleToDrop) == handleLookupTable.end()) // Handle to follow was not registered
        {
            std::cout << "Client: @" << handle << " (" <<inet_ntoa(clientAddress.sin_addr) << ") " << "attempted to [drop] a handle that was not already registered (" << handleToDrop << ")" << std::endl;
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        auto &handleFollowers = handleLookupTable[handle].followers; // Directly get the string vector
        
        if(std::find(handleFollowers.begin(), handleFollowers.end(), handleToDrop) == handleFollowers.end()) { //Check if handle to drop is actually being followed by the client
            std::cout << "Client: @" << handle << " (" <<inet_ntoa(clientAddress.sin_addr) << ") " << "attempted to [drop] a handle that it was not already following (" << handleToDrop << ")" << std::endl;
            sendReturnCode(clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        //Remove element matching the follower to be dropped's handle from the table, this will stay in alphabetically order when it's removed since no other elements will be modified
        handleFollowers.erase(
            std::remove(
                handleFollowers.begin(), 
                handleFollowers.end(), 
                handleToDrop),
             handleFollowers.end());

        // Inform console
        std::cout << "Sucessfully dropped follower (@" << handleToDrop << ") of @" << handle << std::endl;
        std::cout << "Current followers of @" << handle << ": ";
        for (const auto i : handleLookupTable.at(handle).followers)
        {
            std::cout << i << ' ';
        }
        std::cout << std::endl;

        sendReturnCode(clientAddress, Protocol::ReturnCode::SUCCESS);
    }
    break;

    case Protocol::TrackerClientCommands::Exit: {

    }
    break;

    default:
        std::cout << "Command not implimented!" << std::endl;
        break;
    }
}