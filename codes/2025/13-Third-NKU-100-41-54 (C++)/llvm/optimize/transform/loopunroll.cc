#include "loopunroll.h"
#include <iostream>

// #define loop_unroll_debug

#ifdef loop_unroll_debug
#define LOOP_UNROLL_DEBUG_PRINT(x) do { x; } while(0)
#else
#define LOOP_UNROLL_DEBUG_PRINT(x) do {} while(0)
#endif

void LoopUnrollPass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "LoopUnrollPass for function " 
                                      << cfg->function_def->GetFunctionName() << std::endl);
        
        LoopUnroll(cfg);
        
        cfg->BuildCFG();
        cfg->getDomTree()->BuildDominatorTree();
    }
}

void LoopUnrollPass::LoopUnroll(CFG *C) {
    auto loop_info = C->getLoopInfo();
    if (!loop_info) return;
    
    ScalarEvolution *SE = C->getSCEVInfo();
    if (!SE) return;
    
    for (auto loop : loop_info->getTopLevelLoops()) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "分析循环，header block_id=" << loop->getHeader()->block_id << std::endl);
        
        processLoop(loop, C, SE);
    }
}

void LoopUnrollPass::processLoop(Loop* loop, CFG* C, ScalarEvolution* SE) {
    for (auto sub_loop : loop->getSubLoops()) {
        processLoop(sub_loop, C, SE);
    }
    
    if (!canUnrollLoop(loop, C, SE)) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "循环不可展开" << std::endl);
        return;
    }
    
    unrollLoop(loop, C, SE);
}

bool LoopUnrollPass::canUnrollLoop(Loop* loop, CFG* C, ScalarEvolution* SE) {
    if (loop->getExits().size() != 1 || loop->getExitings().size() != 1) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "循环出口不唯一，不可展开" << std::endl);
        return false;
    }
    
    LLVMBlock header = loop->getHeader();
    if (header->Instruction_list.empty()) return false;
    
    Instruction last_inst = header->Instruction_list.back();
    if (last_inst->GetOpcode() != BasicInstruction::BR_COND) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "循环头部不是条件跳转，不可展开" << std::endl);
        return false;
    }
    
    BrCondInstruction* br_cond = (BrCondInstruction*)last_inst;
    Operand cond = br_cond->GetCond();
    
    Instruction cond_inst = SE->getDef(cond);
    if (!cond_inst || cond_inst->GetOpcode() != BasicInstruction::ICMP) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "循环条件不是比较指令，不可展开" << std::endl);
        return false;
    }
    
    IcmpInstruction* icmp = (IcmpInstruction*)cond_inst;
    Operand op1 = icmp->GetOp1();
    Operand op2 = icmp->GetOp2();
    
    SCEV* scev1 = SE->getSCEV(op1, loop);
    SCEV* scev2 = SE->getSCEV(op2, loop);
    
    if (!isConstantLoop(scev1, scev2, loop, SE)) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "不是常数循环，不可展开" << std::endl);
        return false;
    }

    const int MAX_LOOP_BODY_SIZE = 150; 
    int loop_body_size = 0;
    for (auto block : loop->getBlocks()) {
        loop_body_size += block->Instruction_list.size();
    }
    if (loop_body_size > MAX_LOOP_BODY_SIZE) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "循环体过大（" << loop_body_size << " 条指令），不进行展开" << std::endl);
        return false;
    }
    
    return true;
}

// 检查是否有一个是AddRecExpr（归纳变量），另一个是常数或循环不变量
bool LoopUnrollPass::isConstantLoop(SCEV* scev1, SCEV* scev2, Loop* loop, ScalarEvolution* SE) {

	if(loop->getExits().size() != 1) {
		LOOP_UNROLL_DEBUG_PRINT(std::cerr << "循环出口不唯一，不可展开" << std::endl);
		return false;
	}

    SCEV* induction_var = nullptr;
    SCEV* bound = nullptr;
    
    if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(scev1)) {
        if (addrec->getLoop() == loop && dynamic_cast<SCEVConstant*>(scev2)) {
            induction_var = scev1;
            bound = scev2;
        }
    } else if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(scev2)) {
        if (addrec->getLoop() == loop && dynamic_cast<SCEVConstant*>(scev1)) {
            induction_var = scev2;
            bound = scev1;
        }
    }
    
    if (!induction_var || !bound) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "未找到归纳变量" << std::endl);
        return false;
    }
    
    // 检查步长是否为常数
    auto* addrec = dynamic_cast<SCEVAddRecExpr*>(induction_var);    
    SCEV* step = SE->simplify(addrec->getStep());
    if (!dynamic_cast<SCEVConstant*>(step)) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "步长不是常数" << std::endl);
        return false;
    }
    
    // 只允许边界为常数
    SCEV* simplified_bound = SE->simplify(bound);
    if (!dynamic_cast<SCEVConstant*>(simplified_bound)) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "边界不是常数，不可展开" << std::endl);
        return false;
    }
    
    return true;
}

void LoopUnrollPass::unrollLoop(Loop* loop, CFG* C, ScalarEvolution* SE) {
    LOOP_UNROLL_DEBUG_PRINT(std::cerr << "开始展开循环" << std::endl);
    
    LLVMBlock header = loop->getHeader();
    LLVMBlock preheader = loop->getPreheader();
    LLVMBlock latch = *loop->getLatches().begin();
    LLVMBlock exit = *loop->getExits().begin();
    
    BrCondInstruction* br_cond = (BrCondInstruction*)header->Instruction_list.back();
    IcmpInstruction* icmp = (IcmpInstruction*)SE->getDef(br_cond->GetCond());
    
    // 获取归纳变量和边界
    Operand induction_var_op = icmp->GetOp1();
    Operand bound_op = icmp->GetOp2();
    
    SCEV* induction_scev = SE->getSCEV(induction_var_op, loop);
    SCEV* bound_scev = SE->getSCEV(bound_op, loop);
    
    if (!dynamic_cast<SCEVAddRecExpr*>(induction_scev)) {
        std::swap(induction_var_op, bound_op);
        std::swap(induction_scev, bound_scev);
    }
    
    auto* addrec = dynamic_cast<SCEVAddRecExpr*>(induction_scev);
    if (!addrec) return;
    
    // 计算循环参数和迭代次数
    int start_val = getConstantValue(addrec->getStart());
    int step_val = getConstantValue(addrec->getStep());
    int bound_val = getConstantValue(bound_scev);
    
    if (start_val == -1 || step_val == -1 || bound_val == -1) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "无法获取循环参数" << std::endl);
        return;
    }
    
    int iterations = calculateIterations(start_val, bound_val, step_val, icmp->GetCond());
    if (iterations <= 0) {
        LOOP_UNROLL_DEBUG_PRINT(std::cerr << "迭代次数无效: " << iterations << std::endl);
        return;
    }
    
    LOOP_UNROLL_DEBUG_PRINT(std::cerr << "循环参数: start=" << start_val << ", bound=" << bound_val 
                                    << ", step=" << step_val << ", iterations=" << iterations << std::endl);
    
	// 执行循环展开，展开次数为4
    const int UNROLL_FACTOR = 4;
    int unroll_iterations = std::min(iterations, UNROLL_FACTOR);
    int remaining_iterations = iterations - unroll_iterations;
    
    std::vector<LLVMBlock> unrolled_blocks;
    createUnrolledBlocks(loop, C, unroll_iterations, unrolled_blocks, SE);
    
    updateControlFlow(loop, C, unroll_iterations, remaining_iterations, unrolled_blocks, 
                     induction_var_op, step_val, bound_op, icmp->GetCond(), SE);
}

int LoopUnrollPass::getConstantValue(SCEV* scev) {
    if (auto* constant = dynamic_cast<SCEVConstant*>(scev)) {
        return constant->getValue()->GetIntImmVal();
    }
    return -1;
}

int LoopUnrollPass::calculateIterations(int start, int bound, int step, BasicInstruction::IcmpCond cond) {
    if (step == 0) return -1;
    
    int iterations = 0;
    switch (cond) {
        case BasicInstruction::slt: // <
            if (step > 0) {
                iterations = (bound - start + step - 1) / step;
            } else {
                iterations = (start - bound - step - 1) / (-step);
            }
            break;
        case BasicInstruction::sle: // <=
            if (step > 0) {
                iterations = (bound - start) / step + 1;
            } else {
                iterations = (start - bound) / (-step) + 1;
            }
            break;
        case BasicInstruction::sgt: // >
            if (step < 0) {
                iterations = (start - bound + (-step) - 1) / (-step);
            } else {
                iterations = (bound - start - step - 1) / step;
            }
            break;
        case BasicInstruction::sge: // >=
            if (step < 0) {
                iterations = (start - bound) / (-step) + 1;
            } else {
                iterations = (bound - start) / step + 1;
            }
            break;
        default:
            return -1;
    }
    
    return std::max(0, iterations);
}

void LoopUnrollPass::createUnrolledBlocks(Loop* loop, CFG* C, int unroll_count, 
                                         std::vector<LLVMBlock>& unrolled_blocks, ScalarEvolution* SE) {
    std::vector<LLVMBlock> loop_blocks = loop->getBlocks();
    
    // 维护跨迭代的寄存器映射
    // 这样可以确保相同变量的不同迭代使用不同的寄存器
    std::map<int, std::map<int, int>> iteration_reg_maps; // iteration -> (old_reg -> new_reg)
    
    // 为每次迭代创建循环体的副本
    for (int i = 0; i < unroll_count; i++) {
        std::vector<LLVMBlock> iteration_blocks;
        
        // 为循环中的每个基本块创建副本
        for (LLVMBlock orig_block : loop_blocks) {
            LLVMBlock new_block = C->GetNewBlock();
            copyInstructionsWithRenaming(orig_block, new_block, C, i, iteration_reg_maps, loop, SE);
            iteration_blocks.push_back(new_block);
        }
        
        // 将这次迭代的块添加到总列表中
        unrolled_blocks.insert(unrolled_blocks.end(), iteration_blocks.begin(), iteration_blocks.end());
    }
}

void LoopUnrollPass::copyInstructionsWithRenaming(LLVMBlock src, LLVMBlock dst, CFG* C, int iteration,
                                                 std::map<int, std::map<int, int>>& iteration_reg_maps,
                                                 Loop* loop, ScalarEvolution* SE) {
    // 获取当前迭代的寄存器映射
    std::map<int, int>& reg_map = iteration_reg_maps[iteration];
    
    for (Instruction inst : src->Instruction_list) {
        if (inst->isTerminator()) continue;
        
        Instruction new_inst = cloneInstruction(inst, C, reg_map, iteration, loop, SE);
        if (new_inst) {
            dst->Instruction_list.push_back(new_inst);
        }
    }
}

Instruction LoopUnrollPass::cloneInstruction(Instruction inst, CFG* C, std::map<int, int>& reg_map, int iteration, Loop* loop, ScalarEvolution* SE) {
    // 克隆指令并重命名寄存器
    Instruction new_inst = inst->Clone();
    
    // 创建寄存器映射表
    std::map<int, int> store_map; // old_reg -> new_reg
    std::map<int, int> use_map;   // old_reg -> new_reg
    
    // 处理结果寄存器（定义）
    if (new_inst->GetResult() && new_inst->GetResult()->GetOperandType() == BasicOperand::REG) {
        RegOperand* result_reg = (RegOperand*)new_inst->GetResult();
        int old_reg = result_reg->GetRegNo();
        
        // 为这个寄存器分配新的编号
        if (reg_map.find(old_reg) == reg_map.end()) {
            reg_map[old_reg] = ++C->max_reg;
        }
        
        store_map[old_reg] = reg_map[old_reg];
    }
    
    // 处理操作数寄存器（使用）
    auto operands = new_inst->GetNonResultOperands();
    for (Operand op : operands) {
        if (op->GetOperandType() == BasicOperand::REG) {
            RegOperand* reg_op = (RegOperand*)op;
            int old_reg = reg_op->GetRegNo();
            
            // 智能寄存器重命名策略
            bool need_rename = true;

			// 寄存器重命名策略：
			// 1. 如果是循环不变量，可以在不同迭代间共享（不重命名）
			// 2. 如果是归纳变量，每个迭代都需要不同的寄存器
			// 3. 如果是其他变量，每个迭代都需要不同的寄存器
			// 这里简化处理：假设所有寄存器都需要重命名
			// 在实际实现中，可以通过SCEV分析来判断是否为循环不变量
			// 例如：如果SCEV分析显示某个变量是循环不变量，则不需要重命名
				            
            // 如果有循环信息和SCEV分析，检查是否为循环不变量
            if (loop && SE) {
                // 检查这个寄存器是否在循环中定义
                Instruction def_inst = SE->getDef(op);
                if (def_inst) {
                    LLVMBlock def_block = C->GetBlockWithId(def_inst->GetBlockID());
                    if (def_block && !loop->contains(def_block)) {
                        // 定义在循环外，是循环不变量，不需要重命名
                        need_rename = false;
                    }
                }
            }
            
            // 如果需要重命名，分配新的寄存器编号
            if (need_rename) {
                if (reg_map.find(old_reg) == reg_map.end()) {
                    reg_map[old_reg] = ++C->max_reg;
                }
                use_map[old_reg] = reg_map[old_reg];
            }
        }
    }
    
    // 使用ChangeReg方法进行寄存器重命名
    if (!store_map.empty() || !use_map.empty()) {
        new_inst->ChangeReg(store_map, use_map);
    }
    
    return new_inst;
}

void LoopUnrollPass::updateControlFlow(Loop* loop, CFG* C, int unroll_count, int remaining_iterations,
                                      const std::vector<LLVMBlock>& unrolled_blocks,
                                      Operand induction_var, int step_val, Operand bound, 
                                      BasicInstruction::IcmpCond cond, ScalarEvolution* SE) {
    LLVMBlock header = loop->getHeader();
    LLVMBlock preheader = loop->getPreheader();
    LLVMBlock exit = *loop->getExits().begin();
    
    // 创建剩余循环（如果需要）
    LLVMBlock remaining_loop = nullptr;
    if (remaining_iterations > 0) {
        remaining_loop = createRemainingLoop(loop, C, remaining_iterations, 
                                           induction_var, step_val, bound, cond, SE);
    }
    
    // 修改preheader的跳转
    // preheader -> 第一个展开块
    if (!unrolled_blocks.empty()) {
        updatePreheaderJump(preheader, unrolled_blocks[0]);
    }
    
    // 连接展开的块
    connectUnrolledBlocks(unrolled_blocks, loop, C);
    
    // 连接展开块到剩余循环或出口
    if (!unrolled_blocks.empty()) {
        // 找到最后一个展开块（最后一个迭代的最后一个块）
        std::vector<LLVMBlock> loop_blocks = loop->getBlocks();
        int blocks_per_iteration = loop_blocks.size();
        LLVMBlock last_unrolled_block = unrolled_blocks[unrolled_blocks.size() - blocks_per_iteration];
        
        if (remaining_loop) {
            connectToRemainingLoop(last_unrolled_block, remaining_loop, C);
        } else {
            connectToExit(last_unrolled_block, exit, C);
        }
    }
    
    // 删除原始循环块（除了header，因为它可能被其他块引用）
    removeOriginalLoopBlocks(loop, C);
}

LLVMBlock LoopUnrollPass::createRemainingLoop(Loop* loop, CFG* C, int remaining_iterations,
                                             Operand induction_var, int step_val, Operand bound,
                                             BasicInstruction::IcmpCond cond, ScalarEvolution* SE) {
    // 创建剩余循环的基本块
    LLVMBlock remaining_header = C->GetNewBlock();
    LLVMBlock remaining_body = C->GetNewBlock();
    LLVMBlock remaining_latch = C->GetNewBlock();
    
    // 复制循环体指令
    std::vector<LLVMBlock> loop_blocks = loop->getBlocks();
    std::map<int, std::map<int, int>> iteration_reg_maps; // 为剩余循环创建寄存器映射
    for (LLVMBlock orig_block : loop_blocks) {
        if (orig_block != loop->getHeader()) {
            copyInstructionsWithRenaming(orig_block, remaining_body, C, 0, iteration_reg_maps, loop, SE);
        }
    }
    
    // 创建循环条件
    // 这里需要根据具体的循环结构来创建条件
    // 简化实现：假设是简单的递增循环
    
    return remaining_header;
}

void LoopUnrollPass::updatePreheaderJump(LLVMBlock preheader, LLVMBlock first_unrolled_block) {
    // 修改preheader的最后一条指令（无条件跳转）
    if (!preheader->Instruction_list.empty()) {
        Instruction last_inst = preheader->Instruction_list.back();
        if (last_inst->GetOpcode() == BasicInstruction::BR_UNCOND) {
            BrUncondInstruction* br = (BrUncondInstruction*)last_inst;
            br->ChangeDestLabel(GetNewLabelOperand(first_unrolled_block->block_id));
        }
    }
}

void LoopUnrollPass::connectUnrolledBlocks(const std::vector<LLVMBlock>& unrolled_blocks, 
                                          Loop* loop, CFG* C) {
    std::vector<LLVMBlock> loop_blocks = loop->getBlocks();
    int blocks_per_iteration = loop_blocks.size();
    
    // 连接每次迭代内的块
    for (size_t i = 0; i < unrolled_blocks.size(); i += blocks_per_iteration) {
        // 连接这次迭代内的块
        for (int j = 0; j < blocks_per_iteration - 1; j++) {
            LLVMBlock current = unrolled_blocks[i + j];
            LLVMBlock next = unrolled_blocks[i + j + 1];
            
            // 添加无条件跳转到下一个块
            BrUncondInstruction* br = new BrUncondInstruction(GetNewLabelOperand(next->block_id));
            current->Instruction_list.push_back(br);
        }
        
        // 连接这次迭代的最后一个块到下一次迭代的第一个块（如果有的话）
        if (i + blocks_per_iteration < unrolled_blocks.size()) {
            LLVMBlock last_in_iteration = unrolled_blocks[i + blocks_per_iteration - 1];
            LLVMBlock first_next_iteration = unrolled_blocks[i + blocks_per_iteration];
            
            BrUncondInstruction* br = new BrUncondInstruction(GetNewLabelOperand(first_next_iteration->block_id));
            last_in_iteration->Instruction_list.push_back(br);
        }
    }
}

void LoopUnrollPass::connectToRemainingLoop(LLVMBlock last_unrolled, LLVMBlock remaining_loop, CFG* C) {
    BrUncondInstruction* br = new BrUncondInstruction(GetNewLabelOperand(remaining_loop->block_id));
    last_unrolled->Instruction_list.push_back(br);
}

void LoopUnrollPass::connectToExit(LLVMBlock last_unrolled, LLVMBlock exit, CFG* C) {
    BrUncondInstruction* br = new BrUncondInstruction(GetNewLabelOperand(exit->block_id));
    last_unrolled->Instruction_list.push_back(br);
}

void LoopUnrollPass::removeOriginalLoopBlocks(Loop* loop, CFG* C) {
    // 删除原始循环块（除了可能被其他地方引用的块）
    // 这里需要小心处理，确保不删除被其他地方引用的块
    // 简化实现：暂时不删除，让后续的优化pass处理
}
