#include <memory>

#include "ir/instruction.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

bool AggressiveLoopElimination::run(ir::Module *module) {
    logger::INFO("Running AggressiveLoopElimination...");
    // aggressive opt
    for (auto &func : module->get_functions_ref()) {
        for (const auto &loop : func->opt_info.loop_forest) {
            if (loop->exits.size() != 1) continue;
            auto exit = *loop->exits.begin();
            bool used_outside = false;
            for (auto &phi : util::get_phis(exit)) {
                if (phi->is_lcssa) {
                    used_outside = true;
                    break;
                }
            }

            bool side_effect = false;
            for (auto &block : loop->blocks) {
                for (auto &inst : block->get_instructions_ref()) {
                    if (std::dynamic_pointer_cast<ir::Call>(inst) || std::dynamic_pointer_cast<ir::Store>(inst)) {
                        side_effect = true;
                        break;
                    }
                }
                if (side_effect) break;
            }

            if (!used_outside && !side_effect) {
                util::redirect(loop->pre_header, loop->header, exit);
            }
        }
    }
    return false;
}

}  // namespace opt::pass
