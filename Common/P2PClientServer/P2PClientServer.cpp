#include "P2PClientServer.h"
namespace P2PClientServer
{
    void P2PClientServer::start(int P2PServerPort, const char *trackerIP, int trackerPort)
    {
        this->P2PServerPort = P2PServerPort;
        this->trackerIP = trackerIP;
        this->trackerPort = trackerPort;

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
                                                     LRLookupEntry{
                                                        .followerIndex = 0,
                                                        .previousFollowerIndex = (int)tmp.size() - 1,
                                                        .nextFollowerIndex = 1,

                                                        .logicalRingTable = tmp});

        ///////////////////////////////////
        auto &thisLogicalRing = handleLookupLogicalRingInfo.at(initialUserInfo.handle);
        

        const auto &thisLogicalRingTable = handleLookupLogicalRingInfo.at(initialUserInfo.handle).logicalRingTable;

        // Send setup command to the first follower on the list
        const auto &firstFollower = thisLogicalRingTable.at(1);

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

        std::cout << "i " << i << std::endl;
        std::cout << "pti " << previousTupleIndex << std::endl;
        std::cout << "nti " << nextTupleIndex << std::endl;

        std::cout << "first follower ifno " << firstFollower.handle << " " << firstFollower.ipv4Addr << " " << firstFollower.serverPort << std::endl;

        trackerLogicalRingInfo.unlock();

        Protocol::Message m = {
            .command = Protocol::TrackerClientCommands::SetupLogicalRing,
            .argList = {
                serializedLogicalRing,
                std::to_string(i),
                std::to_string(previousTupleIndex),
                std::to_string(nextTupleIndex)}};
        createClientSocket(firstFollower.ipv4Addr.c_str(), firstFollower.serverPort);
        m.sendMessage(socket, targetAddress);
        close(socket);
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

        std::cout << "fi " << followerIndex << std::endl;
        std::cout << "pfi " << previousFollowerIndex << std::endl;
        std::cout << "nfi " << nextFollowerIndex << std::endl;

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
                std::cout << substr << std::endl;
                i.deserialize(substr); // The serialized LR info
                tmp.push_back(i);
            }
        }
        std::cout << "ss" << std::endl;
        const auto &initialUserInfo = tmp.at(0); // The user originating the logical ring
        if (nextFollowerIndex == 1)
        { // The ring has wrapped around to the original handle, exit
            trackerLogicalRingInfo.unlock();
            return true;
        }

        /*
                // TODO: implement updating ring ///////////////////////
                if (handleLookupLogicalRingInfo.find(initialUserInfo.handle) != handleLookupLogicalRingInfo.end())
                { // If we already made a ring associated with our handle reset it
                    handleLookupLogicalRingInfo.at(initialUserInfo.handle).clear();
                }
                */
        handleLookupLogicalRingInfo.insert_or_assign(
            initialUserInfo.handle,
            LRLookupEntry{
                .followerIndex = followerIndex,
                .previousFollowerIndex = previousFollowerIndex,
                .nextFollowerIndex = nextFollowerIndex,
                .logicalRingTable = tmp});

        ///////////////////////////////////

        const auto &thisLogicalRingTable = handleLookupLogicalRingInfo.at(initialUserInfo.handle).logicalRingTable;
        // Send setup command to the next follower on the list
        const auto &nextFollower = thisLogicalRingTable.at(nextFollowerIndex);

        int i = nextFollowerIndex;                                               // Send information about the next one
        int previousTupleIndex = modulo(i - 1, thisLogicalRingTable.size() - 1); // Remove +1 because size() is 1-indexed & not 0-indexed
        int nextTupleIndex = modulo(i + 1, thisLogicalRingTable.size());

        std::cout << "i " << i << std::endl;
        std::cout << "pti " << previousTupleIndex << std::endl;
        std::cout << "nti " << nextTupleIndex << std::endl;
        std::cout << "next follower ifno " << nextFollower.handle << " " << nextFollower.ipv4Addr << " " << nextFollower.serverPort << std::endl;

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
        std::cout << "cs" << std::endl;
        createClientSocket(nextFollower.ipv4Addr.c_str(), nextFollower.serverPort);
        m.sendMessage(socket, targetAddress);
        close(socket);

        return false;
    }

    bool P2PClientServer::startPropTweet(std::vector<std::string> messageData)
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

        Protocol::Message m = {
            .command = Protocol::TrackerClientCommands::P2PSendTweet,
            .argList = {
                initialUserInfo.handle,
                std::to_string(thisLogicalRing.followerIndex),
                tweetInProgressMessage}};

        createClientSocket(nextFollower.ipv4Addr.c_str(), nextFollower.serverPort);
        m.sendMessage(socket, targetAddress);
        close(socket); // Close to reuse netf

        return false;
    }

    bool P2PClientServer::continuePropTweet(std::vector<std::string> messageData)
    {

        const auto &originalHandle = messageData.at(0); // The user originating the tweet
        std::string tweetMessage = messageData.at(2);

        const auto &thisLogicalRing = handleLookupLogicalRingInfo.at(originalHandle);

        if (thisLogicalRing.followerIndex == 0)
            return true; // Came back around

        const auto &nextFollower = thisLogicalRing.logicalRingTable.at(thisLogicalRing.nextFollowerIndex);

        Protocol::Message m = {
            .command = Protocol::TrackerClientCommands::P2PSendTweet,
            .argList = {
                originalHandle,                                // Original handle
                std::to_string(thisLogicalRing.followerIndex), // The index of who is sending the tweet this time around
                tweetMessage                                   // The message itself
            }};

        createClientSocket(nextFollower.ipv4Addr.c_str(), nextFollower.serverPort);
        m.sendMessage(socket, targetAddress);
        close(socket); // Close to reuse netf

        return false; // Didn't completely wrap around
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
                std::cout << "[P2P] Got Logical Ring Setup command" << std::endl;
                bool hasReturnedToBeginning = propLogicalRing(r.msg.argList);

                if (hasReturnedToBeginning)
                    startPropTweet(r.msg.argList);
            }

            if (r.msg.command == Protocol::TrackerClientCommands::P2PSendTweet)
            {
                std::string originatorHandle = r.msg.argList.at(0);
                int lastFollowerIndex = std::stoi(r.msg.argList.at(1)); // Keeps track of the last user to forward the tweet
                std::string lastFollowerHandle = handleLookupLogicalRingInfo.at(originatorHandle).logicalRingTable.at(lastFollowerIndex).handle;
                std::cout << "[Got P2P Tweet]" << std::endl;

                std::cout << "From: " << lastFollowerHandle << std::endl;
                std::cout << "Msg: " << r.msg.argList.at(2) << std::endl;

                // If the tweet has returned back to the original sender let the Tracker know
                bool hasTweetReturnedToBeginning = continuePropTweet(r.msg.argList);
                if (hasTweetReturnedToBeginning)
                {
                    Protocol::Message m = {
                        .command = Protocol::TrackerClientCommands::EndTweet,
                        .argList = {r.msg.argList.at(0)} // The original handle sending the tweet
                    };

                    createClientSocket(trackerIP, trackerPort);
                    m.sendMessage(socket, targetAddress);
                    close(socket);

                    continue;
                }
            }
        }
    }
}