#include "P2PClientServer.h"
namespace P2PClientServer
{
    void P2PClientServer::start(int P2PServerPort)
    {
        this->P2PServerPort = P2PServerPort;

        std::thread mainLoopThread(&P2PClientServer::serverMainLoop, this); // Start thread
        mainLoopThread.detach();
    }

    void P2PClientServer::dieWithError(const char *errorMessage) // External error handling function
    {
        perror(errorMessage);
        std::exit(1);
    }

    void P2PClientServer::createClientSocket(const char *targetIPAddress, int targetPort)
    {
        this->targetIP = targetIPAddress; // First arg: server IP address (dotted decimal)
        this->targetPort = targetPort;    // Second arg: Use given port

        // Create a datagram/UDP socket
        if ((socket = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            dieWithError("client: socket() failed");

        // Construct the server address structure
        memset(&targetAddress, 0, sizeof(targetAddress));    // Zero out structure
        targetAddress.sin_family = AF_INET;                  // Use internet addr family
        targetAddress.sin_addr.s_addr = inet_addr(targetIP); // Set server's IP address
        targetAddress.sin_port = htons(targetPort);          // Set server's port
    }

    void P2PClientServer::createLogicalRing(std::vector<std::string> serializedData)
    {
        trackerLogicalRingInfo.lock();

        // Deserialize data
        std::vector<LogicalRingInstance> tmp;
        for (const auto &s : serializedData)
        {
            LogicalRingInstance i;
            i.deserialize(s);
            tmp.push_back(i);
        }

        const auto &initialUserInfo = tmp.at(0); // This is 'us', the user about to make a tweet

        // TODO: impliment updaing ring ///////////////////////
        /*if (handleLookupLogicalRingInfo.find(initialUserInfo.handle) != handleLookupLogicalRingInfo.end())
        { // If we already made a ring associated with our handle reset it
            handleLookupLogicalRingInfo.at(initialUserInfo.handle).clear();
        }*/
        handleLookupLogicalRingInfo.insert_or_assign(initialUserInfo.handle,
                                                     LRLookupEntry{.logicalRingTable = tmp});

        ///////////////////////////////////

        const auto &thisLogicalRingTable = handleLookupLogicalRingInfo.at(initialUserInfo.handle).logicalRingTable;

        // Send setup command to the first follower on the list
        const auto &firstFollower = thisLogicalRingTable.at(1);
        createClientSocket(firstFollower.ipv4Addr.c_str(), firstFollower.serverPort);

        int i = 1;
        int previousTupleIndex = (i - 1) % (thisLogicalRingTable.size() + 1);
        int nextTupleIndex = (i + 1) % (thisLogicalRingTable.size() + 1);

        std::string serializedLogicalRing;
        for (LogicalRingInstance lri : thisLogicalRingTable)
        {
            std::cout << lri.handle << " " << lri.ipv4Addr << " " << lri.serverPort << std::endl;
            serializedLogicalRing.append(lri.serialize());
            serializedLogicalRing.append(",");
        }

        trackerLogicalRingInfo.unlock();

        Protocol::Message m = {
            .command = Protocol::TrackerClientCommands::SetupLogicalRing,
            .argList = {
                serializedLogicalRing,
                std::to_string(i),
                std::to_string(previousTupleIndex),
                std::to_string(nextTupleIndex)}};

        m.sendMessage(socket, targetAddress);
    }

    int P2PClientServer::modulo(int x, int N)
    {
        return (x % N + N) % N;
    }

    bool P2PClientServer::propLogicalRing(std::vector<std::string> messageData)
    {
        trackerLogicalRingInfo.lock();

        std::string serializedLogicalRing = messageData.at(0);
        int followerIndex = std::stoi(messageData.at(1));
        int previousFollowerIndex = std::stoi(messageData.at(2));
        int nextFollowerIndex = std::stoi(messageData.at(3));

        // Deserialize data
        std::vector<LogicalRingInstance> tmp;

        std::stringstream ss(serializedLogicalRing);

        while (ss.good())
        {
            std::string substr;
            getline(ss, substr, ',');
            if (substr != "")
            {
                LogicalRingInstance i;
                i.deserialize(substr); // The serialized LR info
                tmp.push_back(i);
            }
        }

        const auto &initialUserInfo = tmp.at(0); // The user originating the logical ring

        if (nextFollowerIndex == 1) // The ring has wrapped around to the original handle, exit
            return true;

        /*
                // TODO: implement updating ring ///////////////////////
                if (handleLookupLogicalRingInfo.find(initialUserInfo.handle) != handleLookupLogicalRingInfo.end())
                { // If we already made a ring associated with our handle reset it
                    handleLookupLogicalRingInfo.at(initialUserInfo.handle).clear();
                }
                */
        handleLookupLogicalRingInfo.insert_or_assign(initialUserInfo.handle, LRLookupEntry{
                                                                                 .logicalRingTable = tmp});

        ///////////////////////////////////

        const auto &thisLogicalRingTable = handleLookupLogicalRingInfo.at(initialUserInfo.handle).logicalRingTable;
        // Send setup command to the next follower on the list
        const auto &nextFollower = thisLogicalRingTable.at(nextFollowerIndex);
        createClientSocket(nextFollower.ipv4Addr.c_str(), nextFollower.serverPort);

        int i = nextFollowerIndex;                                          // Send information about the next one
        int previousTupleIndex = modulo(i - 1, thisLogicalRingTable.size() - 1); // Remove +1 because size() is 1-indexed & not 0-indexed
        int nextTupleIndex = modulo(i + 1, thisLogicalRingTable.size());


        std::string updatedSerializedLogicalRing;
        for (LogicalRingInstance lri : thisLogicalRingTable)
        {
            updatedSerializedLogicalRing.append(lri.serialize());
            updatedSerializedLogicalRing.append(",");
        }

        trackerLogicalRingInfo.unlock();

        Protocol::Message m = {
            .command = Protocol::TrackerClientCommands::SetupLogicalRing,
            .argList = {
                updatedSerializedLogicalRing,
                std::to_string(i),
                std::to_string(previousTupleIndex),
                std::to_string(nextTupleIndex)}};

        m.sendMessage(socket, targetAddress);
        close(socket);

        return false;
    }

    bool P2PClientServer::propTweet(std::vector<std::string> messageData)
    {
        std::string serializedLogicalRing = messageData.at(0);
        std::vector<LogicalRingInstance> tmp;

        std::stringstream ss(serializedLogicalRing);

        while (ss.good())
        {
            std::string substr;
            getline(ss, substr, ',');
            if (substr != "")
            {
                LogicalRingInstance i;
                i.deserialize(substr); // The serialized LR info
                tmp.push_back(i);
            }
        }

        const auto &initialUserInfo = tmp.at(0); // The user originating the logical ring

        const auto &thisLogicalRing = handleLookupLogicalRingInfo.at(initialUserInfo.handle);
        const auto &nextFollower = thisLogicalRing.logicalRingTable.at(thisLogicalRing.nextFollowerIndex);
        createClientSocket(nextFollower.ipv4Addr.c_str(), nextFollower.serverPort);

        Protocol::Message m = {
            .command = Protocol::TrackerClientCommands::P2PSendTweet,
            .argList = {

            }};
    }

    void P2PClientServer::sendTweet(std::string msg, std::vector<std::string> serializedData)
    {
        tweetInProgressMessageMutex.lock();
        tweetInProgressMessage = msg;
        tweetInProgressMessageMutex.unlock();

        createLogicalRing(serializedData);
    }

    void P2PClientServer::serverMainLoop()
    {

        server = std::make_unique<SocketServer::Server>(P2PServerPort, true); // Set blocking mode to false
        while (true)
        {
            SocketServer::Response r = server->getIncomingBlockingMessage();
            if (r.msg.command == Protocol::TrackerClientCommands::SetupLogicalRing)
            {
                std::cout << "Got logical ring setup cmd" << std::endl;
                bool hasReturnedToBeginning = propLogicalRing(r.msg.argList);

                if (hasReturnedToBeginning)
                    propTweet(r.msg.argList);
            }
        }
    }
}