#ifndef GEECEECEE_OPT_EXEC_INTERPRETER_HPP
#define GEECEECEE_OPT_EXEC_INTERPRETER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "ir/mod.hpp"

namespace opt::exec {

enum class FailReason {
    ExceedMemoryLimit,
    ExceedTimeLimit,
    ExceedMaxRecursiveDepth,
    MemoryError,
    DividedByZero,
    Unsupported,
    UnknownError,
};

struct IOContext {
    // Placeholders for future IO injection
};

// Runtime value representation
// Put std::monostate first so variant is default-constructible
using RuntimeValue =
    std::variant<std::monostate, ir::ConstantInt, ir::ConstantFloat, uintptr_t, std::shared_ptr<ir::Function>>;

class Interpreter final {
public:
    Interpreter(uint64_t timeBudgetNs, size_t memBudgetBytes, size_t maxRecursiveDepth, bool collectStats);

    // Execute function with constant arguments. Returns a constant on success if return type is scalar.
    std::variant<std::shared_ptr<ir::Constant>, FailReason> execute(
        ir::Module &module,
        const std::shared_ptr<ir::Function> &func,
        const std::vector<std::shared_ptr<ir::Constant>> &args,
        IOContext *io = nullptr) const;

    struct StoreEffect {
        std::shared_ptr<ir::Instruction> base_alloca;  // tracked cloned alloca in stub
        int index;                                     // linear index (elements)
        std::shared_ptr<ir::Constant> value;           // stored constant value
    };

    std::variant<std::shared_ptr<ir::Constant>, FailReason> execute_with_effects(
        ir::Module &module,
        const std::shared_ptr<ir::Function> &func,
        const std::vector<std::shared_ptr<ir::Constant>> &args,
        const std::unordered_set<std::shared_ptr<ir::Instruction>> &tracked_allocas,
        std::vector<StoreEffect> *out_effects,
        IOContext *io = nullptr) const;

    // Configure maximum allowed interpreter steps (default: 1,000,000)
    void setStepBudget(size_t stepBudget) { mStepBudget = stepBudget; }

private:
    uint64_t mTimeBudgetNs;
    size_t mMemBudgetBytes;
    size_t mMaxRecursiveDepth;
    bool mCollectStats;
    size_t mStepBudget = 1'000'000ULL;
};

}  // namespace opt::exec

#endif  // GEECEECEE_OPT_EXEC_INTERPRETER_HPP
