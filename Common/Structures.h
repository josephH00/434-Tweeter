#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>

#include <map>
#include <vector>
#include <string>

typedef unsigned int uint;
typedef uint SerializedTrackerClientCommand;

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
    std::string header = "434Tweeter";
    TrackerClientCommands command;
    std::vector<const char *> argList;

    void sendMessage(int socketFD) {
        //Partitions data into the following format: "header,commandString,Arg1,Arg2,Arg3,Arg#,"
        std::string formattedData = header + "," + TrackerToStrMap.at(command) + ",";
        for(const auto& i : argList) {
            formattedData.append(i);
            formattedData.append(",");
        }

        uint32_t strLen = htonl(formattedData.length());
        send(socketFD, &strLen, sizeof(strLen), 0);

        const char *cStrFormattedData = formattedData.c_str();
        send(socketFD, cStrFormattedData, strLen, 0);
    }
};

