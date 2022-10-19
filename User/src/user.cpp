#include <iostream>
#include <cstdlib>

#include "Common/Structures.h"
#include "User/User.h"

int main(int argc, char *argv[])
{
    if (argc < 3) // Test for correct number of arguments
    {
        std::cerr << "Usage: " << argv[0] << " <Server IP address> <Echo Port>" << std::endl;
        std::exit(1);
    }

    User user(
        argv[1],           // First arg: Server IP addr
        std::atoi(argv[2]) // Second arg: Server port
    );

    user.run();
}
