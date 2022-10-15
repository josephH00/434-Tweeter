#include <stdio.h>

#include "Common/Structures.h"
#include "User/User.h"

void DieWithError(const char *errorMessage) // External error handling function
{
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[])
{

    if (argc < 3) // Test for correct number of arguments
    {
        fprintf(stderr, "Usage: %s <Server IP address> <Echo Port>\n", argv[0]);
        exit(1);
    }

    User user(
        argv[1],      // First arg: Server IP addr
        atoi(argv[2]) // Second arg: Server port
    );

    user.run();
}
