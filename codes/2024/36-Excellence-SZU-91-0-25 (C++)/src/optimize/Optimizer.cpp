#include "../optimize/Optimizer.h"

Optimizer optimizer;

void Optimizer::Optimize()
{
    int loop_time=7;
    for(int i=0;i<loop_time;i++)
    {
        ConstReplace();
        ConstantFold();
        ArithmeticSimplify();
        LocalConstantPropagate();
        //BabyVariableEliminate(); 
    }
    BasicBlockEliminate();
}
