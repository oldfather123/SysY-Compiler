#include "../../include/ir/adce.h"
#include "../../include/ir/cfg.h"
#include "../../include/ir/function_analysis.h"
#include <iostream>
#include <set>
#include <map>
#include <deque>
#include <algorithm>
#include <unordered_map>

namespace llvm_ir {
namespace adce {

// 判断指令是否有副作用（不能被删除）
bool hasSideEffect(Instruction* inst, function_analysis::FunctionAttributeAnalyzer& analyzer) {
    switch (inst->opcode) {
        case Opcode::Store:
        case Opcode::Ret:
        case Opcode::Br:
            return true;
        case Opcode::Call:
            if (analyzer.isCallSafe(static_cast<CallInst*>(inst))) {
                // 如果是readnone/readonly，则没有副作用
                return false;
            }
            return true;
        default:
            return false;
    }
}

// 判断函数是否为内置函数（不能被删除）
bool isBuiltinFunction(const std::string& funcName) {
    static const std::set<std::string> builtinFunctions = {
        "getint", "getch", "getfloat", "getarray", "getfarray",
        "putint", "putch", "putfloat", "putarray", "putfarray",
        "putf", "starttime", "stoptime", "_sysy_stoptime", "_sysy_starttime"
    };
    return builtinFunctions.find(funcName) != builtinFunctions.end();
}

// 构建控制依赖图（CDG）
std::vector<std::vector<BasicBlock*>> buildCDG(Function& function, cfg::DominatorTree& postDomTree) {
    std::vector<std::vector<BasicBlock*>> cdgPrecursor;
    
    // 构建基本块到索引的映射
    std::unordered_map<BasicBlock*, int> blockToIndex;
    int index = 0;
    for (auto& bb : function.basicBlocks) {
        blockToIndex[bb.get()] = index++;
    }
    
    // 为每个基本块初始化CDG前驱列表
    cdgPrecursor.resize(function.basicBlocks.size());
    
    // 根据后向支配边界构建CDG
    for (auto& bb : function.basicBlocks) {
        auto domFrontier = postDomTree.getDominanceFrontier(bb.get());
        
        for (auto vbb : domFrontier) {
            auto it = blockToIndex.find(vbb);
            if (it != blockToIndex.end()) {
                cdgPrecursor[it->second].push_back(bb.get());
            }
        }
    }
    
    return cdgPrecursor;
}

// 在单个函数上执行ADCE
bool runOnFunction(Function& function, function_analysis::FunctionAttributeAnalyzer& analyzer) {
    std::cout << "[ADCE] 在函数 " << function.name << " 上执行激进死代码消除..." << std::endl;
    
    // 1. 构建CFG
    cfg::buildCFG(function);
    
    // 2. 构建正向支配树和后向支配树
    cfg::DominatorTree domTree;
    cfg::DominatorTree postDomTree;
    
    domTree.runOnFunction(function, false);  // 正向支配树
    postDomTree.runOnFunction(function, true); // 后向支配树
    
    // 3. 构建控制依赖图
    auto cdgPrecursor = buildCDG(function, postDomTree);
    
    // 4. 初始化工作列表和数据结构
    std::deque<Instruction*> worklist;
    std::map<Value*, Instruction*> defMap;
    std::set<Instruction*> liveInstructionSet;
    std::set<BasicBlock*> liveBBSet;
    
    // 5. 初始化：收集所有定义和标记有副作用的指令为活指令
    for (auto& bb : function.basicBlocks) {
        for (auto& inst : bb->instructions) {
            inst->parent = bb.get();
            
            // 标记有副作用的指令为活指令
            if (hasSideEffect(inst.get(), analyzer)) {
                worklist.push_back(inst.get());
            }
            
            // 收集定义
            if (!inst->name.empty()) {
                defMap[inst.get()] = inst.get();
            }
        }
        
        // 处理PHI指令
        for (auto& phi : bb->phi_instructions) {
            phi->parent = bb.get();
            if (!phi->name.empty()) {
                defMap[phi.get()] = phi.get();
            }
        }
    }
    
    // 6. 主循环：传播活指令
    while (!worklist.empty()) {
        auto inst = worklist.front();
        worklist.pop_front();
        
        if (liveInstructionSet.find(inst) != liveInstructionSet.end()) {
            continue;
        }
        
        liveInstructionSet.insert(inst);
        auto parentBB = inst->parent;
        liveBBSet.insert(parentBB);
        
        // 处理PHI指令的特殊情况
        if (inst->opcode == Opcode::PHI) {
            auto phiInst = static_cast<PhiInst*>(inst);
            for (auto& [val, fromBB] : phiInst->incoming_values) {
                // 如果PHI指令是活的，那么其incoming值也应该是活的
                if (val && defMap.find(val) != defMap.end()) {
                    auto defInst = defMap[val];
                    if (liveInstructionSet.find(defInst) == liveInstructionSet.end()) {
                        worklist.push_front(defInst);
                    }
                }
                
                // 控制依赖：如果PHI是活的，那么其前驱块的终止指令也应该是活的
                auto terminalInst = fromBB->getTerminator();
                if (terminalInst && liveInstructionSet.find(terminalInst) == liveInstructionSet.end()) {
                    worklist.push_front(terminalInst);
                    liveBBSet.insert(fromBB);
                }
            }
        }
        
        // 处理控制依赖
        if (parentBB) {
            // 找到parentBB在基本块列表中的索引
            int parentIndex = 0;
            for (auto& bb : function.basicBlocks) {
                if (bb.get() == parentBB) {
                    break;
                }
                parentIndex++;
            }
            
            if (parentIndex < cdgPrecursor.size()) {
                for (auto cdgPre : cdgPrecursor[parentIndex]) {
                    auto terminalInst = cdgPre->getTerminator();
                    if (terminalInst && liveInstructionSet.find(terminalInst) == liveInstructionSet.end()) {
                        worklist.push_front(terminalInst);
                    }
                }
            }
        }
        
        // 处理操作数依赖
        for (auto op : inst->operands) {
            if (op && defMap.find(op) != defMap.end()) {
                auto defInst = defMap[op];
                if (liveInstructionSet.find(defInst) == liveInstructionSet.end()) {
                    worklist.push_front(defInst);
                }
            }
        }
    }
    
    // 7. 删除死指令和不可达的基本块
    int removedInstructions = 0;
    int removedBlocks = 0;
    
    // 处理基本块中的指令
    for (auto& bb : function.basicBlocks) {
        auto it = bb->instructions.begin();
        while (it != bb->instructions.end()) {
            auto inst = it->get();
            
            if (liveInstructionSet.find(inst) == liveInstructionSet.end()) {
                // 删除死指令
                // std::cout << "[ADCE] 删除死指令: " << inst->name << std::endl;
                // 从操作数的use链中移除
                for (auto op : inst->operands) {
                    if (op) op->removeUse(inst);
                }
                it = bb->instructions.erase(it);
                removedInstructions++;
            } else {
                // 指令是活的，不需要修改其操作数
                ++it;
            }
        }
        
        // 处理PHI指令
        auto phiIt = bb->phi_instructions.begin();
        while (phiIt != bb->phi_instructions.end()) {
            auto phi = phiIt->get();
            
            if (liveInstructionSet.find(phi) == liveInstructionSet.end()) {
                // 删除死PHI指令
                // std::cout << "[ADCE] 删除死PHI指令: " << phi->name << std::endl;
                // 从incoming值的use链中移除
                for (auto& [val, fromBB] : phi->incoming_values) {
                    if (val) val->removeUse(phi);
                }
                phiIt = bb->phi_instructions.erase(phiIt);
                removedInstructions++;
            } else {
                // PHI指令是活的，不需要修改其incoming值
                ++phiIt;
            }
        }
    }
    
    // 8. 处理死基本块
    std::vector<BasicBlock*> deadBlocks;
    for (auto& bb : function.basicBlocks) {
        if (liveBBSet.find(bb.get()) == liveBBSet.end() && bb.get() != function.getEntryBlock()) {
            deadBlocks.push_back(bb.get());
        }
    }

    for (auto bb : deadBlocks) {
        std::cout << "[ADCE] 删除死基本块: " << bb->getName() << std::endl;
        function.deleteBasicBlock(bb); 
        removedBlocks++;
    }
    
    // 9. 重建use-def关系
    if (removedInstructions > 0) {
        std::cout << "[ADCE] 重建use-def关系" << std::endl;
        cfg::rebuildUseDefChainsOnFunction(function, false);
    }
    if (removedBlocks > 0) {
        std::cout << "[ADCE] 重建CFG" << std::endl;
        cfg::buildCFG(function);
    }
    
    std::cout << "[ADCE] 函数 " << function.name << " 完成，删除了 " 
              << removedInstructions << " 条指令和 " << removedBlocks << " 个基本块" << std::endl;
    
    return removedInstructions > 0 || removedBlocks > 0;
}

// ADCE 优化 pass 主入口
bool runOnModule(Module& module) {
    std::cout << "[ADCE] 开始激进死代码消除优化..." << std::endl;
    
    bool changed = false;

    function_analysis::FunctionAttributeAnalyzer analyzer(&module);
    analyzer.analyzeModule(module);
    
    for (auto& func : module.functions) {
        // 跳过内置函数和仅声明的函数
        if (isBuiltinFunction(func->name) || func->isDeclaration()) {
            continue;
        }
        
        changed |= runOnFunction(*func, analyzer);
    }
    
    std::cout << "[ADCE] 激进死代码消除优化完成" << std::endl;
    return changed;
}

} // namespace adce
} // namespace llvm_ir 