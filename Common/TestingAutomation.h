#pragma once
#include <string>
#include <fstream>
#include <iostream>

namespace TestingAutomation
{
    const std::string trackerInputFile = "../TrackerSTDIN.txt";
    const std::string clientInputFile = "../ClientSTDIN.txt";
    void feedSTDINWithFileInput(std::string inputFileName)
    {
        std::ifstream inputFile(inputFileName);

        std::stringstream IFBuffer;
        IFBuffer << inputFile.rdbuf();

        std::istringstream inputOSS(IFBuffer.str());
        std::cin.rdbuf(inputOSS.rdbuf()); // Replace cin input with input file stream
    }
}