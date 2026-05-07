#include <memory>
#include <optional>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

namespace {

// Return ConstantInt value if v is ConstantInt, otherwise std::nullopt
std::optional<int> as_const_int(const std::shared_ptr<ir::Value> &v) {
    if (auto cint = std::dynamic_pointer_cast<ir::ConstantInt>(v)) {
        return cint->get_val();
    }
    return std::nullopt;
}

// Collect index operands of a GEP (operands[1..])
std::vector<std::shared_ptr<ir::Value>> get_indexes(const std::shared_ptr<ir::Getelementptr> &gep) {
    std::vector<std::shared_ptr<ir::Value>> idx;
    auto &ops = gep->get_operands_ref();
    for (size_t i = 1; i < ops.size(); ++i) idx.push_back(ops[i]);
    return idx;
}

// Recompute GEP result type from base pointer and indexes
std::shared_ptr<ir::Type> compute_gep_result_type(const std::shared_ptr<ir::Value> &base,
                                                  const std::vector<std::shared_ptr<ir::Value>> &indexes) {
    auto cur_type = base->get_type();
    for (size_t i = 0; i < indexes.size(); ++i) {
        if (cur_type->is_pointer_ty()) {
            cur_type = std::dynamic_pointer_cast<ir::PointerType>(cur_type)->get_reference_type();
        } else if (cur_type->is_array_ty()) {
            cur_type = std::dynamic_pointer_cast<ir::ArrayType>(cur_type)->get_element_type();
        } else {
            __builtin_unreachable();
        }
    }
    return ir::PointerType::get(cur_type);
}

// Set target(base) of GEP and update its type accordingly
void modify_target(const std::shared_ptr<ir::Getelementptr> &gep, const std::shared_ptr<ir::Value> &new_base) {
    opt::util::substitute_operand(gep, 0, new_base);
    auto new_type = compute_gep_result_type(new_base, get_indexes(gep));
    gep->set_type(new_type);
}

// Replace all index operands with new indexes and update type
void modify_indexes(const std::shared_ptr<ir::Getelementptr> &gep,
                    const std::vector<std::shared_ptr<ir::Value>> &new_indexes) {
    auto &ops = gep->get_operands_ref();
    // detach old index operands from users list
    for (size_t i = 1; i < ops.size(); ++i) {
        ops[i]->remove_user(gep);
    }
    // keep base at ops[0]
    ops.erase(ops.begin() + 1, ops.end());
    // append new indexes
    for (const auto &idx : new_indexes) {
        gep->add_operand(idx);
        idx->add_user(gep);
    }
    // update result type
    auto new_type = compute_gep_result_type(ops[0], new_indexes);
    gep->set_type(new_type);
}

bool is_single_zero_gep(const std::shared_ptr<ir::Getelementptr> &gep) {
    auto idx = get_indexes(gep);
    if (idx.size() != 1) return false;
    auto v = as_const_int(idx[0]);
    return v.has_value() && v.value() == 0;
}

}  // namespace

bool GepFuse::run(ir::Module *module) {
    bool modified = false;
    for (auto &func : module->get_functions_ref()) {
        if (ir::Function::is_lib(func)) continue;

        // Phase 1: remove "%x = gep base, 0"
        for (auto &block : func->get_basic_blocks_ref()) {
            auto &insts = block->get_instructions_ref();
            for (auto it = insts.begin(); it != insts.end();) {
                if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(*it)) {
                    if (is_single_zero_gep(gep)) {
                        opt::util::replace_all_uses_with(gep, gep->base_ptr());
                        it = opt::util::remove_instruction(gep);
                        modified = true;
                        continue;
                    }
                }
                ++it;
            }
        }

        // Phase 2: fuse nested GEP
        for (auto &block : func->get_basic_blocks_ref()) {
            for (auto &inst : block->get_instructions_ref()) {
                auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst);
                if (!gep) continue;
                auto pre_gep = std::dynamic_pointer_cast<ir::Getelementptr>(gep->base_ptr());
                if (!pre_gep) continue;

                auto pre_idx = get_indexes(pre_gep);
                if (pre_idx.empty()) continue;
                auto now_idx = get_indexes(gep);
                if (now_idx.empty()) continue;

                auto pre_last = pre_idx.back();
                auto now_first = now_idx.front();

                auto pre_ci = as_const_int(pre_last);
                auto now_ci = as_const_int(now_first);

                bool fused = false;

                // Case 1: both constants -> add
                if (pre_ci.has_value() && now_ci.has_value()) {
                    std::vector<std::shared_ptr<ir::Value>> new_indexes;
                    new_indexes.insert(new_indexes.end(), pre_idx.begin(), pre_idx.end() - 1);
                    int sum = pre_ci.value() + now_ci.value();
                    new_indexes.push_back(std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), sum));
                    new_indexes.insert(new_indexes.end(), now_idx.begin() + 1, now_idx.end());

                    modify_target(gep, pre_gep->base_ptr());
                    modify_indexes(gep, new_indexes);
                    fused = true;
                } else if (pre_ci.has_value() || now_ci.has_value()) {
                    // Case 2: value fuse when one is zero constant
                    std::shared_ptr<ir::Value> chosen;
                    if (now_ci.has_value() && now_ci.value() == 0) {
                        chosen = pre_last;
                    } else if (pre_ci.has_value() && pre_ci.value() == 0) {
                        chosen = now_first;
                    }
                    if (chosen) {
                        std::vector<std::shared_ptr<ir::Value>> new_indexes;
                        new_indexes.insert(new_indexes.end(), pre_idx.begin(), pre_idx.end() - 1);
                        new_indexes.push_back(chosen);
                        new_indexes.insert(new_indexes.end(), now_idx.begin() + 1, now_idx.end());

                        modify_target(gep, pre_gep->base_ptr());
                        modify_indexes(gep, new_indexes);
                        fused = true;
                    }
                }

                if (fused) {
                    modified = true;
                    if (pre_gep->get_users_ref().empty()) {
                        opt::util::remove_instruction(pre_gep);
                    }
                }
            }
        }
    }
    return modified;
}

}  // namespace opt::pass
