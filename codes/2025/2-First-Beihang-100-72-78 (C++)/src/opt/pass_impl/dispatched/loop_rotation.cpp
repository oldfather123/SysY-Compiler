#include <algorithm>
#include <memory>

#include "ir/support.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/pass.hpp"
namespace opt::pass {
static void rotate(std::shared_ptr<ir::opt_support::LoopInfo> loop);

bool LoopRotation::run(ir::Module *module) {
    logger::INFO("Running LoopRotation...");
    for (auto &func : module->get_functions_ref()) {
        for (const auto &loop : func->opt_info.loop_forest) {
            rotate(loop);
        }
    }
    return false;
}

static void rotate(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    if (loop->header == loop->latch()) return;
    // if (std::any_of(loop->header->get_instructions_ref().begin(),
    //                 loop->header->get_instructions_ref().end(),
    //                 [](const auto &inst) { return inst->is_type(ir::Instruction::InstructionType::CALL); }))
    //     return;
    auto header = loop->header;
    if (loop->exitings.size() != 1) {
        logger::WARN("[LoopRotation::rotate] loop has ", loop->exitings.size(), " exitings, skip rotate");
        return;
    }
}

}  // namespace opt::pass
