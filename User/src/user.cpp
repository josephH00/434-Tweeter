#include <stdio.h>      // for printf() and fprintf()
#include <sys/socket.h> // for socket(), connect(), sendto(), and recvfrom()
#include <arpa/inet.h>  // for sockaddr_in and inet_addr()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()

#include "Common/Structures.h"
#include "User/User.h"

void DieWithError( const char *errorMessage ) // External error handling function
{
    perror( errorMessage );
    exit(1);
}

int main( int argc, char *argv[] )
{

    if (argc < 3)    // Test for correct number of arguments
    {
        fprintf( stderr, "Usage: %s <Server IP address> <Echo Port>\n", argv[0] );
        exit( 1 );
    }

    User user(
        argv[1],      // First arg: Server IP addr
        atoi(argv[2]) // Second arg: Server port
    );

    std::cout << "sending registration" << std::endl;
    Protocol::Message reg = {
        .command = Protocol::TrackerClientCommands::Register,
        .argList = {"handle2", "ip", "1024", "69"}};
    reg.sendMessage(sock, echoServAddr);
    Protocol::Message rr;
    rr.getIncomingMessage(sock, echoServAddr);
    std::cout << rr.argList.at(0);

    
    std::cout << "sending follow" << std::endl;
    Protocol::Message m = {
        .command = Protocol::TrackerClientCommands::Follow,
        .argList = {"follow", "handle2", "handle"}};
    m.sendMessage(sock, echoServAddr);



    close( sock );
    exit( 0 );
}
