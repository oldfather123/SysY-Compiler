#pragma once

#include "../utility/System.h"

class Optimizer
{
private:
    void ConstReplace();
    void ConstantFold();
    void LocalConstantPropagate();
    void ArithmeticSimplify();
    void BabyVariableEliminate();
    void BasicBlockEliminate();

public:
    void Optimize();
};

extern Optimizer optimizer;