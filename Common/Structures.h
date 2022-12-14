#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>

#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>

#include "base64/base64.h"

typedef unsigned int uint;

namespace Protocol
{

    const size_t maxBufferSizeAnnouncementLength = sizeof(uint32_t);

    enum class TrackerClientCommands
    {
        Register,
        QueryHandles,
        Follow,
        Drop,
        Tweet,
        EndTweet,
        Exit,
        ReturnCode, // This is used for sending back information to the client about a previous operation (eg registering for the service)
        __NotSet    // Internal value used for converting back strings to enum values
    };
    const std::map<TrackerClientCommands, std::string> TrackerToStrMap = {
        {TrackerClientCommands::Register, "register"},
        {TrackerClientCommands::QueryHandles, "query-handles"},
        {TrackerClientCommands::Follow, "follow"},
        {TrackerClientCommands::Drop, "drop"},
        {TrackerClientCommands::Tweet, "tweet"},
        {TrackerClientCommands::EndTweet, "end-tweet"},
        {TrackerClientCommands::Exit, "exit"},
        {TrackerClientCommands::ReturnCode, "return-code"}};

    enum class ReturnCode
    {
        SUCCESS,
        FAILURE,
        ARBITRARY // If the data returned is neither a failure or success operation (eg. returning the number of handles a client has)
    };
    const std::map<ReturnCode, std::string> ReturnCodeToStrMap = {
        {ReturnCode::SUCCESS, "SUCCESS"},
        {ReturnCode::FAILURE, "FAILURE"},
        {ReturnCode::ARBITRARY, "ARBITRARY"}};

    struct Message
    {
        TrackerClientCommands command = TrackerClientCommands::__NotSet;
        std::vector<std::string> argList;

        void getIncomingMessage(int socketFD, sockaddr_in &clientAddress)
        {
            /* Get incoming message size */
            uint clientAddressLen = sizeof(clientAddress);
            uint32_t incomingMessageSize = 0;

            auto recMsgLen = recvfrom(
                socketFD,
                &incomingMessageSize,
                Protocol::maxBufferSizeAnnouncementLength,
                0,
                (struct sockaddr *)&clientAddress,
                &clientAddressLen);

            auto msgSize = ntohl(incomingMessageSize); // Convert byte-order to original intended value
                                                       // The message could have a variable length of arguements we get how long it should be before it arrives

            /* Get the just sent message */
            char *cStrIncomingMessage = new char[msgSize]; // Create enough space for the message buffer

            auto recMsgSize = recvfrom(
                socketFD,
                cStrIncomingMessage,
                msgSize,
                0,
                (struct sockaddr *)&clientAddress,
                &clientAddressLen);

            cStrIncomingMessage[msgSize] = '\0';

            std::string strIncomingMessage = std::string(cStrIncomingMessage); // Can't delete before it's used
            delete[] cStrIncomingMessage;

            parseIncoming(strIncomingMessage, ',', true);
        }

        bool parseIncoming(std::string inStr, char delim, bool isInputBase64Encoded = false)
        {

            if (command != TrackerClientCommands::__NotSet)
                throw std::runtime_error("Attempted to overwrite an already populated message!"); // Set command value to prevent accidental use of non overwriten data

            std::stringstream ss(inStr);

            std::string msgCommand;
            getline(ss, msgCommand, delim);

            for (const auto &[key, value] : TrackerToStrMap) // Reverse search the message's string to a tracker command enum value
                if (msgCommand == value)
                    command = key;

            if (command == TrackerClientCommands::__NotSet)
                return false; // A command was not resolved from the message string

            // Parse the command's arguments, spliting on commas
            while (ss.good())
            {
                std::string substr;
                getline(ss, substr, delim);

                if (isInputBase64Encoded)
                    substr = base64_decode(substr); // Decode b64 argument

                if (substr != "") // Avoid extra whitespace
                    argList.push_back(substr);
            }

            return true;
        }

        void sendMessage(int socketFD, sockaddr_in &address)
        {
            // Partitions data into the following format: "commandString,Arg1,Arg2,Arg3,Arg#,"
            // It would be nice to just send over structs so you can structured data, but you'd need to use some sort of protocol buffer, but this was easier and cost less time.
            std::string formattedData = TrackerToStrMap.at(command) + ",";
            for (const auto &i : argList)
            {
                formattedData.append(
                    base64_encode(i) // Encode argument in b64 to prevent escaping
                );
                formattedData.append(",");
            }
            formattedData[formattedData.length()] = '\0';

            uint32_t strLen = htonl(formattedData.length()); // Send length of message beforehand
            if (sendto(
                    socketFD,
                    &strLen,
                    sizeof(strLen),
                    0,
                    (sockaddr *)&address,
                    sizeof(address)) != sizeof(strLen))
            {
                std::cout << "Message <<" << formattedData << ">> was not able to send size beforehand\n";
                exit(1);
            }

            const char *cStrFormattedData = formattedData.c_str(); // Send 'formatted' message
            if (sendto(
                    socketFD,
                    cStrFormattedData,
                    std::strlen(cStrFormattedData),
                    0,
                    (sockaddr *)&address,
                    sizeof(address)) != strlen(cStrFormattedData))
            {
                std::cout << "Message <<" << formattedData << ">> was not able to send expected number of bytes\n";
                exit(1);
            };
        }
    };
}