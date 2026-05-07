#pragma once

#include <iostream>
#include <map>

namespace utils
{
    struct CompilerArgs
    {
        std::string inputFileName;
        std::string IRFileName;
        std::string ASMFileName;
        int optLevel = 0;
        bool genIR = false;
        bool genASM = false;

        void verify();
    };

    struct ArgvParser
    {
        ArgvParser() = default;

        CompilerArgs run(int argc, const char *argv[]);
    };
}