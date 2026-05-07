#include <iostream>
#include <memory>
#include <unordered_set>

#include "ir/instruction.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {
static bool modified = false;

static std::unordered_set<std::shared_ptr<ir::GlobalVariable>> delete_list;

static void try_localize(const std::shared_ptr<ir::GlobalVariable> &global_var);

bool GlobalScalarLocalization::run(ir::Module *module) {
    logger::INFO("Running GlobalScalarLocalization...");
    modified = false;
    delete_list.clear();
    module->for_each_global_variable([](auto &global_var) { try_localize(global_var); });
    modified = !delete_list.empty();
    for (const auto &global_var : delete_list) {
        module->erase(global_var);
    }
    return modified;
}

static void try_localize(const std::shared_ptr<ir::GlobalVariable> &global_var) {
    // only localize scalars
    auto ref_type = std::dynamic_pointer_cast<ir::PointerType>(global_var->get_type())->get_reference_type();
    if (ref_type->is_array_ty()) return;
    bool stored = false;
    std::unordered_set<std::shared_ptr<ir::Function>> user_funcs;
    std::unordered_set<std::shared_ptr<ir::Instruction>> user_insts;
    for (auto &weak_user : global_var->get_users_ref()) {
        auto user = weak_user.lock();
        if (auto user_inst = std::dynamic_pointer_cast<ir::Instruction>(user)) {
            user_insts.insert(user_inst);
            auto function = user_inst->get_parent_block().lock()->get_parent_func().lock();
            user_funcs.insert(function);
            if (std::dynamic_pointer_cast<ir::Store>(user_inst)) stored = true;
        }
    }

    // if never stored, simplify it as constant
    if (!stored) {
        delete_list.insert(global_var);
        for (const auto &user_inst : user_insts) {
            if (auto load = std::dynamic_pointer_cast<ir::Load>(user_inst)) {
                auto init_val = global_var->get_init_value();
                std::shared_ptr<ir::Value> replacement;
                if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(init_val))
                    replacement = std::make_shared<ir::ConstantInt>(const_int);
                else if (auto const_float = std::dynamic_pointer_cast<ir::ConstantFloat>(init_val))
                    replacement = std::make_shared<ir::ConstantFloat>(const_float);
                else
                    __builtin_unreachable();
                util::substitute(load, replacement);
            }
        }
        return;
    }

    // if stored, localize it when it's only used in main function
    // TODO(Xingkun): is it possible to localize it when it's only used in main function? (call analysis)
    if (user_funcs.size() == 1 && (*user_funcs.begin())->is_main()) {
        delete_list.insert(global_var);
        auto main_func = *user_funcs.begin();
        auto entry_block = main_func->entry_block();
        auto alloca = ir::Alloca::create(entry_block, ref_type, gen_local_var_name());
        auto store =
            ref_type->is_integer_ty()
                ? ir::Store::create(entry_block,
                                    std::make_shared<ir::ConstantInt>(
                                        std::dynamic_pointer_cast<ir::ConstantInt>(global_var->get_init_value())),
                                    alloca,
                                    gen_local_var_name())
                : ir::Store::create(entry_block,
                                    std::make_shared<ir::ConstantFloat>(
                                        std::dynamic_pointer_cast<ir::ConstantFloat>(global_var->get_init_value())),
                                    alloca,
                                    gen_local_var_name());
        entry_block->add_instruction(entry_block->get_instructions_ref().begin(), store);
        entry_block->add_instruction(entry_block->get_instructions_ref().begin(), alloca);
        util::replace_all_uses_with(global_var, alloca);
    }
}
}  // namespace opt::pass
