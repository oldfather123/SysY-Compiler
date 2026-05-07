#pragma once
#include <iostream>
namespace Error
{
    inline void Error(const char *who, std::string message)
    {
        std::cerr << who << ' ' << "Error:" << message << std::endl;
        exit(114514);
    }

    inline void Error(const char *who, int line, std::string message)
    {
        std::cerr << who << ' ' << "Error at line " << line << ": " << message << std::endl;
        exit(114514);
    }
}

namespace Warning
{
    inline void Warning(const char *who, std::string message)
    {
        std::cerr << who << "Warning:" << message << std::endl;
        exit(114514);
    }
}