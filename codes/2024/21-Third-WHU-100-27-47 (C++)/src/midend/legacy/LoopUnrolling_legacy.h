// #pragma once

// #include "Common.h"
// #include "DFA.h"
// #include "CFA.h"
// #include "LoopDet.h"

// namespace ir {
//     static int UNROLLT = 6;
//     struct UnrollLoop {
//         Vector<ir::BBPtr> bbs, bodies;
//         ir::BBPtr header{nullptr}, exit{nullptr}, preheader{nullptr};
//         Ptr<ir::Value> ind_var{nullptr}, initial{nullptr}, bound{nullptr}, step{nullptr};
//         ir::InstPtr cond{nullptr};
//     };
//     // the entry function of loop unrolling
//     void runLoopUnroll(ir::FuncPtr func);
//     bool doUnroll(ir::UnrollLoop loop);
//     void handleUnroll(Ptr<ir::Loop> loop);
//     void unrollFunction(ir::UnrollLoop loop);
//     // this function decides if we need to unroll a loop
//     bool getUnrollInfo(Ptr<Loop> loop, ir::UnrollLoop &ret);
// }
