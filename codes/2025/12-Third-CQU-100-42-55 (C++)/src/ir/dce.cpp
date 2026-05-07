#include "../../include/ir/dce.h"
#include "../../include/ir/mem2reg.h"
#include <set>
#include <map>
#include <iostream>
#include <algorithm>

namespace llvm_ir {
namespace dce {

// 判断指令是否有副作用（不能被删除）
static bool hasSideEffect(Instruction* inst) {
    switch (inst->opcode) {
        case Opcode::Store:
        case Opcode::Call:
        case Opcode::Ret:
        case Opcode::Br:
            return true;
        default:
            return false;
    }
}

// 判断函数是否为内置函数（不能被删除）
static bool isBuiltinFunction(const std::string& funcName) {
    static const std::set<std::string> builtinFunctions = {
        "getint", "getch", "getfloat", "getarray", "getfarray",
        "putint", "putch", "putfloat", "putarray", "putfarray",
        "putf", "starttime", "stoptime", "_sysy_stoptime", "_sysy_starttime"
    };
    return builtinFunctions.find(funcName) != builtinFunctions.end();
}

// 收集所有被调用的函数
static std::set<std::string> collectCalledFunctions(Module& module) {
    std::set<std::string> calledFunctions;
    
    for (const auto& func : module.functions) {
        for (const auto& bb : func->basicBlocks) {
            for (const auto& inst : bb->instructions) {
                if (inst->opcode == Opcode::Call) {
                    auto callInst = static_cast<CallInst*>(inst.get());
                    calledFunctions.insert(callInst->functionName);
                }
            }
        }
    }
    
    return calledFunctions;
}

// 删除未被调用的函数
void removeUnusedFunctions(Module& module) {
    std::cout << "[DCE] 开始删除未被调用的函数..." << std::endl;
    
    // 收集所有被调用的函数
    std::set<std::string> calledFunctions = collectCalledFunctions(module);
    
    // 记录删除的函数数量
    int removedCount = 0;
    
    // 遍历所有函数，删除未被调用的函数
    auto it = module.functions.begin();
    while (it != module.functions.end()) {
        const std::string& funcName = (*it)->name;
        // 跳过内置函数、main、仅声明的函数
        if (isBuiltinFunction(funcName) || funcName == "main" || funcName == "global" || (*it)->isDeclaration()) {
            ++it;
            continue;
        }
        
        // 检查函数是否被调用
        if (calledFunctions.find(funcName) == calledFunctions.end()) {
            std::cout << "[DCE] 删除未被调用的函数: " << funcName << std::endl;
            it = module.functions.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }
    
    std::cout << "[DCE] 删除了 " << removedCount << " 个未被调用的函数" << std::endl;
    
    // 删除函数后重建use-def关系
    if (removedCount > 0) {
        cfg::rebuildUseDefChainsModule(module);
    }
}

// 删除死指令
int removeDeadInstructions(Function& function) {
    cfg::rebuildUseDefChainsOnFunction(function, false);

    int totalRemoved = 0;
    bool changed = true;
    int iteration = 0;
    std::set<Instruction*> toRemove;
    while (changed) {
        changed = false;
        iteration++;
        // 1. 先处理PHI指令
        for (auto& bb : function.basicBlocks) {
            for (auto& phi: bb->phi_instructions) {
                if (phi->getUses().empty()) {
                    toRemove.insert(phi.get());
                }
            }
        }
        // 2. 再处理普通指令
        for (auto& bb : function.basicBlocks) {
            for (auto& inst: bb->instructions) {
                if (inst->getUses().empty() && !hasSideEffect(inst.get())) {
                    toRemove.insert(inst.get());
                }
            }
            // auto it = bb->instructions.begin();
            // while (it != bb->instructions.end()) {
            //     Instruction* inst = it->get();
                
            //     // 特殊处理global函数中的常量store指令 TODO, 无法判断是不是不变的const int or float...
            //     // if (function.name == "global" && inst->opcode == Opcode::Store) {
            //     //     auto storeInst = static_cast<StoreInst*>(inst);
            //     //     Value* valueOperand = storeInst->getValueOperand();
                    
            //     //     // 如果存储的值是常量，直接删除这个store指令
            //     //     if (dynamic_cast<ConstantInt*>(valueOperand) || dynamic_cast<ConstantFloat*>(valueOperand)) {
            //     //         std::cout << "[DCE] 删除global函数中的常量store指令: " << inst->toString() << std::endl;
                        
            //     //         // 从所有操作数的use链中移除
            //     //         for (auto* op : inst->operands) {
            //     //             if (op) op->removeUse(inst);
            //     //         }
            //     //         it = bb->instructions.erase(it);
            //     //         totalRemoved++;
            //     //         changed = true;
            //     //         continue;
            //     //     }
            //     // }
                
            //     if (hasSideEffect(inst)) {
            //         ++it;
            //         continue;
            //     }
                
            //     // 检查指令是否被使用
            //     if (inst->getUses().empty()) {
            //         // 从所有操作数的use链中移除
            //         for (auto* op : inst->operands) {
            //             if (op) op->removeUse(inst);
            //         }
            //         it = bb->instructions.erase(it);
            //         totalRemoved++;
            //         changed = true;
            //     } else {
            //         ++it;
            //     }
            // }
        }
        for (auto& inst: toRemove) {
            std::cout << "[DCE] Deleting dead inst: "; std::cout << inst->toString() << std::endl;
            inst->removeFromParent();
            totalRemoved++;
            changed = true;
        }
        toRemove.clear();
    }
    
    // 返回删除的指令数量，供调用者使用
    return totalRemoved;
}

// DCE 优化 pass 主入口
void runOnModule(Module& module) {
    // 1. 删除未被调用的函数
    removeUnusedFunctions(module);
    
    // 2. 删除死指令
    std::cout << "[DCE] 开始删除死指令..." << std::endl;
    int totalInstructionsRemoved = 0;
    for (auto& func : module.functions) {
        totalInstructionsRemoved += removeDeadInstructions(*func);
    }
    
    // 3. 如果有任何删除操作，重建use-def关系
    if (totalInstructionsRemoved > 0) {
        std::cout << "[DCE] 死指令删除完成，共删除 " << totalInstructionsRemoved << " 条指令" << std::endl;
        cfg::rebuildUseDefChainsModule(module);
    }
    
    std::cout << "[DCE] 死代码消除优化完成" << std::endl;
}

} // namespace dce
} // namespace llvm_ir
