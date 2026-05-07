#include <cassert>
#include <memory>
#include <optional>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/mod.hpp"
#include "ir/value.hpp"
#include "opt/exec/interpreter.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace {

bool is_supported_type(const std::shared_ptr<ir::Type> &ty) {
    return ty->is_integer_ty() || ty->is_float_ty() || ty->is_void_ty();
}

bool is_constexpr_func_rec(const std::shared_ptr<ir::Function> &func,
                           std::unordered_set<std::shared_ptr<ir::Function>> & /*visiting*/) {
    if (!func) return false;
    if (ir::Function::is_lib(func)) return false;  // 排除库 IO

    // 类型约束：返回类型需为整型/浮点/void；参数不得为 void
    if (!is_supported_type(func->get_return_type())) return false;
    for (auto &pt : func->get_param_types()) {
        if (!is_supported_type(pt)) return false;
        if (pt->is_void_ty()) return false;
    }

    // 属性约束：依据分析属性决定
    const auto &info = func->opt_info;
    if (info.has_side_effect) return false;
    if (info.has_global_memory_write) return false;
    // if (info.has_global_memory_read) return false;  // 可选

    return true;
}

bool is_constexpr_func(const std::shared_ptr<ir::Function> &func) {
    std::unordered_set<std::shared_ptr<ir::Function>> visiting;
    return is_constexpr_func_rec(func, visiting);
}

std::optional<std::shared_ptr<ir::Constant>> eval_function(ir::Module *module,
                                                           const std::shared_ptr<ir::Function> &func,
                                                           const std::vector<std::shared_ptr<ir::Constant>> &args) {
    // Increase time budget (10s) and recursion depth for heavy constexpr evaluation
    opt::exec::Interpreter interpreter{1000'000'000'00ULL, 1 << 20, 8192, false};
    interpreter.setStepBudget(100'000'000ULL);  // allow up to 50M steps

    auto res = interpreter.execute(*module, func, args, nullptr);
    if (std::holds_alternative<std::shared_ptr<ir::Constant>>(res)) return std::get<std::shared_ptr<ir::Constant>>(res);
    return std::nullopt;
}

}  // namespace

namespace opt::pass {

bool ConstexprFunctionEvaluation::run(ir::Module *module) {
    bool modified = false;

    // A) function-level: constexpr函数（无参）整体常量折叠
    for (auto &func : module->get_functions_ref()) {
        if (!is_constexpr_func(func)) continue;
        if (!func->get_arguments_ref().empty()) continue;  // 仅处理无参函数

        auto res = eval_function(module, func, {});
        if (res) {
            // clear blocks and rebuild minimal body: entry + ret const
            // use util::gen_block_name() to align naming with other passes
            auto new_block = std::make_shared<ir::BasicBlock>(func, gen_block_name());
            func->get_basic_blocks_ref().clear();
            func->add_basic_block(func->get_basic_blocks_ref().end(), new_block);

            // Make a placeholder value to insert ret constant: we need a Value typed constant
            auto ret_const = *res;
            auto ret = ir::Ret::create(new_block, ret_const, "");
            new_block->add_instruction(new_block->get_instructions_ref().end(), ret);
            modified = true;
        }
    }

    // B) 调用点级：被调constexpr且实参全为常量 → 替换为常量
    module->for_each_func([&](auto f) {
        f->for_each_block([&](auto b) {
            // copy list of instructions to avoid iterator invalidation on removal
            std::vector<std::shared_ptr<ir::Instruction>> insts(b->get_instructions_ref().begin(),
                                                                b->get_instructions_ref().end());
            for (auto &ins : insts) {
                auto call = std::dynamic_pointer_cast<ir::Call>(ins);
                if (!call) continue;
                auto callee = call->get_function();
                if (!is_constexpr_func(callee)) continue;
                auto args = call->args();
                std::vector<std::shared_ptr<ir::Constant>> const_args;
                const_args.reserve(args.size());
                bool all_const = true;
                for (auto &a : args) {
                    auto c = std::dynamic_pointer_cast<ir::Constant>(a);
                    if (!c) {
                        all_const = false;
                        break;
                    }
                    const_args.push_back(c);
                }
                if (!all_const) continue;

                auto res = eval_function(module, callee, const_args);
                if (!res) continue;

                // Replace uses and remove the call (detach operands to avoid dangling users)
                opt::util::replace_all_uses_with(call, *res);
                opt::util::remove_instruction(call);
                modified = true;
            }
        });
    });
    if (modified) { /* pipeline will run DCE/SCCP afterwards */
    }
    return modified;
}

}  // namespace opt::pass
