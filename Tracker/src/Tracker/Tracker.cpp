#include "Tracker.h"

Tracker::Tracker(int serverPort)
{
    socketServer = std::make_unique<SocketServer::Server>(serverPort, true);
}

void Tracker::run()
{
    while (true)
    {
        auto r = socketServer->getIncomingBlockingMessage();

        std::cout << "Received command [" << Protocol::TrackerToStrMap.at(r.msg.command) << "] from client @" << inet_ntoa(r.clientAddress.sin_addr) << ":\n\tArgs: ";
        for (const auto &i : r.msg.argList)
            std::cout << i << " ";
        std::cout << std::endl;

        parseClientMessage(r); // Process the newly received information

        std::cout << std::endl
                  << std::endl; // Spread out messages on the console
    }
}

void Tracker::parseClientMessage(SocketServer::Response response)
{
    switch (response.msg.command)
    {
    case Protocol::TrackerClientCommands::Register:
    {
        // Register arguments list: [@handle] [IPv4 Address] [Ports used by User]
        // Returns: _SUCCESS_ if new unique handle, _FAILURE_ otherwise

        if (response.msg.argList.size() < 3)
        {
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " sent malformed [register] request" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to register with is less than the required
            break;
        }

        std::string handle = response.msg.argList.at(0);
        std::string ipv4Addr = response.msg.argList.at(1);

        std::vector<int> ports;
        for (auto i = response.msg.argList.begin() + 2; i != response.msg.argList.end(); i++)
        {
            ports.push_back(
                atoi(i->c_str())); // Converts string port to int and adds it to the vector
        }

        if (handle.length() > 15)
        {
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " attempted to [register] a handler greater than 15 characters (" << response.msg.argList.at(0) << ")" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        if (handleLookupTable.find(handle) != handleLookupTable.end())
        { // A duplicate handle was already registered
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " attempted to [register] a handler that already exists (" << response.msg.argList.at(0) << ")" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        // If a client with the same ipv4 address arg registers it faults, todo: Fix
        handleLookupTable.insert(
            {handle, {.ipv4Addr = ipv4Addr, .P2PServerPorts = ports, .followers = {}, .tweetInProgress = false}}); // All else no errors, register client

        socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::SUCCESS);
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

        for (const auto &i : handleLookupTable)
            registeredHandles.push_back(i.first); // Append all the handles registered in vector

        socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::ARBITRARY, registeredHandles);
    }
    break;

    case Protocol::TrackerClientCommands::Follow:
    {
        // Follow arg list: [original handle] [handle to follow]
        // Return: SUCCESS if possible, FAILURE if not registered

        if (response.msg.argList.size() < 2)
        {
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " sent malformed [follow] request" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to follow with is less than the required
            break;
        }

        std::string handle = response.msg.argList.at(0);
        std::string handleToFollow = response.msg.argList.at(1);

        if (handleLookupTable.find(handle) == handleLookupTable.end()) // Handle was not registered (iterator reaches the end of the map)
        {
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " attempted to [follow] with a handle that was not already registered (" << handle << ")" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        if (handleLookupTable.find(handleToFollow) == handleLookupTable.end()) // Handle to follow was not registered
        {
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " attempted to [follow] a handle that was not already registered (" << handleToFollow << ")" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
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
        std::cout << "Successfully added follower (@" << handleToFollow << ") to @" << handle << std::endl;
        std::cout << "Current followers of @" << handle << ": ";
        for (const auto i : handleLookupTable.at(handle).followers)
        {
            std::cout << i << ' ';
        }
        std::cout << std::endl;

        socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::SUCCESS);
    }
    break;

    case Protocol::TrackerClientCommands::Drop:
    {
        // Removes a handle from the follower list

        // Follow arg list: [original handle] [handle to drop]
        // Return: SUCCESS if possible, FAILURE if not registered

        if (response.msg.argList.size() < 2)
        {
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " sent malformed [drop] request" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to follow with is less than the required
            break;
        }

        std::string handle = response.msg.argList.at(0);
        std::string handleToDrop = response.msg.argList.at(1);

        if (handleLookupTable.find(handle) == handleLookupTable.end()) // Handle was not registered (iterator reaches the end of the map)
        {
            std::cout << "Client: @" << handle << " (" << inet_ntoa(response.clientAddress.sin_addr) << ") "
                      << "attempted to [drop] with a handle that was not already registered (" << handle << ")" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        if (handleLookupTable.find(handleToDrop) == handleLookupTable.end()) // Handle to follow was not registered
        {
            std::cout << "Client: @" << handle << " (" << inet_ntoa(response.clientAddress.sin_addr) << ") "
                      << "attempted to [drop] a handle that was not already registered (" << handleToDrop << ")" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        auto &handleFollowers = handleLookupTable[handle].followers; // Directly get the string vector

        if (std::find(handleFollowers.begin(), handleFollowers.end(), handleToDrop) == handleFollowers.end())
        { // Check if handle to drop is actually being followed by the client
            std::cout << "Client: @" << handle << " (" << inet_ntoa(response.clientAddress.sin_addr) << ") "
                      << "attempted to [drop] a handle that it was not already following (" << handleToDrop << ")" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        // Remove element matching the follower to be dropped's handle from the table, this will stay in alphabetically order when it's removed since no other elements will be modified
        handleFollowers.erase(
            std::remove(
                handleFollowers.begin(),
                handleFollowers.end(),
                handleToDrop),
            handleFollowers.end());

        // Inform console
        std::cout << "Successfully dropped follower (@" << handleToDrop << ") of @" << handle << std::endl;
        std::cout << "Current followers of @" << handle << ": ";
        for (const auto i : handleLookupTable.at(handle).followers)
        {
            std::cout << i << ' ';
        }
        std::cout << std::endl;

        socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::SUCCESS);
    }
    break;

    case Protocol::TrackerClientCommands::Exit:
    {
        // Removes a client's handle from the registry
        // Args: [handle to be removed]

        if (response.msg.argList.size() < 1)
        {
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " sent malformed [exit] request" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to follow with is less than the required
            break;
        }

        std::string handleToDrop = response.msg.argList.at(0);

        if (handleLookupTable.find(handleToDrop) == handleLookupTable.end()) // Handle was not registered (iterator reaches the end of the map)
        {
            std::cout << "Client: @" << handleToDrop << " (" << inet_ntoa(response.clientAddress.sin_addr) << ") "
                      << "attempted to [exit] with a handle that was not already registered (" << handleToDrop << ")" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        /*
         * TODO: Check if a tweet is in progress & delay before removing
         */

        // Iterates through all of the registered handles and removes the handle-to-be-removed from it's follower list
        for (auto &registeredHandle : handleLookupTable)
        {
            auto &handleFollowers = (registeredHandle.second).followers; // Directly get the string vector from the K-V pair

            if (std::find(handleFollowers.begin(), handleFollowers.end(), handleToDrop) == handleFollowers.end()) // Skip if the currently iterated handle doesn't contain the to-be-removed-handle. Technically when passing in a value that doesn't exist to the remove-erase idiom it produces undefined behavior for the vector<T>::erase, during testing it just silently failed but just to make sure
                continue;

            // Remove element matching the follower to be dropped's handle from the table
            handleFollowers.erase(
                std::remove(
                    handleFollowers.begin(),
                    handleFollowers.end(),
                    handleToDrop),
                handleFollowers.end());
        }

        /*
         * TODO: Remove state of logical ring of exiting handle
         *      Should receive a 'exit-complete' command from the client
         */

        handleLookupTable.erase( // Remove exiting-handle from the list of registered clients
            handleLookupTable.find(handleToDrop));

        // Inform console
        std::cout << "Successfully processed handle (@" << handleToDrop << ") exit" << std::endl;
        std::cout << "Remaining registered handles: ";
        for (const auto &i : handleLookupTable)
        {
            std::cout << i.first << ' ';
        }
        std::cout << std::endl;

        socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::SUCCESS);
    }
    break;

    case Protocol::TrackerClientCommands::Tweet:
    {
        // Returns all the users that follow the handle

        // Arg list: [handle] [message]
        // Return: ARBITRARY if possible, FAILURE if not not registered or empty tweet
        //      "b64[userHandle],b64[userIP],b64[userP2PServerPort]","b64[follower1Handle],b64[follower2IP],b64[follower2P2PServerPort]", ...

        if (response.msg.argList.size() < 2)
        {
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " sent malformed [tweet] request" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to follow with is less than the required
            break;
        }

        std::string handle = response.msg.argList.at(0);
        std::string tweetMessage = response.msg.argList.at(1);

        if (handleLookupTable.find(handle) == handleLookupTable.end()) // Handle was not registered (iterator reaches the end of the map)
        {
            std::cout << "Client: @" << handle << " (" << inet_ntoa(response.clientAddress.sin_addr) << ") "
                      << "attempted to [tweet] with a handle that was not already registered (" << handle << ")" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        if (tweetMessage.length() == 0 || tweetMessage.length() > 140) // Tweet was empty or over max characters
        {
            std::cout << "Client: @" << handle << " (" << inet_ntoa(response.clientAddress.sin_addr) << ") "
                      << "attempted to [tweet] with an invalid message length of (" << tweetMessage.length() << ")" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        auto &userEntry = handleLookupTable[handle]; // Directly get the string vector

        if (userEntry.tweetInProgress)
        {
            std::cout << "Client: @" << handle << " (" << inet_ntoa(response.clientAddress.sin_addr) << ") "
                      << "attempted to [tweet] with another tweet already in progress" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE);
            break;
        }

        std::vector<std::string> followersList  = {};

        // Insert the calling-handle's information to send back to for constructing the logical ring
        followersList.push_back(
            P2PClientServer::LogicalRingInstance(
                handle,
                userEntry.ipv4Addr,
                userEntry.P2PServerPorts.at(0)) // There should only be one port that's registered, due to time constraints this will be reverted to one port at a later time
                .serialize());

        // Find all users who are following the user-who-is-about-to-send-a-tweet
        for (const auto &[uHandle, uEntry] : handleLookupTable)
        {
            if (uHandle == handle) // Skip if it's the user who is sending the tweet
                continue;

            bool userIsFollowingHandle = (std::find(uEntry.followers.begin(), uEntry.followers.end(), handle) != uEntry.followers.end()); // Find if another other user is following the handle-about-to-tweet
            if (!userIsFollowingHandle)                                                                                                   // If the user doesn't follow them, skip
                continue;
            std::cout << uHandle << " " << handleLookupTable[uHandle].ipv4Addr << " " << handleLookupTable[uHandle].P2PServerPorts.at(0) << std::endl;
            followersList.push_back(
                P2PClientServer::LogicalRingInstance(
                    uHandle,                                        // The user's handle
                    handleLookupTable[uHandle].ipv4Addr,            // User's IP address
                    handleLookupTable[uHandle].P2PServerPorts.at(0) // User's P2P server port
                    )
                    .serialize());
        }

        userEntry.tweetInProgress = true;

        socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::ARBITRARY, followersList);
    }
    break;

    case Protocol::TrackerClientCommands::EndTweet:
    {
        if (response.msg.argList.size() < 1)
        {
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " sent malformed [end-tweet] request" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to follow with is less than the required
            break;
        }

        std::string handle = response.msg.argList.at(0);
        if (handleLookupTable.at(handle).tweetInProgress == false)
        {
            std::cout << "Client: " << inet_ntoa(response.clientAddress.sin_addr) << " attempted to end a tweet that did not start" << std::endl;
            socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::FAILURE); // The amount of arguments the client is trying to follow with is less than the required
            break;
        }

        handleLookupTable.at(handle).tweetInProgress = false;

        socketServer->sendReturnCode(response.clientAddress, Protocol::ReturnCode::SUCCESS);
    }
    break;

    default:
        std::cout << "Command not implemented!" << std::endl;
        break;
    }
}