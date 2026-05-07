#include <cstdint>
#include <memory>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/module.hpp"
#include "ir/support.hpp"
#include "ir/value.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

static ir::opt_support::PointerBaseInfo *pointer_base_info;

static std::unordered_map<std::shared_ptr<ir::Value>, std::vector<std::shared_ptr<ir::Instruction>>> graph;
static std::unordered_map<std::shared_ptr<ir::Value>, int> degree;

static void analyze_func(std::shared_ptr<ir::Function> func, ir::Module *module);
static std::optional<std::shared_ptr<ir::Value>> trace_inter_procedural_arg(std::shared_ptr<ir::Function> func,
                                                                            std::shared_ptr<ir::Argument> arg,
                                                                            int idx,
                                                                            int depth);
static std::optional<std::shared_ptr<ir::Value>> trace_inter_procedural_inst(std::shared_ptr<ir::Function> func,
                                                                             std::shared_ptr<ir::Instruction> inst,
                                                                             int depth);
static std::optional<std::shared_ptr<ir::Value>> trace_inter_procedural_val(std::shared_ptr<ir::Function> func,
                                                                            std::shared_ptr<ir::Value> val,
                                                                            int depth);
static void adjust();
static std::shared_ptr<ir::Value> adjust_dfs(std::shared_ptr<ir::Value> root);

bool PointerBaseAnalyzation::run(ir::Module *module) {
    logger::INFO("Running PointerBaseAnalyzation");
    pointer_base_info = &module->opt_info.pointer_base_info;
    pointer_base_info->clear();
    module->for_each_func([module](auto &func) { analyze_func(func, module); });
    adjust();
    return false;
}

static void analyze_func(std::shared_ptr<ir::Function> func, ir::Module *module) {
    graph.clear();
    degree.clear();

    for (auto &block : func->get_basic_blocks_ref()) {
        for (auto &inst : block->get_instructions_ref()) {
            if (!inst->get_type()->is_pointer_ty()) continue;
            if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst)) {
                graph[gep->base_ptr()].push_back(gep);
                degree[gep] = 1;
            } else if (auto bitcast = std::dynamic_pointer_cast<ir::Bitcast>(inst)) {
                graph[bitcast->val()].push_back(bitcast);
                degree[bitcast] = 1;
            } else if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst)) {
                degree[phi] = static_cast<int>(phi->get_operands_ref().size()) / 2;
                for (size_t i = 0; i < phi->get_operands_ref().size(); i += 2) {
                    graph[phi->get_operands_ref()[i]].push_back(phi);
                }
            }
        }
    }
    std::queue<std::shared_ptr<ir::Instruction>> q;
    const auto set_storage = [&](std::shared_ptr<ir::Value> dst, std::shared_ptr<ir::Value> src) {
        assert(!pointer_base_info->contains(dst) || pointer_base_info->query(dst).value() == src);
        pointer_base_info->put(dst, src);
        for (const auto &child : graph[dst]) {
            assert(degree.at(child) > 0);
            if (--degree.at(child) == 0) q.push(child);
        }
    };

    for (const auto &global_var : module->get_global_variables_ref()) {
        set_storage(global_var, global_var);
    }
    bool directly_use_global = !q.empty();

    int arg_idx = 0, arg_cnt = 0;
    std::shared_ptr<ir::Value> unique_ptr_arg;
    for (const auto &arg : func->get_arguments_ref()) {
        if (arg->get_type()->is_pointer_ty()) {
            auto base = trace_inter_procedural_arg(func, arg, arg_idx, 0);
            if (base.has_value()) set_storage(arg, base.value());
            unique_ptr_arg = arg;
            ++arg_cnt;
        }
        ++arg_idx;
    }
    if (!directly_use_global && arg_cnt == 1) set_storage(unique_ptr_arg, unique_ptr_arg);

    for (auto &block : func->get_basic_blocks_ref()) {
        for (auto &inst : block->get_instructions_ref()) {
            if (inst->get_type()->is_pointer_ty() && inst->is_type(ir::Instruction::InstructionType::ALLOCA))
                set_storage(inst, inst);
        }
    }

    while (!q.empty()) {
        auto inst = q.front();
        q.pop();

        if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst)) {
            set_storage(gep, gep->base_ptr());
        } else if (auto bitcast = std::dynamic_pointer_cast<ir::Bitcast>(inst)) {
            set_storage(bitcast, bitcast->val());
        } else if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst)) {
            std::shared_ptr<ir::Value> src;
            bool same = true;
            for (size_t i = 0; i < phi->get_operands_ref().size(); i += 2) {
                auto entry = phi->get_operands_ref()[i];
                if (entry == inst) continue;
                auto from = pointer_base_info->query(entry).value();
                if (src == nullptr || from == src)
                    src = from;
                else {
                    same = false;
                    break;
                }
            }
            if (same && src) set_storage(phi, src);
        }
    }
}

static std::optional<std::shared_ptr<ir::Value>> trace_inter_procedural_val(std::shared_ptr<ir::Function> func,
                                                                            std::shared_ptr<ir::Value> val,
                                                                            int depth) {
    if (std::dynamic_pointer_cast<ir::GlobalVariable>(val) || std::dynamic_pointer_cast<ir::Alloca>(val)) return val;
    if (auto inst = std::dynamic_pointer_cast<ir::Instruction>(val))
        return trace_inter_procedural_inst(func, inst, depth);
    if (std::dynamic_pointer_cast<ir::Argument>(val)) {
        int idx = 0;
        for (auto &arg : func->get_arguments_ref()) {
            if (arg == val) return trace_inter_procedural_arg(func, arg, idx, depth);
            ++idx;
        }
    }
    return std::nullopt;
}

static std::optional<std::shared_ptr<ir::Value>> trace_inter_procedural_inst(std::shared_ptr<ir::Function> func,
                                                                             std::shared_ptr<ir::Instruction> inst,
                                                                             int depth) {
    if (depth > 7) return std::nullopt;
    if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst)) {
        return trace_inter_procedural_val(func, gep->base_ptr(), depth + 1);
    } else if (auto bitcast = std::dynamic_pointer_cast<ir::Bitcast>(inst)) {
        return trace_inter_procedural_val(func, bitcast->val(), depth + 1);
    } else if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst)) {
        std::shared_ptr<ir::Value> common_src;
        for (size_t i = 0; i < phi->get_operands_ref().size(); i += 2) {
            auto val = phi->get_operands_ref()[i];
            if (val == inst) continue;
            auto src = trace_inter_procedural_val(func, val, depth + 1);
            if (!src.has_value()) return std::nullopt;
            if (common_src == nullptr)
                common_src = src.value();
            else if (common_src != src.value())
                return std::nullopt;
        }
        if (!common_src) return std::nullopt;
        return common_src;
    }
    return std::nullopt;
}

static std::optional<std::shared_ptr<ir::Value>> trace_inter_procedural_arg(std::shared_ptr<ir::Function> func,
                                                                            std::shared_ptr<ir::Argument> arg,
                                                                            int idx,
                                                                            int depth) {
    if (depth > 7) return std::nullopt;
    std::shared_ptr<ir::Value> common_src = nullptr;
    for (auto &weak_user : func->get_users_ref()) {
        auto call = std::dynamic_pointer_cast<ir::Call>(weak_user.lock());
        assert(call);
        auto arg_val = call->get_operands_ref()[idx + 1];
        if (arg_val == arg) continue;
        auto src = trace_inter_procedural_val(func, arg, depth + 1);
        if (!src.has_value()) return std::nullopt;
        if (!common_src)
            common_src = src.value();
        else if (common_src != src.value())
            return std::nullopt;
    }
    if (!common_src) return std::nullopt;
    return common_src;
}

static void adjust() {
    for (auto pair : pointer_base_info->map()) {
        adjust_dfs(pair.first);
    }
}

static std::shared_ptr<ir::Value> adjust_dfs(std::shared_ptr<ir::Value> root) {
    auto val = pointer_base_info->query(root);
    if (val.value() == root) {
        return root;
    }
    auto new_val = adjust_dfs(val.value());
    pointer_base_info->put(root, new_val);
    return new_val;
}

}  // namespace opt::pass
