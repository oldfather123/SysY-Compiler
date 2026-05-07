#include "ir/module.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
bool handle_func(std::shared_ptr<ir::Function> func);

bool RedundantRetEliminate::run(ir::Module *module) {
    // probably useless
    bool modified = false;
    module->for_each_func([&modified](const auto &function) { modified |= handle_func(function); });
    return modified;
}

bool handle_func(std::shared_ptr<ir::Function> func) {
    if (func->is_main() || func->get_return_type() == ir::VoidType::get()) {
        return false;
    }
    bool has_user = false;
    for (auto caller : func->get_users_ref()) {
        if (!caller.lock()->get_users_ref().empty()) {
            has_user = true;
            break;
        }
    }
    if (has_user) {
        return false;
    }
    for (auto &block : func->get_basic_blocks_ref()) {
        auto &terminator = block->get_instructions_ref().back();
        if (terminator->get_ins_type() == ir::Instruction::InstructionType::RET) {
            util::remove_instruction(terminator);
            block->add_instructions({ir::Ret::create(block)});
        }
    }
    func->set_type(ir::FunctionType::get(ir::VoidType::get(), func->get_param_types()));
    for (auto caller : func->get_users_ref()) {
        caller.lock()->set_type(ir::VoidType::get());
    }
    return true;
}

}  // namespace opt::pass
