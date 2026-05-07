// #pragma once

// #include "Common.h"
// #include "DFA.h"
// #include "CFA.h"

// namespace ir {
//     void runHosit(ir::FuncPtr func);
//     class ifElse {
//     private:
//         ir::InstPtr condition;
//         ir::BBPtr thenBB;
//         ir::BBPtr elseBB;

//     public:
//         ir::InstPtr getCondtion() { return condition; }
//         ir::BBPtr getThenBB() { return thenBB; }
//         ir::BBPtr getElseBB() { return elseBB; }
//     };
//     class ifElseDetector {
//     private:
//         Vector<ifElse> ifElseSet;

//     public:
//         void getIfElse(ir::FuncPtr);
//         Vector<ifElse> getSubIfElse(Ptr<ir::ifElse>);
//     };
// }
