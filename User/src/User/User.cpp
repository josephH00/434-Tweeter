#include "User.h"

User::User(const char *serverIPAddress, int serverPort)
{

    this->serverIP = serverIPAddress; // First arg: server IP address (dotted decimal)
    this->serverPort = serverPort;    // Second arg: Use given port

    // Create a datagram/UDP socket
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        dieWithError("client: socket() failed");

    // Construct the server address structure
    memset(&serverAddress, 0, sizeof(serverAddress));    // Zero out structure
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
    std::cout << "Commands arguments are delimited with a SPACE (' ')" << std::endl;
    while (true)
    {
        std::cout << "Command> " << std::flush;

        std::string userInput;
        std::getline(std ::cin, userInput); // Get user input from STDIN

        Protocol::Message m;
        if (!m.parseIncoming(userInput, ' ')) // If there is an error parsing the user's input, let them know
        {
            std::cout << "<<Malformed command, try again>>" << std::endl;
            continue; // Skip loop
        }

        // Let user know what's being sent
        std::cout << "Sending command [" << Protocol::TrackerToStrMap.at(m.command) << "] to server [" << serverIP << "] with arguments: ";
        for (const auto &i : m.argList)
            std::cout << i << " ";
        std::cout << std::endl;

        m.sendMessage(sock, serverAddress);

        // Block for server response
        Protocol::Message response;
        response.getIncomingMessage(sock, serverAddress);
        if (response.command != Protocol::TrackerClientCommands::ReturnCode)
        {
            std::cout << "Server sent command [" << Protocol::TrackerToStrMap.at(response.command) << "] instead of expected response code" << std::endl;
            continue; // Go back user input
        }

        std::string strResponseCode = response.argList.at(0); // The response code eg SUCCESS, FAILURE is in the first arg

        std::string formattedResponse;
        if (strResponseCode == Protocol::ReturnCodeToStrMap.at(Protocol::ReturnCode::ARBITRARY)) // If the server sends back additional data (instead of a success or failure), just display that
            for (auto i = response.argList.begin() + 1; i != response.argList.end(); i++)
            {
                formattedResponse.append(*i);
                formattedResponse.append(" ");
            }
        else
            formattedResponse = strResponseCode;

        std::cout << "-> Got response from server: " << formattedResponse << std::endl
                  << std::endl;

        // Start the P2P server after successfully registering
        if (m.command == Protocol::TrackerClientCommands::Register && strResponseCode == Protocol::ReturnCodeToStrMap.at(Protocol::ReturnCode::SUCCESS))
        {
            P2PCS.start(std::stoi(m.argList.at(2))); // Third argument is the port#
        }

        // Create/modify ring
        if (m.command == Protocol::TrackerClientCommands::Tweet && strResponseCode == Protocol::ReturnCodeToStrMap.at(Protocol::ReturnCode::ARBITRARY))
        {
            std::vector<std::string> trackerInfo = response.argList;
            trackerInfo.erase(trackerInfo.begin()); // Remove 'Arbitrary' tag from data

            P2PCS.sendTweet(m.argList.at(1),trackerInfo);
        }
    }
}