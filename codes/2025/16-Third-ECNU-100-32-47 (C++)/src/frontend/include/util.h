#ifndef AAAC_UTIL_H
#define AAAC_UTIL_H
#include <map>
#include <string>
#include "Parser.h"
namespace aaac {
namespace frontend {
    std::map<int, std::string> tokname = {
        #define TOKNAME(tok) { Parser::tok, #tok },
        #include "TOKNAME.inc"
        #undef TOKNAME
    };
}
}

#endif