#include <iostream>
#include <cstdlib>

#include "Server/Server.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << "[Server Port Number]" << std::endl;
        std::exit(1);
    }

    Tracker tracker(
        std::atoi(argv[1]));

    tracker.run();
}
