#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

typedef unsigned int uint;

namespace Protocol {

    const size_t maxBufferSizeAnnouncementLength = sizeof(uint32_t);

    enum class TrackerClientCommands
    {
        Register,
        Query,
        Follow,
        Drop,
        Tweet,
        EndTweet,
        Exit
    };
    const std::map<TrackerClientCommands, std::string> TrackerToStrMap = {
        {TrackerClientCommands::Register, "register"},
        {TrackerClientCommands::Query, "query"},
        {TrackerClientCommands::Follow, "follow"},
        {TrackerClientCommands::Drop, "drop"},
        {TrackerClientCommands::Tweet, "tweet"},
        {TrackerClientCommands::EndTweet, "end-tweet"},
        {TrackerClientCommands::Exit, "exit"},
    };

    struct Message
    {
        TrackerClientCommands command;
        std::vector<std::string> argList;

        void parseIncoming(std::string inStr)
        {
            std::stringstream ss(inStr);

            std::string msgCommand;
            getline(ss, msgCommand, ',');

            for (const auto &[key, value] : TrackerToStrMap) //Reverse search the message's string to a tracker command enum value
                if (msgCommand == value)
                    command = key;

            //Parse the command's arguments, spliting on commas
            while(ss.good()) {
                std::string substr;
                getline(ss, substr, ',');
                argList.push_back(substr);
            }
        }

        void sendMessage(int socketFD, sockaddr_in& address) {
            //Partitions data into the following format: "commandString,Arg1,Arg2,Arg3,Arg#,"
            //It would be nice to just send over structs so you can structured data, but you'd need to use some sort of protocol buffer, but this was easier and cost less time. 
            std::string formattedData = TrackerToStrMap.at(command) + ",";
            for(const auto& i : argList) {
                formattedData.append(i);
                formattedData.append(",");
            }
            formattedData[ formattedData.length() ] = '\0';
            std::cout << formattedData << std::endl;
            

            uint32_t strLen = htonl(formattedData.length());
            if(sendto(
                socketFD, 
                &strLen, 
                sizeof(strLen), 
                0,
                (sockaddr*) &address,
                sizeof(address)
                ) != sizeof(strLen)) {
                std::cout << "Message <<" << formattedData << ">> was not able to send size beforehand\n";
                exit(1);
            }

            const char *cStrFormattedData = formattedData.c_str();
            if(sendto(
                socketFD,
                cStrFormattedData,
                strlen(cStrFormattedData),
                0,
                (sockaddr*) &address,
                sizeof(address)
                ) != strlen(cStrFormattedData)) {
                std::cout << "Message <<" << formattedData << ">> was not able to send expected number of bytes\n";
                exit(1);
            };
        }
    };
}