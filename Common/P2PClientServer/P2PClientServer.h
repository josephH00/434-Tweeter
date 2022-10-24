#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <cstring>
#include <sstream>
#include <thread>
#include <mutex>
#include <utility>
#include "base64/base64.h"

#include "Common/SocketServer/SocketServer.h"
#include "Common/Protocol.h"

namespace P2PClientServer
{
    struct LogicalRingInstance
    {
        std::string handle;
        std::string ipv4Addr;
        int serverPort;
        int indexNumber;

        LogicalRingInstance(std::string _handle, std::string _ipv4Addr, int _port) : handle(_handle), ipv4Addr(_ipv4Addr), serverPort(_port) {}
        LogicalRingInstance() : handle{}, ipv4Addr{}, serverPort{} {}

        std::string serialize()
        {
            return base64_encode(base64_encode(handle) + "," + base64_encode(ipv4Addr) + "," + base64_encode(std::to_string(serverPort)));
        }

        bool deserialize(std::string serializedMsg)
        {
            serializedMsg = base64_decode(serializedMsg);
            std::vector<std::string> parsedMsg;
            std::stringstream ss(serializedMsg);

            while (ss.good())
            {
                std::string substr;
                std::getline(ss, substr, ',');
                parsedMsg.push_back(substr);
            }

            if (parsedMsg.size() < 3)
                return false;

            handle = base64_decode(parsedMsg.at(0));

            ipv4Addr = base64_decode(parsedMsg.at(1));
            serverPort = std::stoi(base64_decode(parsedMsg.at(2)));
            return true;
        }
    };

    struct LRLookupEntry
    {
        int followerIndex;
        int previousFollowerIndex;
        int nextFollowerIndex;
        std::vector<LogicalRingInstance> logicalRingTable;
    };

    class P2PClientServer
    {
    public:
        void start(int P2PServerPort);

        void createClientSocket(const char *targetIPAddress, int targetPort);

        void sendTweet(std::string msg, std::vector<std::string> serializedData);

    private:
        int socket;
        struct sockaddr_in targetAddress;
        unsigned short targetPort;
        const char *targetIP;
        void dieWithError(const char *errorMessage);

        int P2PServerPort;
        std::unique_ptr<SocketServer::Server> server;

        std::mutex trackerLogicalRingInfo;
        std::map<std::string, LRLookupEntry> handleLookupLogicalRingInfo;

        std::string tweetInProgressMessage;
        std::mutex tweetInProgressMessageMutex;

        void createLogicalRing(std::vector<std::string> serializedData); // Impliment updating
        bool startPropTweet(std::vector<std::string> messageData);            // Propigate tweet throughout ring
        bool continuePropTweet(std::vector<std::string> messageData);

        /**
         * @brief Handles the SetupLogicalRing server command and propagates the previously created ring
         *
         * @param messageData
         * @return true The logical ring has propagated back around to the original node
         * @return false It is still traversing the ring
         */
        bool propLogicalRing(std::vector<std::string> messageData);

        void serverMainLoop();

        int modulo(int x, int N); // The C implementation returns a wrong result
    };
}