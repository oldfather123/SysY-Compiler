#include "LCSSA.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include "Instruction.hpp"

#include "LoopDetection.hpp"
#include "Value.hpp"
#include <algorithm>
#include <cassert>

using namespace std;

void LCSSA::run(Function *func, AnalysisPassManager &APM) {
    if (func->is_declaration()) {
        return;
    }
    auto &&loops = APM.getResult<LoopDetection>(func)->get_loops();
    for (auto &loop : loops) {
        for (auto &exit : loop->get_exits()) {
            auto &block = *exit.first;
            // for (auto &inst1 : block.get_instructions()) {
            //     auto inst = &inst1;
            //     if (auto *user = inst->get_user()) {
            //         auto &phi = func->create_phi_node(I->get_type());
            //         phi.add_incoming(user, exit.second);
            //         I->replace_all_uses_with(&phi);
            //     }
            // }
        }
    }
}