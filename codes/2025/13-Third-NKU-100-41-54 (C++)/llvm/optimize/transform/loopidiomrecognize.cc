#include "loopidiomrecognize.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <set>
#include "../../include/Instruction.h"

// 调试宏定义
// 精简调试输出，只保留关键信息
// #define LOOP_IDIOM_DEBUG

#ifdef LOOP_IDIOM_DEBUG
#define LOOP_IDIOM_DEBUG_PRINT(x) do {std::cerr << "[LoopIdiom] "; x; } while(0)
#else
#define LOOP_IDIOM_DEBUG_PRINT(x) do {} while(0)
#endif

void LoopIdiomRecognizePass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        processFunction(cfg);
        
        // 重新构建CFG和支配树
        cfg->BuildCFG();
        cfg->getDomTree()->BuildDominatorTree();
    }
}

void LoopIdiomRecognizePass::processFunction(CFG* C) {
    auto loop_info = C->getLoopInfo();
    if (!loop_info) return;
    
    ScalarEvolution* SE = C->getSCEVInfo();
    if (!SE) return;
    
    for (auto loop : loop_info->getTopLevelLoops()) {
        processLoop(loop, C, SE);
    }
}

void LoopIdiomRecognizePass::processLoop(Loop* loop, CFG* C, ScalarEvolution* SE) {
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "处理循环，header block_id=" << loop->getHeader()->block_id << std::endl);
    
    for (auto sub_loop : loop->getSubLoops()) {
        processLoop(sub_loop, C, SE);
    }
    
	// loop->getExitings().size() != 1，防止ret,break,continue等指令, 难以求出上下界
    if (!loop->verifySimplifyForm(C) || loop->getExits().size() != 1 || loop->getExitings().size() != 1) {
        LOOP_IDIOM_DEBUG_PRINT(std::cerr << "不是简单循环，跳过" << std::endl);
        return;
    }
    
    // 分析循环的外提信息
    LoopHoistingInfo info = analyzeLoopHoisting(loop, C, SE);
    
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "分析结果：总共 " << info.all_candidates.size() 
                                  << " 个可外提变量，可以识别memset: " 
                                  << (info.can_recognize_memset ? "是" : "否") << std::endl);
    
    // 根据分析结果决定优化策略
    // 第一步：先对非induction变量进行外提
    if (!info.other_candidates.empty()) {
        LOOP_IDIOM_DEBUG_PRINT(std::cerr << "先外提非induction变量" << std::endl);
        executePartialHoisting(loop, C, SE, info.other_candidates);
		if(info.all_hoistable){
			// 外提所有归纳变量
			LOOP_IDIOM_DEBUG_PRINT(std::cerr << "所有变量均可外提，尝试外提归纳变量" << std::endl);
			executePartialHoisting(loop, C, SE, info.induction_candidates);
		}
    }
    
    // 第二步：检查是否可以识别memset
    if (info.can_recognize_memset && info.all_hoistable) {
        LOOP_IDIOM_DEBUG_PRINT(std::cerr << "可以识别memset习语，执行memset优化" << std::endl);
        if (executeMemsetWithHoisting(loop, C, SE, info)) {
            LOOP_IDIOM_DEBUG_PRINT(std::cerr << "memset优化成功" << std::endl);
        return;
        }
    }
    
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "无法进行任何优化" << std::endl);
}


/**
 * 检查循环是否能被memset优化（只检查可行性，不实际执行）
 */
bool LoopIdiomRecognizePass::canRecognizeMemsetIdiom(Loop* loop, CFG* C, ScalarEvolution* SE) {
    // 只对最内层循环进行memset优化
    if (!loop->getSubLoops().empty()) {
        return false;
    }
    
    auto blocks = loop->getBlocks();
    
    // 查找包含store指令的基本块，并统计store指令数量
    LLVMBlock body = nullptr;
    StoreInstruction* store = nullptr;
    int store_count = 0;
    
    for (auto block : blocks) {
        if (block == loop->getHeader()) continue;
        
        for (auto inst : block->Instruction_list) {
        if (inst->GetOpcode() == BasicInstruction::STORE) {
                store_count++;
                if (!body) {
                    body = block;
            store = (StoreInstruction*)inst;
                }
            }
        }
    }
    
    // 只对包含单个store指令的循环进行memset优化
    if (store_count != 1) {
        LOOP_IDIOM_DEBUG_PRINT(std::cerr << "循环包含 " << store_count << " 个store指令，不是单个store，跳过memset优化" << std::endl);
        return false;
    }
    
    // 只对整数常量支持memset优化
    Operand store_value = store->GetValue();
    if (store_value->GetOperandType() != BasicOperand::IMMI32) {
        LOOP_IDIOM_DEBUG_PRINT(std::cerr << "store指令的值不是常量，跳过memset优化" << std::endl);
        return false;
    }
    
    // 检查常量值是否在memset支持的范围内（0x00 ~ 0xFF）
	// 八字节填充的适用范围非常有限
    int const_val = ((ImmI32Operand*)store_value)->GetIntImmVal();
    if (const_val != 0 && const_val != -1) {
        return false;
    }

    // 使用extractLoopParams获取循环参数
    LoopParams params = SE->extractLoopParams(loop, C);
    // 暂时只支持步长为1的memset；trip count 可为动态（上下界循环不变量）
	if(params.count != 0) { // 可以静态获取迭代次数, 过小的memset优化没有意义
		if (params.count < 20) { 
			return false;
		}
	}
    if (params.step_val != 1) return false;
	
    // 检查数组访问模式
    SCEV* array_scev = SE->getSimpleSCEV(store->GetPointer(), loop);
    LLVMBlock header = loop->getHeader();
    BrCondInstruction* br_cond = (BrCondInstruction*)header->Instruction_list.back();
    IcmpInstruction* icmp = (IcmpInstruction*)SE->getDef(br_cond->GetCond());

    // 确定归纳变量
    Operand induction_var = (icmp->GetOp1()->GetOperandType() == BasicOperand::REG) ? icmp->GetOp1() : icmp->GetOp2();
    
    if (!isLinearArrayAccess(array_scev, induction_var, loop, SE)) {
        return false;
    }

    // 提取GEP参数
    LLVMBlock preheader = loop->getPreheader();
    GepParams gep_params = SE->extractGepParams(array_scev, loop, preheader, C);
    if (!gep_params.base_ptr) {
        return false;
    }
    
    return true;
}



bool LoopIdiomRecognizePass::isLinearArrayAccess(SCEV* array_scev, Operand induction_var, Loop* loop, ScalarEvolution* SE) {
    // 检查数组访问是否为线性模式：base + induction_var

    if (auto* add_expr = dynamic_cast<SCEVAddExpr*>(array_scev)) {
        bool has_induction = false;
        bool has_base = false;
        
        for (SCEV* operand : add_expr->getOperands()) {
            if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(operand)) {
                if (addrec->getLoop() == loop) {
                    has_induction = true;
                }
            } else if (auto* mul_expr = dynamic_cast<SCEVMulExpr*>(operand)) {
                // 检查乘法表达式中是否包含当前循环的归纳变量
                for (SCEV* mul_operand : mul_expr->getOperands()) {
                    if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(mul_operand)) {
                        if (addrec->getLoop() == loop) {
                            has_induction = true;
                        }
                    }
                }
            } else if (dynamic_cast<SCEVUnknown*>(operand) || dynamic_cast<SCEVConstant*>(operand)) {
                has_base = true;
            }
        }
        
        return has_induction && has_base;
    }
    
    return false;
}

bool LoopIdiomRecognizePass::isArithmeticOperation(Instruction inst, BasicInstruction::LLVMIROpcode op, Operand& lhs, Operand& rhs) {
    if (auto arith = dynamic_cast<ArithmeticInstruction*>(inst)) {
        if (arith->GetOpcode() == op) {
            lhs = arith->GetOperand1();
            rhs = arith->GetOperand2();
            return true;
        }
    }
    return false;
}

void LoopIdiomRecognizePass::replaceWithMemset(Loop* loop, CFG* C, Operand array, Operand value, GepParams gep_params) {
    LLVMBlock preheader = loop->getPreheader();
    LLVMBlock exit = *loop->getExits().begin();

    ScalarEvolution* SE = C->getSCEVInfo();
    LoopParams params = SE->extractLoopParams(loop, C);

        preheader->Instruction_list.pop_back();
	LOOP_IDIOM_DEBUG_PRINT(std::cerr << "弹出br指令" << std::endl);

    // 计算动态/静态的memset字节数参数：trip_count * 4
    // 从header的icmp提取 start 与 bound（均允许为循环不变量表达式）
    LLVMBlock header = loop->getHeader();
    BrCondInstruction* br_cond = (BrCondInstruction*)header->Instruction_list.back();
    IcmpInstruction* icmp = (IcmpInstruction*)SE->getDef(br_cond->GetCond());

    SCEV* scev1 = SE->getSimpleSCEV(icmp->GetOp1(), loop);
    SCEV* scev2 = SE->getSimpleSCEV(icmp->GetOp2(), loop);

    SCEVAddRecExpr* induction_addrec = nullptr;
    SCEV* bound_scev = nullptr;
    if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(scev1); addrec && addrec->getLoop() == loop) {
        induction_addrec = addrec;
        bound_scev = scev2;
    } else if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(scev2); addrec && addrec->getLoop() == loop) {
        induction_addrec = addrec;
        bound_scev = scev1;
    }

    Operand trip_bytes_op = nullptr;
    if (induction_addrec && bound_scev) {
        // start = addrec.start; bound = expr
        Operand start_op = SE->buildExpression(induction_addrec->getStart(), preheader, C);
        Operand bound_op = SE->buildExpression(bound_scev, preheader, C);

        if (start_op && bound_op) {
            // 保证二者均为循环不变量（若为寄存器）
            bool start_ok = true, bound_ok = true;
            if (start_op->GetOperandType() == BasicOperand::REG) {
                start_ok = SE->isLoopInvariant(start_op, loop);
            }
            if (bound_op->GetOperandType() == BasicOperand::REG) {
                bound_ok = SE->isLoopInvariant(bound_op, loop);
            }
            if (start_ok && bound_ok) {
				// diff = bound - start
				Operand diff_reg = GetNewRegOperand(++C->max_reg);
				auto* subInst = new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::SUB, BasicInstruction::I32, bound_op, start_op, diff_reg);
				preheader->Instruction_list.push_back(subInst);

				Operand trip_reg = diff_reg;
				auto cmp_op = icmp->GetCond();
				if (cmp_op == BasicInstruction::IcmpCond::sle || cmp_op == BasicInstruction::IcmpCond::ule) {
					// trip = diff + 1
					Operand plus1_reg = GetNewRegOperand(++C->max_reg);
					auto* add1 = new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::ADD, BasicInstruction::I32, trip_reg, new ImmI32Operand(1), plus1_reg);
					preheader->Instruction_list.push_back(add1);
					trip_reg = plus1_reg;
				}

				// bytes = trip * 4
				Operand bytes_reg = GetNewRegOperand(++C->max_reg);
				auto* mulBytes = new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::MUL, BasicInstruction::I32, trip_reg, new ImmI32Operand(4), bytes_reg);
				preheader->Instruction_list.push_back(mulBytes);
				trip_bytes_op = bytes_reg;
            }
        }
    }

    // 生成线性GEP指令计算起始地址
    GetElementptrInstruction* gep = nullptr;
    if (gep_params.offset_op) {
        // 使用计算出的偏移量生成线性GEP指令
        gep = new GetElementptrInstruction(
            BasicInstruction::I32,
            GetNewRegOperand(++C->max_reg),
            gep_params.base_ptr,
            gep_params.offset_op,
            BasicInstruction::I32
        );
        LOOP_IDIOM_DEBUG_PRINT(std::cerr << "生成GEP指令（带偏移量）: " << gep->GetResult()->GetFullName() 
                                      << " = getelementptr i32, ptr " << gep_params.base_ptr->GetFullName() 
                                      << ", i32 " << gep_params.offset_op->GetFullName() << std::endl);
    
    } else {
        // 如果没有偏移量，直接使用基址
        gep = new GetElementptrInstruction(
            BasicInstruction::I32,
            GetNewRegOperand(++C->max_reg),
            gep_params.base_ptr,
            new ImmI32Operand(0),
            BasicInstruction::I32
        );
        LOOP_IDIOM_DEBUG_PRINT(std::cerr << "生成GEP指令（无偏移量）: " << gep->GetResult()->GetFullName() 
                                      << " = getelementptr i32, ptr " << gep_params.base_ptr->GetFullName() 
                                      << ", i32 0" << std::endl);
    
    }
    preheader->Instruction_list.push_back(gep);
    
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "GEP指令已插入到preheader block_id=" << preheader->block_id 
                                  << "，当前指令数量: " << preheader->Instruction_list.size() << std::endl);

    // 生成memset调用
    std::vector<std::pair<BasicInstruction::LLVMType, Operand>> args;
    args.push_back(std::make_pair(BasicInstruction::PTR, gep->GetResult()));
    args.push_back(std::make_pair(BasicInstruction::I8, value));
    if (trip_bytes_op) {
        args.push_back(std::make_pair(BasicInstruction::I32, trip_bytes_op));
    } else {
        // 回退到静态参数（仅当可用）
        args.push_back(std::make_pair(BasicInstruction::I32, new ImmI32Operand(params.count * 4)));
    }
    args.push_back(std::make_pair(BasicInstruction::I1, new ImmI32Operand(0)));

    CallInstruction* memset_call = new CallInstruction(BasicInstruction::VOID, nullptr, "llvm.memset.p0.i32", args);
    preheader->Instruction_list.push_back(memset_call);
    
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "memset指令已插入到preheader block_id=" << preheader->block_id 
                                  << "，当前指令数量: " << preheader->Instruction_list.size() << std::endl);
    
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "生成memset指令: call void @llvm.memset.p0.i32(ptr " << gep->GetResult()->GetFullName() 
                                  << ", i8 " << value->GetFullName() 
                                  << ", i32 " << (trip_bytes_op ? trip_bytes_op->GetFullName() : "static_size") 
                                  << ", i1 0)" << std::endl);


    // preheader -> exit
    BrUncondInstruction* br = new BrUncondInstruction(GetNewLabelOperand(exit->block_id));
    preheader->Instruction_list.push_back(br);
    
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "重新插入br指令到preheader，跳转到block_id=" << exit->block_id 
                                  << "，当前指令数量: " << preheader->Instruction_list.size() << std::endl);

}



LoopIdiomRecognizePass::LoopHoistingInfo LoopIdiomRecognizePass::analyzeLoopHoisting(Loop* loop, CFG* C, ScalarEvolution* SE) {
    LoopHoistingInfo info;
    
    // 第一步：收集所有在循环外使用的变量
    std::vector<HoistingCandidate> allCandidates = findHoistingCandidates(loop, C, SE);
	LoopParams params = SE->extractLoopParams(loop, C);
    
    // 第二步：检查哪些变量可以外提
    for (const auto& candidate : allCandidates) {
        if (canCalculateFinalValue(candidate, loop, C, SE)) {
            info.all_candidates.push_back(candidate);
            
            // 分类变量
            if (isInductionVariable(candidate.operand, loop)) {
                info.induction_candidates.push_back(candidate);
            } else {
                info.other_candidates.push_back(candidate);
            }
        }
    }
    
    // 第三步：检查是否可以识别为memset习语
    info.can_recognize_memset = canRecognizeMemsetIdiom(loop, C, SE) && params.is_simple_loop;
    
    // 第四步：判断是否所有变量都能外提
    info.all_hoistable = (info.all_candidates.size() == allCandidates.size()) && params.is_simple_loop;
    
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "分析完成：总共 " << allCandidates.size() 
                                  << " 个变量，可外提 " << info.all_candidates.size() << " 个" << std::endl);
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "induction变量 " << info.induction_candidates.size() 
                                  << " 个，其他变量 " << info.other_candidates.size() 
                                  << " 个" << std::endl);
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "可以识别memset: " << (info.can_recognize_memset ? "是" : "否") << std::endl);
    
    return info;
}



void LoopIdiomRecognizePass::executePartialHoisting(Loop* loop, CFG* C, ScalarEvolution* SE, const std::vector<HoistingCandidate>& candidates) {
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "执行部分外提优化，候选变量数量: " << candidates.size() << std::endl);
    
    LoopParams params = SE->extractLoopParams(loop, C);
    
    // 外提指定的候选变量
    for (const auto& candidate : candidates) {
		if(params.is_simple_loop){
			int finalValue = calculateFinalValue(candidate, params, loop, C, SE);
			LOOP_IDIOM_DEBUG_PRINT(std::cerr << "外提变量: " << candidate.operand->GetFullName() 
										  << "，最终值: " << finalValue << std::endl);
			hoistVariable(loop, C, candidate, finalValue);
		} else {
			LOOP_IDIOM_DEBUG_PRINT(std::cerr << "非简单循环，跳过外提" << std::endl);
		}
    }
    
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "部分外提优化完成" << std::endl);
}

bool LoopIdiomRecognizePass::executeMemsetWithHoisting(Loop* loop, CFG* C, ScalarEvolution* SE, const LoopHoistingInfo& info) {
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "执行memset优化并外提变量" << std::endl);
    
    // 由于在processLoop中已经确认了canRecognizeMemsetIdiom为true，
    // 这里可以直接查找store指令，不需要重复检查条件
    auto blocks = loop->getBlocks();
    
    // 查找包含store指令的基本块
    LLVMBlock body = nullptr;
    StoreInstruction* store = nullptr;
    
    for (auto block : blocks) {
        if (block == loop->getHeader()) continue;
        
        for (auto inst : block->Instruction_list) {
            if (inst->GetOpcode() == BasicInstruction::STORE) {
                body = block;
                store = (StoreInstruction*)inst;
                break;
            }
        }
        if (store) break;
    }
    
    // 由于canRecognizeMemsetIdiom已经确认了store指令存在且条件满足，
    assert(store && "store指令应该在canRecognizeMemsetIdiom中已经确认存在");
    
    // 提取GEP参数
    SCEV* array_scev = SE->getSimpleSCEV(store->GetPointer(), loop);
    LLVMBlock preheader = loop->getPreheader();
    GepParams gep_params = SE->extractGepParams(array_scev, loop, preheader, C);
    
    if (!gep_params.base_ptr) {
        LOOP_IDIOM_DEBUG_PRINT(std::cerr << "无法提取GEP参数" << std::endl);
        return false;
    }
    
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "提取GEP参数: base_ptr=" << gep_params.base_ptr->GetFullName() 
                                  << ", offset_op=" << (gep_params.offset_op ? gep_params.offset_op->GetFullName() : "null") << std::endl);
    
    // 执行memset优化
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "执行memset优化" << std::endl);
    replaceWithMemset(loop, C, store->GetPointer(), store->GetValue(), gep_params);
    
    // 外提induction变量
    if (!info.induction_candidates.empty()) {
        LOOP_IDIOM_DEBUG_PRINT(std::cerr << "外提induction变量" << std::endl);
        LoopParams params = SE->extractLoopParams(loop, C);
        for (const auto& candidate : info.induction_candidates) {
            int finalValue = calculateFinalValue(candidate, params, loop, C, SE);
            LOOP_IDIOM_DEBUG_PRINT(std::cerr << "外提induction变量: " << candidate.operand->GetFullName() 
                                          << "，最终值: " << finalValue << std::endl);
            hoistVariable(loop, C, candidate, finalValue);
        }
    }
    
    LOOP_IDIOM_DEBUG_PRINT(std::cerr << "memset优化并外提变量完成" << std::endl);
    return true;
}

bool LoopIdiomRecognizePass::isInductionVariable(Operand op, Loop* loop) {
    LLVMBlock header = loop->getHeader();
    for (auto inst : header->Instruction_list) {
        if (inst->GetOpcode() == BasicInstruction::ICMP) {
            auto icmp = (IcmpInstruction*)inst;
            if (icmp->GetOp1() == op || icmp->GetOp2() == op) {
                return true;
            }
        }
    }
    return false;
}



/**
 * 查找在循环外被使用的变量
 */
std::vector<HoistingCandidate> 
LoopIdiomRecognizePass::findHoistingCandidates(Loop* loop, CFG* C, ScalarEvolution* SE) {
    std::vector<HoistingCandidate> candidates;
    std::set<Operand> loopExternUses;
    
    // 查找循环外对循环内定义变量的使用
    for (auto [block_id, block] : *C->block_map) {
        if (loop->contains(block)) continue; // 跳过循环内的块
        
        for (auto inst : block->Instruction_list) {
            for (auto op : inst->GetNonResultOperands()) {
                if (op->GetOperandType() == BasicOperand::REG) {
                    // 检查这个寄存器是否在循环内定义
                    Instruction def_inst = SE->getDef(op);
                    if (def_inst) {
                        LLVMBlock def_block = C->GetBlockWithId(def_inst->GetBlockID());
                        if (def_block && loop->contains(def_block)) {
                            loopExternUses.insert(op);
                        }
                    }
                }
            }
        }
    }
    
    // 为每个在循环外使用的变量创建候选，并检查循环内使用情况
    for (auto op : loopExternUses) {
        LOOP_IDIOM_DEBUG_PRINT(std::cerr << "检查循环外使用变量: " << op->GetFullName() << std::endl);
        
        // 检查该变量在循环内是否除了自身迭代外不被使用
        if (!isVariableOnlyUsedForSelfIteration(op, loop, C)) {
            LOOP_IDIOM_DEBUG_PRINT(std::cerr << "变量 " << op->GetFullName() << " 在循环内被其他指令使用，跳过" << std::endl);
            continue;
        }
        
        SCEV* scev = SE->getSimpleSCEV(op, loop);
        if (scev) {
            LOOP_IDIOM_DEBUG_PRINT(std::cerr << "添加候选变量: " << op->GetFullName() << std::endl);
            candidates.push_back({op, scev});
        } else {
            LOOP_IDIOM_DEBUG_PRINT(std::cerr << "变量 " << op->GetFullName() << " 无法获取SCEV表达式，跳过" << std::endl);
        }
    }
    
    return candidates;
}

/**
 * 检查变量在循环内是否只用于自身迭代和循环控制
 * 对于归纳变量，只有在其他外部使用的变量都外提之后才能外提
 * 允许的使用：
 * 1. 不被循环内的其他指令使用，除了自己的phi指令
 * 2. induction变量, 虽然induction变量需要确保整个循环无效了才能外提
 */
bool LoopIdiomRecognizePass::isVariableOnlyUsedForSelfIteration(Operand op, Loop* loop, CFG* C) {
    
    // 检查该变量是否为归纳变量
    if (isInductionVariable(op, loop)) {
        return true;
    } else {
        // 对于非归纳变量，检查是否只在自身的Phi指令集合中循环调用
        // 而不被Phi指令集合之外的其他循环内变量调用
		LOOP_IDIOM_DEBUG_PRINT(std::cerr << "检查变量: " << op->GetFullName() << " 是否只用于自身迭代和循环控制" << std::endl);
        
        // 首先，找到该变量所属的Phi指令集合，并将所有op和phi指令的结果op都加入集合
        std::set<Operand> phi_set;
        // phi指令都在header中，将header中的所有phi指令的结果op也加入phi_set
        LLVMBlock header = loop->getHeader();
        for (auto inst : header->Instruction_list) {
            if (inst->GetOpcode() == BasicInstruction::PHI) {
                auto phi_inst = (PhiInstruction*)inst;
                for (auto operand : phi_inst->GetNonResultOperands()) {
                    phi_set.insert(operand);
                }
                phi_set.insert(phi_inst->GetResult());
            }
			if(phi_set.find(op) != phi_set.end()){
				break;
			}
        }
        
        // 检查变量在循环内的所有使用
        for (auto block : loop->getBlocks()) {
            for (auto inst : block->Instruction_list) {
                for (auto operand : inst->GetNonResultOperands()) {
                    if (operand == op) {
                        // 如果变量被使用，检查这个使用是否在Phi指令集合中
                        if (inst->GetOpcode() != BasicInstruction::PHI) {
                            // 检查使用该变量的指令的结果是否在Phi指令集合中
                            if (phi_set.find(inst->GetResult()) != phi_set.end()) {
                                // 如果结果在Phi指令集合中，这是允许的
                                continue;
                            } else {
                                // 如果结果不在Phi指令集合中，说明被其他计算使用
                                return false;
                            }
                        }
                    }
                }
            }
        }
        
        return true;
    }
}

/**
 * 判断是否可以计算出最终值
 */
bool LoopIdiomRecognizePass::canCalculateFinalValue(const HoistingCandidate& candidate, 
                                                   Loop* loop, CFG* C, ScalarEvolution* SE) {
    SCEV* scev = candidate.scev;
    
    // 检查是否为AddRecExpr（归纳变量）
    if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(scev)) {
        if (addrec->getLoop() != loop) return false;
        
        // 检查起始值和步长是否为常数
        if (auto* start_const = dynamic_cast<SCEVConstant*>(addrec->getStart())) {
            if (auto* step_const = dynamic_cast<SCEVConstant*>(addrec->getStep())) {
                return true; // 简单递推表达式：{start,+,step}
            }
        }
        
		// 检查是否为复杂的递推表达式，如 {0,+,{0,+,1}}（sum变量）只处理两层嵌套的情况
        SCEV* start = addrec->getStart();
        SCEV* step = addrec->getStep();
        
        if (auto* step_addrec = dynamic_cast<SCEVAddRecExpr*>(step)) {
            if (step_addrec->getLoop() == loop) {
                // 检查起始值是否为常数
                if (!dynamic_cast<SCEVConstant*>(start)) return false;
                
                // 检查步长的起始值和步长是否为常数
                if (!dynamic_cast<SCEVConstant*>(step_addrec->getStart())) return false;
                if (!dynamic_cast<SCEVConstant*>(step_addrec->getStep())) return false;
                
                // 对于嵌套的递推表达式，只要所有参数都是常数，就可以计算最终值
                return true;
            }
        }
    }
    
    return false;
}

/**
 * 计算变量的最终值
 * 支持多种习语：
 * 1. 简单递推：{0,+,1} -> i = n
 * 2. 算术级数求和：{0,+,{0,+,1}} -> sum = n*(n-1)/2
 * 3. 模运算求和：{0,+,{0,+,1}} % p -> 需要特殊处理
 * 4. 乘法习语：{1,*,{0,+,1}} -> prod = n!
 * 5. 异或求和：{0,^,{0,+,1}} -> xor_sum = 0^1^2^...^(n-1)
 */
int LoopIdiomRecognizePass::calculateFinalValue(const HoistingCandidate& candidate, 
                                               const LoopParams& params, 
                                               Loop* loop, CFG* C, ScalarEvolution* SE) {
    SCEV* scev = candidate.scev;
    
    if (auto* addrec = dynamic_cast<SCEVAddRecExpr*>(scev)) {
        if (addrec->getLoop() == loop) {
            // 简单递推表达式：{start,+,step}
            if (auto* start_const = dynamic_cast<SCEVConstant*>(addrec->getStart())) {
                if (auto* step_const = dynamic_cast<SCEVConstant*>(addrec->getStep())) {
                    int start_val = start_const->getValue()->GetIntImmVal();
                    int step_val = step_const->getValue()->GetIntImmVal();
                    
                    // 计算最终值：start + step * count
                    // 对于 while(i < n) 循环，循环结束后 i = n
                    return start_val + step_val * params.count;
                }
            }
            
            // 处理嵌套递推表达式
            SCEV* start = addrec->getStart();
            SCEV* step = addrec->getStep();
            
            // 算术级数求和：{start,+,{step_start,+,step_step}}
            if (auto* start_const = dynamic_cast<SCEVConstant*>(start)) {
                if (auto* step_addrec = dynamic_cast<SCEVAddRecExpr*>(step)) {
                    if (step_addrec->getLoop() == loop) {
                        if (auto* step_start_const = dynamic_cast<SCEVConstant*>(step_addrec->getStart())) {
                            if (auto* step_step_const = dynamic_cast<SCEVConstant*>(step_addrec->getStep())) {
                                int start_val = start_const->getValue()->GetIntImmVal();
                                int step_start = step_start_const->getValue()->GetIntImmVal();
                                int step_step = step_step_const->getValue()->GetIntImmVal();
                                
                                // 计算等差数组求和
                                // 外层：sum从start_val开始
                                // 内层：每次增加i，其中i从step_start开始，步长为step_step
                                // 使用等差数组求和公式：Sn = n * (a1 + an) / 2
                                // 其中：a1 = step_start, an = step_start + (count-1) * step_step, n = count
                                // 等差数组和 = count * (step_start + (step_start + (count-1) * step_step)) / 2
                                //            = count * (2 * step_start + (count-1) * step_step) / 2
                                //            = count * step_start + count * (count-1) * step_step / 2
                                
                                int a1 = step_start;
                                int an = step_start + (params.count - 1) * step_step;
                                int arithmetic_sum = params.count * (a1 + an) / 2;
                                
                                return start_val + arithmetic_sum;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 处理其他类型的习语（乘法、异或等）
    // 这里可以根据需要扩展
    
    return 0; // 默认值
}

/**
 * 执行变量外提
 */
void LoopIdiomRecognizePass::hoistVariable(Loop* loop, CFG* C, 
                                          const HoistingCandidate& candidate, int finalValue) {
    std::cerr << "=== 进入hoistVariable函数 ===" << std::endl;
    LLVMBlock preheader = loop->getPreheader();
    
    std::cerr << "Preheader block_id: " << preheader->block_id 
              << ", 指令数量: " << preheader->Instruction_list.size() << std::endl;
    
    // 打印preheader中的所有指令
    for (int i = 0; i < preheader->Instruction_list.size(); i++) {
        std::cerr << "  [" << i << "] ";
        preheader->Instruction_list[i]->PrintIR(std::cerr);
    }
    
    // 在preheader中插入最终值的赋值
    Operand result_reg = GetNewRegOperand(++C->max_reg);
    
    // 对于常量值，直接使用常量赋值指令
    auto* assign_inst = new ArithmeticInstruction(
        BasicInstruction::LLVMIROpcode::ADD, 
        BasicInstruction::I32, 
        new ImmI32Operand(finalValue), 
        new ImmI32Operand(0), // 加0作为赋值
        result_reg
    );
    
    // 设置指令的块ID
    assign_inst->SetBlockID(preheader->block_id);
    
    // 在preheader的最后一条指令之前插入（通常是跳转指令）
    if (!preheader->Instruction_list.empty()) {
        // 先弹出最后一条指令（通常是跳转指令）
        Instruction last_inst = preheader->Instruction_list.back();
        preheader->Instruction_list.pop_back();
        
        // 插入新的赋值指令
        preheader->Instruction_list.push_back(assign_inst);
        
        // 再把跳转指令放回去
        preheader->Instruction_list.push_back(last_inst);
        
    } else {
        preheader->Instruction_list.push_back(assign_inst);
    }
    
    // 替换循环外对该变量的使用
    loop->replaceLoopExternUses(C, candidate.operand, result_reg);
}


