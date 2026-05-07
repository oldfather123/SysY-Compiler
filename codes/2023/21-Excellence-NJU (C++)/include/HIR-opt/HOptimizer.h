//
// Created by q1w2e3r4 on 23-7-25.
//

#include "GlobalUnit.h"
#include "DomNode.h"
#ifndef COMPILER_HOPTIMIZER_H
#define COMPILER_HOPTIMIZER_H


class HOptimizer {
public:
    GlobalUnit * globalUnit;

    HOptimizer(GlobalUnit * gu);
    void Optimize();

    void BuildCFG();

    void Constlize();

    void debug();
};


#endif //COMPILER_HOPTIMIZER_H
