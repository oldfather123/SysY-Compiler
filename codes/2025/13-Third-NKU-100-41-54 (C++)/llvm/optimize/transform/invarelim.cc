#include "invarelim.h"
#include <iostream>
#include <sstream>

void InvariantVariableEliminationPass::Execute() {
    INVAR_ELIM_DEBUG_PRINT("开始归纳变量消除优化");
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        INVAR_ELIM_DEBUG_PRINT("处理函数: " << cfg->function_def->GetFunctionName());
        processFunction(cfg);
    }
    INVAR_ELIM_DEBUG_PRINT("归纳变量消除优化完成");
}

void InvariantVariableEliminationPass::processFunction(CFG* C) {
    ScalarEvolution* SE = C->getSCEVInfo();
    if (!SE) {
        INVAR_ELIM_DEBUG_PRINT("警告: CFG没有SCEV信息，跳过");
        return;
    }
    
    LoopInfo* LI = C->getLoopInfo();
    if (!LI) {
        INVAR_ELIM_DEBUG_PRINT("警告: CFG没有LoopInfo，跳过");
        return;
    }
    
    // 递归处理所有循环
    std::vector<Loop*> worklist;
    for (auto loop : LI->getTopLevelLoops()) {
        worklist.push_back(loop);
    }
    
    while (!worklist.empty()) {
        Loop* loop = worklist.back();
        worklist.pop_back();
        
        INVAR_ELIM_DEBUG_PRINT("处理循环，header block_id=" << loop->getHeader()->block_id);
        
        processLoop(loop, C, SE);
        
        // 添加子循环到工作队列
        for (auto subloop : loop->getSubLoops()) {
            worklist.push_back(subloop);
        }
    }
}

void InvariantVariableEliminationPass::processLoop(Loop* loop, CFG* C, ScalarEvolution* SE) {
    // 分析循环中的归纳变量
    auto inductionGroups = analyzeInductionVariables(loop, C, SE);
    
    // 处理每组具有相同归纳模式的变量
    for (const auto& [pattern, variables] : inductionGroups) {
        if (variables.size() > 1) {
            INVAR_ELIM_DEBUG_PRINT("发现相同归纳模式的变量组: " << pattern);
            for (auto var : variables) {
                INVAR_ELIM_DEBUG_PRINT("  " << var->GetFullName());
            }
            
            // 选择代表变量
            Operand representative = selectRepresentativeVariable(variables, loop);
            INVAR_ELIM_DEBUG_PRINT("选择代表变量: " << representative->GetFullName());
            
            // 替换其他变量的使用并删除对应的phi指令
            for (auto var : variables) {
                if (var != representative) {
                    INVAR_ELIM_DEBUG_PRINT("替换变量: " << var->GetFullName() << " -> " << representative->GetFullName());
                    replaceVariableUses(var, representative, C);
                    removePhiInstruction(var, loop);
                }
            }
            

        }
    }
}

std::map<std::string, std::vector<Operand>> InvariantVariableEliminationPass::analyzeInductionVariables(Loop* loop, CFG* C, ScalarEvolution* SE) {
    std::map<std::string, std::vector<Operand>> inductionGroups;
    
    // 只分析循环头部的phi指令
    LLVMBlock header = loop->getHeader();
    for (auto inst : header->Instruction_list) {
        if (inst->GetOpcode() == BasicInstruction::PHI && inst->GetResult()) {
            Operand var = inst->GetResult();
            SCEV* scev = SE->getSCEV(var, loop);
            SCEV* simplifiedScev = SE->simplify(scev);
            
            if (simplifiedScev && simplifiedScev->getType() == scAddRecExpr) {
                auto* addrec = dynamic_cast<SCEVAddRecExpr*>(simplifiedScev);
                if (addrec && addrec->getLoop() == loop) {
                    // 使用SCEV的字符串表示作为模式标识符
                    std::ostringstream oss;
                    simplifiedScev->print(oss);
                    std::string pattern = oss.str();
                    
                    INVAR_ELIM_DEBUG_PRINT("变量 " << var->GetFullName() << " 的归纳模式: " << pattern);
                    
                    inductionGroups[pattern].push_back(var);
                }
            }
        }
    }
    
    return inductionGroups;
}



Operand InvariantVariableEliminationPass::selectRepresentativeVariable(const std::vector<Operand>& variables, Loop* loop) {
    // 优先选择在循环控制中使用的变量
    for (auto var : variables) {
        if (isUsedInLoopControl(var, loop)) {
            return var;
        }
    }
    
    // 如果没有在循环控制中使用的变量，选择第一个
    return variables[0];
}

bool InvariantVariableEliminationPass::isUsedInLoopControl(Operand var, Loop* loop) {
    LLVMBlock header = loop->getHeader();
    for (auto inst : header->Instruction_list) {
        if (inst->GetOpcode() == BasicInstruction::ICMP) {
            auto* icmp = dynamic_cast<IcmpInstruction*>(inst);
            if (icmp && (icmp->GetOp1() == var || icmp->GetOp2() == var)) {
                return true;
            }
        }
    }
    return false;
}

void InvariantVariableEliminationPass::replaceVariableUses(Operand oldVar, Operand newVar, CFG* C) {
    // 替换所有基本块中的变量使用
    for (auto& [blockId, block] : *C->block_map) {
        for (auto inst : block->Instruction_list) {
            // 替换指令的操作数
            auto operands = inst->GetNonResultOperands();
            bool changed = false;
            for (auto& operand : operands) {
                if (operand == oldVar) {
                    operand = newVar;
                    changed = true;
                }
            }
            if (changed) {
                inst->SetNonResultOperands(operands);
            }
        }
    }
}

void InvariantVariableEliminationPass::removePhiInstruction(Operand var, Loop* loop) {
    // 删除循环头部中定义该变量的phi指令
    LLVMBlock header = loop->getHeader();
    for (auto it = header->Instruction_list.begin(); it != header->Instruction_list.end();) {
        if ((*it)->GetOpcode() == BasicInstruction::PHI && (*it)->GetResult() == var) {
            INVAR_ELIM_DEBUG_PRINT("删除phi指令: " << var->GetFullName());
            
            // 获取phi指令的操作数，删除它们的定义
            auto* phi = dynamic_cast<PhiInstruction*>(*it);
            if (phi) {
                auto operands = phi->GetNonResultOperands();
                for (auto operand : operands) {
                    removeOperandDefinition(operand, loop);
                }
            }
            
            it = header->Instruction_list.erase(it);
        } else {
            ++it;
        }
    }
}

void InvariantVariableEliminationPass::removeOperandDefinition(Operand operand, Loop* loop) {
    // 检查操作数是否还在其他地方被使用
    if (isOperandStillUsed(operand, loop)) {
        return; // 如果还在使用，不删除定义
    }
    
    // 在循环的所有基本块中查找并删除定义该操作数的指令
    for (auto block : loop->getBlocks()) {
        for (auto it = block->Instruction_list.begin(); it != block->Instruction_list.end();) {
            if ((*it)->GetResult() == operand) {
                INVAR_ELIM_DEBUG_PRINT("删除操作数定义: " << operand->GetFullName());
                it = block->Instruction_list.erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool InvariantVariableEliminationPass::isOperandStillUsed(Operand operand, Loop* loop) {
    // 检查操作数是否还在循环中被使用
    for (auto block : loop->getBlocks()) {
        for (auto inst : block->Instruction_list) {
            auto operands = inst->GetNonResultOperands();
            for (auto op : operands) {
                if (op == operand) {
                    return true; // 还在使用
                }
            }
        }
    }
    return false; // 不再使用
}


