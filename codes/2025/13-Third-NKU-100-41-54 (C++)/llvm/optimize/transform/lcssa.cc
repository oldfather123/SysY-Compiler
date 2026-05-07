#include "lcssa.h"

void LCSSAPass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        auto loopInfo = static_cast<LoopInfo*>(cfg->getLoopInfo());
        if (!loopInfo) continue;
        
        for (auto loop : loopInfo->getTopLevelLoops()) {
            processLoop(loop, cfg);
        }
    }
}

void LCSSAPass::processLoop(Loop* loop, CFG* cfg) {
    // 先递归处理所有子循环
    for (auto subLoop : loop->getSubLoops()) {
        processLoop(subLoop, cfg);
    }
    
    // 再处理当前循环
    if (loop->getExits().size() > 1) return;
    
    auto usedVars = getUsedOperandOutOfLoop(cfg, loop);
    auto exit_bb = *loop->getExits().begin();
    
    // 为每个在循环外使用的变量创建Phi节点
    for (auto [regno, type] : usedVars) {
        auto newReg = GetNewRegOperand(++cfg->max_reg);
        auto phiI = new PhiInstruction(type, newReg, cfg->max_reg);
        for (auto bb : cfg->GetPredecessor(exit_bb)) {
            phiI->AddPhi({GetNewLabelOperand(bb->block_id), GetNewRegOperand(regno)});
        }
        exit_bb->InsertInstruction(0, phiI);
        
        // 替换循环外对该变量的使用
        for (auto [id, bb] : *cfg->block_map) {
            if (loop->contains(bb)) continue;
            for (auto I : bb->Instruction_list) {
                if (I->GetOpcode() == BasicInstruction::PHI && bb == exit_bb) continue;
                
                auto operands = I->GetNonResultOperands();
                bool changed = false;
                for (auto& op : operands) {
                    if (op->GetOperandType() == BasicOperand::REG && 
                        ((RegOperand*)op)->GetRegNo() == regno) {
                        op = GetNewRegOperand(cfg->max_reg);
                        changed = true;
                    }
                }
                if (changed) I->SetNonResultOperands(operands);
            }
        }
    }
}

std::map<int, BasicInstruction::LLVMType> LCSSAPass::getUsedOperandOutOfLoop(CFG* cfg, Loop* loop) {
    std::set<int> loopDefs;
    std::map<int, BasicInstruction::LLVMType> defTypes;
    
    // 收集循环内定义的所有变量
    for (auto bb : loop->getBlocks()) {
        for (auto I : bb->Instruction_list) {
            int defReg = I->GetDefRegno();
            if (defReg != -1) {
                loopDefs.insert(defReg);
                // 对于指针类型，应该使用PTR而不是元素类型
                if (I->GetOpcode() == BasicInstruction::GETELEMENTPTR) {
                    defTypes[defReg] = BasicInstruction::PTR;
                } else {
                    defTypes[defReg] = I->GetType();
                }
            }
        }
    }
    
    // 找出在循环外使用的变量
    std::map<int, BasicInstruction::LLVMType> usedVars;
    for (auto [id, bb] : *cfg->block_map) {
        if (loop->contains(bb)) continue;
        for (auto I : bb->Instruction_list) {
            if (I->GetOpcode() == BasicInstruction::PHI && bb == *loop->getExits().begin()) continue;
            
            for (auto op : I->GetNonResultOperands()) {
                if (op->GetOperandType() == BasicOperand::REG) {
                    int regno = ((RegOperand*)op)->GetRegNo();
                    if (loopDefs.count(regno)) {
                        usedVars[regno] = defTypes[regno];
                    }
                }
            }
        }
    }
    return usedVars;
} 