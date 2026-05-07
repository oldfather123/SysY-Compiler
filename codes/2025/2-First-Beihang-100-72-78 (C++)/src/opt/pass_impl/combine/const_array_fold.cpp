#include <cstddef>
#include <memory>
#include <unordered_set>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/mod.hpp"
#include "ir/module.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {
static bool modified = false;

// global arrays that have never been stored or called
static std::unordered_set<std::shared_ptr<ir::Value>> pure_global_arrs;

static void collect_pure_global_arrs(ir::Module *module);
static void fold(ir::Module *module);
static std::shared_ptr<ir::Value> search_ptr_root(std::shared_ptr<ir::Value> ptr);
static std::shared_ptr<ir::Value> seek_concrete_index(std::shared_ptr<ir::GlobalVariable> global_var,
                                                      std::vector<std::shared_ptr<ir::Value>> indexes);

bool ConstArrayFold::run(ir::Module *module) {
    logger::INFO("Running ConstArrayFold...");
    modified = false;
    collect_pure_global_arrs(module);
    modified = !pure_global_arrs.empty();
    fold(module);
    return modified;
}

static void collect_pure_global_arrs(ir::Module *module) {
    pure_global_arrs.clear();

    module->for_each_global_variable([&](auto &global_var) {
        if (global_var->is_arr()) pure_global_arrs.insert(global_var);
    });
    module->for_each_func([&](auto &func) {
        func->for_each_block([&](auto &block) {
            block->for_each_instruction([&](auto &inst) {
                if (auto store = std::dynamic_pointer_cast<ir::Store>(inst)) {
                    auto root = search_ptr_root(store->addr());
                    if (auto global_var = std::dynamic_pointer_cast<ir::GlobalVariable>(root)) {
                        pure_global_arrs.erase(global_var);
                    }
                }
                if (auto call = std::dynamic_pointer_cast<ir::Call>(inst)) {
                    for (auto &arg : call->args()) {
                        auto root = search_ptr_root(arg);
                        if (auto global_var = std::dynamic_pointer_cast<ir::GlobalVariable>(root)) {
                            pure_global_arrs.erase(global_var);
                        }
                    }
                }
            });
        });
    });
}

static void fold(ir::Module *module) {
    module->for_each_func([&](auto &func) {
        func->for_each_block([&](auto &block) {
            auto instructions = block->get_instructions();
            for (auto inst : instructions) {
                if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst)) {
                    auto ptr = gep->base_ptr();
                    if (!contains(pure_global_arrs, ptr)) continue;
                    if (auto global_var = std::dynamic_pointer_cast<ir::GlobalVariable>(ptr)) {
                        auto indexes = gep->get_indexes();
                        bool all_const_flag = std::all_of(indexes.begin(), indexes.end(), [](const auto &index) {
                            return std::dynamic_pointer_cast<ir::ConstantInt>(index) != nullptr;
                        });
                        if (!all_const_flag || std::dynamic_pointer_cast<ir::PointerType>(ptr->get_type())
                                                   ->get_reference_type()
                                                   ->is_pointer_ty())
                            continue;
                        auto concrete_index = seek_concrete_index(global_var, indexes);
                        for (auto &weak_users : gep->get_users_ref()) {
                            if (auto load = std::dynamic_pointer_cast<ir::Load>(weak_users.lock())) {
                                util::replace_all_uses_with(load, concrete_index);
                                util::remove_instruction(load);
                            }
                        }
                    }
                }
            }
        });
    });
}

static std::shared_ptr<ir::Value> search_ptr_root(std::shared_ptr<ir::Value> ptr) {
    for (auto root = ptr;;) {
        if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(root))
            root = gep->base_ptr();
        else if (auto load = std::dynamic_pointer_cast<ir::Load>(root))
            root = load->addr();
        else
            return root;
    }
}

static std::shared_ptr<ir::Value> seek_concrete_index(std::shared_ptr<ir::GlobalVariable> global_var,
                                                      std::vector<std::shared_ptr<ir::Value>> indexes) {
    auto init_val = global_var->get_init_value();
    if (auto zero_initializer = std::dynamic_pointer_cast<ir::ZeroInitializer>(init_val)) {
        auto target_type = zero_initializer->get_type();
        while (target_type->is_array_ty()) {
            target_type = std::dynamic_pointer_cast<ir::ArrayType>(target_type)->get_element_type();
        }
        return ir::get_zero(target_type);
    }

    for (size_t i = 1; i < indexes.size(); i++) {
        init_val = std::dynamic_pointer_cast<ir::ConstantArray>(init_val)
                       ->get_vals()[std::dynamic_pointer_cast<ir::ConstantInt>(indexes[i])->get_val()];
    }
    return init_val;
}

}  // namespace opt::pass
