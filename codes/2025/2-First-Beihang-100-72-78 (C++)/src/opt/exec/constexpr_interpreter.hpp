#ifndef GEECEECEE_OPT_EXEC_CONSTEXPR_INTERPRETER_HPP
#define GEECEECEE_OPT_EXEC_CONSTEXPR_INTERPRETER_HPP

#include <optional>
#include <unordered_map>
#include <vector>

#include "ir/mod.hpp"

namespace opt {

using ConstEnv = std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Constant>>;

class ConstexprInterpreter {
public:
    // Evaluate a function with a given constant environment for its inputs.
    // Constraints:
    // - Single basic block only
    // - Sequential execution, no memory/control/call
    // - Supported ops only
    // Returns a constant on success, std::nullopt on failure
    static std::optional<std::shared_ptr<ir::Constant>> eval_function_with_env(
        const std::shared_ptr<ir::Function> &func, const ConstEnv &env);

    // Build environment from constant arguments and evaluate the callee
    static std::optional<std::shared_ptr<ir::Constant>> eval_callee_on_constants(
        const std::shared_ptr<ir::Function> &callee, const std::vector<std::shared_ptr<ir::Constant>> &const_args);
};

}  // namespace opt

#endif  // GEECEECEE_OPT_EXEC_CONSTEXPR_INTERPRETER_HPP
