//
// Created by 牛奕博 on 2023/7/14.
//

#ifndef ANUC_LIVENESSINFO_H
#define ANUC_LIVENESSINFO_H
#include "type.h"
#include <set>
using namespace std;
namespace anuc {
    struct LiveOut {
        set<RegisterVar *> integerReg;
        set<RegisterVar *> floatReg;
    };
}



#endif //ANUC_LIVENESSINFO_H
