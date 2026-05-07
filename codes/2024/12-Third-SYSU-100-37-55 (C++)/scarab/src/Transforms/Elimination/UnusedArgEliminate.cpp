#include "UnusedArgEliminate.h"

void removeUnusedArgs(FunctionPtr func, const std::vector<uint32_t>& UnusedIdx) {
    for (auto callerInstructions : func->callerInstructions) {
        if (auto caller = dynamic_cast<CallInstruction*>(callerInstructions.get())) {
            auto& argv = caller->argv;
            for (auto idx : UnusedIdx) {
                argv.erase(argv.begin() + idx);
            }
        }
    }
}

void UnusedArgEliminate(Module &module) {
    // 遍历每个全局函数
    for (auto &func : module.globalFunctions) {
        if (!func || func->basicBlocks.empty()) {
            continue;
        }

        // 保存原始参数列表
        auto &args = func->formalArguments;
        uint32_t idx = 0;
        std::vector<uint32_t> UnusedIdx;
        std::vector<ValuePtr> newArgs;

        // 移除未使用的参数
        for (auto arg : args) {
            const auto id = idx++;
            if (arg->getUseCount() == 0) {
                UnusedIdx.push_back(id);
            }
            else {
                newArgs.push_back(arg);
            }
        }

        // 更新函数的参数列表
        args = newArgs;

        // 如果有参数被移除，更新函数类型并删除对应的引用
        if (!UnusedIdx.empty()) {
            std::reverse(UnusedIdx.begin(), UnusedIdx.end());
            removeUnusedArgs(func, UnusedIdx);
        }
    }
}