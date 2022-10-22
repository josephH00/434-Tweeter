#include <iostream>
#include <cstdlib>

#include "Tracker/Tracker.h"
#include "Common/TestingAutomation.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << "[Server Port Number]" << std::endl;
        std::exit(1);
    }

#ifdef __DEBUG
    TestingAutomation::feedSTDINWithFileInput(TestingAutomation::trackerInputFile);
#endif

    Tracker tracker(
        std::atoi(argv[1]));

    tracker.run();
}
