#include <stdio.h>      // for printf() and fprintf()
#include <sys/socket.h> // for socket() and bind()
#include <arpa/inet.h>  // for sockaddr_in and inet_ntoa()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()

#include "Common/Structures.h"


void DieWithError( const char *errorMessage ) // External error handling function
{
    perror( errorMessage );
    exit( 1 );
}

int main( int argc, char *argv[] )
{
    int sock;                        // Socket

    struct sockaddr_in echoServAddr; // Local address of server
    struct sockaddr_in echoClntAddr; // Client address

    unsigned short echoServPort;     // Server port

    if( argc != 2 )         // Test for correct number of parameters
    {
        fprintf( stderr, "Usage:  %s <UDP SERVER PORT>\n", argv[ 0 ] );
        exit( 1 );
    }

    echoServPort = atoi(argv[1]);  // First arg: local port

    // Create socket for sending/receiving datagrams
    if( ( sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
        DieWithError( "server: socket() failed" );

    // Construct local address structure */
    memset( &echoServAddr, 0, sizeof( echoServAddr ) ); // Zero out structure
    echoServAddr.sin_family = AF_INET;                  // Internet address family
    echoServAddr.sin_addr.s_addr = htonl( INADDR_ANY ); // Any incoming interface
    echoServAddr.sin_port = htons( echoServPort );      // Local port

    // Bind to the local address
    if( bind( sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0 )
        DieWithError( "server: bind() failed" );

	printf( "server: Port server is listening to is: %d\n", echoServPort );

    while(true)
    {

        uint clientAddressLen = sizeof(echoClntAddr);

        uint32_t incomingMessageSize = 0;
        auto recievedMsgSize = recvfrom(
            sock,
            &incomingMessageSize,
            Protocol::maxBufferSizeAnnouncementLength,
            0,
            (struct sockaddr *)&echoClntAddr,
            &clientAddressLen);
        incomingMessageSize = ntohl(incomingMessageSize); //Convert byte-order to original intended value
        std::cout << incomingMessageSize << std::endl;

        char *cStrIncomingMessage = new char[incomingMessageSize]; //Create enough space for the message buffer
        recievedMsgSize = recvfrom(
            sock,
            cStrIncomingMessage,
            incomingMessageSize,
            0,
            (struct sockaddr *)&echoClntAddr,
            &clientAddressLen);
        cStrIncomingMessage[ incomingMessageSize ] = '\0';

        std::string strIncomingMessage = std::string(cStrIncomingMessage);
        delete[] cStrIncomingMessage;

        Protocol::Message m;
        m.parseIncoming(strIncomingMessage);
    
    }
}
