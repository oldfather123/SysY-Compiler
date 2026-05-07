#include "loopstrengthreduce.h"
#include <unordered_set>

// 定义LSR_SUBLOOP宏，用于控制是否启用子循环处理
#define LSR_SUBLOOP 1 // set 1 to use subloop in loopstrengthreduce

bool isInductionVariable(Operand op, Loop* loop);

// not support:
// 1. while(i > n) i /= 2 ...
// 2. while(i > n) i %= 2 ...
// 3. float loop

//#define lsr_debug 

#ifdef lsr_debug
#define LSR_DEBUG_PRINT(x) do { x; } while(0)
#else
#define LSR_DEBUG_PRINT(x) do {} while(0)
#endif

//  检查操作数是否可以被安全地移动到preheader
bool canMoveToPreheader(Operand op, LLVMBlock preheader, Loop* curLoop, std::unordered_set<Instruction>& toDelete) {
    // 如果是常量或立即数，可以直接使用
    if (dynamic_cast<ImmI32Operand*>(op)) {
        return true;
    }
    
    // 查找定义该操作数的指令
    Instruction defInst = nullptr;
    for (LLVMBlock BB : curLoop->getBlocks()) {
        for (Instruction Inst : BB->Instruction_list) {
            if (Inst->GetResult() == op) {
                defInst = Inst;
                break;
            }
        }
        if (defInst) break;
    }
    
    // 如果找不到定义，或者定义在preheader中，则安全
    if (!defInst) {
        return true; // 可能是函数参数或全局变量
    }
    
    // 检查定义是否在preheader中
    for (Instruction Inst : preheader->Instruction_list) {
        if (Inst->GetResult() == op) {
            return true;
        }
    }
    
    // 检查该操作数是否只被需要删除的指令使用
    bool onlyUsedByToDelete = true;
    for (LLVMBlock BB : curLoop->getBlocks()) {
        for (Instruction Inst : BB->Instruction_list) {
            if (toDelete.count(Inst)) continue; // 跳过要删除的指令
            
            // 检查指令的操作数
            if (auto* arith = dynamic_cast<ArithmeticInstruction*>(Inst)) {
                if (arith->GetOperand1() == op || arith->GetOperand2() == op) {
                    onlyUsedByToDelete = false;
                    break;
                }
            } else if (auto* gep = dynamic_cast<GetElementptrInstruction*>(Inst)) {
                if (gep->GetPtrVal() == op) {
                    onlyUsedByToDelete = false;
                    break;
                }
                // 检查索引操作数
                for (auto& index : gep->GetIndexes()) {
                    if (index == op) {
                        onlyUsedByToDelete = false;
                        break;
                    }
                }
                if (!onlyUsedByToDelete) break;
            } else if (auto* phi = dynamic_cast<PhiInstruction*>(Inst)) {
                for (auto& [label, val] : phi->GetPhiList()) {
                    if (val == op) {
                        onlyUsedByToDelete = false;
                        break;
                    }
                }
            }
            // 可以添加其他指令类型的检查
        }
        if (!onlyUsedByToDelete) break;
    }
    
    if (onlyUsedByToDelete) {
        // 可以将定义指令移动到preheader
        LSR_DEBUG_PRINT(std::cerr << "[LSR] 操作数 " << op->GetFullName() << " 只被要删除的指令使用，可以移动定义" << std::endl);
        return true;
    }
    
    LSR_DEBUG_PRINT(std::cerr << "[LSR] 操作数 " << op->GetFullName() << " 被其他指令使用，不能移动" << std::endl);
    return false;
}

//  递归检查SCEV表达式中所有操作数是否可以被安全移动
bool canMoveSCEVToPreheader(SCEV* expr, LLVMBlock preheader, Loop* curLoop, std::unordered_set<Instruction>& toDelete) {
    if (!expr) return true;
    
    if (auto *c = dynamic_cast<SCEVConstant*>(expr)) {
        return true;
    } else if (auto *u = dynamic_cast<SCEVUnknown*>(expr)) {
        return canMoveToPreheader(u->getValue(), preheader, curLoop, toDelete);
    } else if (auto *add = dynamic_cast<SCEVAddExpr*>(expr)) {
        for (auto *op : add->getOperands()) {
            if (!canMoveSCEVToPreheader(op, preheader, curLoop, toDelete)) {
                return false;
            }
        }
        return true;
    } else if (auto *mul = dynamic_cast<SCEVMulExpr*>(expr)) {
        for (auto *op : mul->getOperands()) {
            if (!canMoveSCEVToPreheader(op, preheader, curLoop, toDelete)) {
                return false;
            }
        }
        return true;
    }
    
    // 其他类型默认不能移动
    return false;
}

//  将操作数的定义指令移动到preheader
void moveOperandDefToPreheader(Operand op, LLVMBlock preheader, Loop* curLoop, std::unordered_set<Instruction>& toDelete) {
    // 查找定义该操作数的指令
    Instruction defInst = nullptr;
    LLVMBlock defBlock = nullptr;
    
    for (LLVMBlock BB : curLoop->getBlocks()) {
        for (auto it = BB->Instruction_list.begin(); it != BB->Instruction_list.end(); ++it) {
            if ((*it)->GetResult() == op) {
                defInst = *it;
                defBlock = BB;
                break;
            }
        }
        if (defInst) break;
    }
    
    if (defInst && defBlock) {
        // 从原位置删除
        for (auto it = defBlock->Instruction_list.begin(); it != defBlock->Instruction_list.end(); ++it) {
            if (*it == defInst) {
                defBlock->Instruction_list.erase(it);
                break;
            }
        }
        
        // 添加到preheader
        Instruction br = preheader->Instruction_list.back();
        preheader->Instruction_list.pop_back();
        preheader->Instruction_list.push_back(defInst);
        preheader->Instruction_list.push_back(br);
        
        LSR_DEBUG_PRINT(std::cerr << "[LSR] 移动指令到preheader: " << defInst->GetResult()->GetFullName() << std::endl);
    }
}

//  递归移动SCEV表达式中所有操作数的定义到preheader
void moveSCEVDefsToPreheader(SCEV* expr, LLVMBlock preheader, Loop* curLoop, std::unordered_set<Instruction>& toDelete) {
    if (!expr) return;
    
    if (auto *u = dynamic_cast<SCEVUnknown*>(expr)) {
        moveOperandDefToPreheader(u->getValue(), preheader, curLoop, toDelete);
    } else if (auto *add = dynamic_cast<SCEVAddExpr*>(expr)) {
        for (auto *op : add->getOperands()) {
            moveSCEVDefsToPreheader(op, preheader, curLoop, toDelete);
        }
    } else if (auto *mul = dynamic_cast<SCEVMulExpr*>(expr)) {
        for (auto *op : mul->getOperands()) {
            moveSCEVDefsToPreheader(op, preheader, curLoop, toDelete);
        }
    }
}

//  检查复杂递增量是否可以被安全移动到preheader
bool canMoveComplexStepToPreheader(SCEV* stepExpr, LLVMBlock preheader, Loop* curLoop, std::unordered_set<Instruction>& toDelete) {
    return canMoveSCEVToPreheader(stepExpr, preheader, curLoop, toDelete);
}

// 递归生成偏移表达式，每步都插入preheader，保证顺序
Operand buildOffsetExpr(SCEV* expr, LLVMBlock preheader, CFG* cfg, Loop* curLoop, std::unordered_set<Instruction>& toDelete) {
    Operand offsetReg = nullptr;
    if (expr) {
        if (auto *c = dynamic_cast<SCEVConstant*>(expr)) {
            offsetReg = c->getValue();
        } else if (auto *u = dynamic_cast<SCEVUnknown*>(expr)) {
            offsetReg = u->getValue();
        } else if (auto *add = dynamic_cast<SCEVAddExpr*>(expr)) {
            Operand lhs = buildOffsetExpr(add->getOperand(0), preheader, cfg, curLoop, toDelete);
            for (size_t i = 1; i < add->getNumOperands(); ++i) {
                Operand rhs = buildOffsetExpr(add->getOperand(i), preheader, cfg, curLoop, toDelete);
                Operand res = GetNewRegOperand(++cfg->max_reg);
                auto* addInst = new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::ADD, BasicInstruction::I32, lhs, rhs, res);
                preheader->Instruction_list.push_back(addInst);
                lhs = res;
            }
            offsetReg = lhs;
        } else if (auto *mul = dynamic_cast<SCEVMulExpr*>(expr)) {
            Operand lhs = buildOffsetExpr(mul->getOperand(0), preheader, cfg, curLoop, toDelete);
            for (size_t i = 1; i < mul->getNumOperands(); ++i) {
                Operand rhs = buildOffsetExpr(mul->getOperand(i), preheader, cfg, curLoop, toDelete);
                Operand res = GetNewRegOperand(++cfg->max_reg);
                auto* mulInst = new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::MUL, BasicInstruction::I32, lhs, rhs, res);
                preheader->Instruction_list.push_back(mulInst);
                lhs = res;
            }
            offsetReg = lhs;
        }
        // 其它类型...
    }
    return offsetReg;
}

//  将复杂递增量移动到preheader并返回计算结果的寄存器
Operand buildComplexStepExpr(SCEV* stepExpr, LLVMBlock preheader, CFG* cfg, Loop* curLoop, std::unordered_set<Instruction>& toDelete) {
    // 先移动所有操作数定义到preheader
    moveSCEVDefsToPreheader(stepExpr, preheader, curLoop, toDelete);
    // 然后构建表达式
    return buildOffsetExpr(stepExpr, preheader, cfg, curLoop, toDelete);
}

//  处理复杂递推式的循环强度削弱
bool handleComplexAddRecExpr(SCEVAddRecExpr* addRec, Operand res, LLVMBlock preheader, LLVMBlock header, LLVMBlock latch, CFG* cfg, Loop* curLoop, std::unordered_set<Instruction>& toDelete) {
    LSR_DEBUG_PRINT(std::cerr << "[LSR] 处理复杂递推式: "; addRec->print(std::cerr); std::cerr << std::endl);
    
    // 检查start和step是否都可以安全移动
    if (!canMoveSCEVToPreheader(addRec->getStart(), preheader, curLoop, toDelete)) {
        LSR_DEBUG_PRINT(std::cerr << "[LSR] 警告: start表达式不能被安全移动到preheader" << std::endl);
        return false;
    }
    
    if (!canMoveComplexStepToPreheader(addRec->getStep(), preheader, curLoop, toDelete)) {
        LSR_DEBUG_PRINT(std::cerr << "[LSR] 警告: step表达式不能被安全移动到preheader" << std::endl);
        return false;
    }
    
    // 先弹出跳转指令
    Instruction br = preheader->Instruction_list.back();
    preheader->Instruction_list.pop_back();
    
    // 1. 在preheader中计算start值
    Operand startReg = buildOffsetExpr(addRec->getStart(), preheader, cfg, curLoop, toDelete);
    
    // 2. 在preheader中计算step值
    Operand stepReg = buildComplexStepExpr(addRec->getStep(), preheader, cfg, curLoop, toDelete);
    
    // 重新插入跳转指令
    preheader->Instruction_list.push_back(br);
    
    // 3. 在latch中计算递增量（使用stepReg）
    Operand addStepRes = GetNewRegOperand(++cfg->max_reg);
    if (latch) {
        Instruction br = latch->Instruction_list.back();
        latch->Instruction_list.pop_back();
        auto* addStep = new ArithmeticInstruction(BasicInstruction::ADD, BasicInstruction::I32, res, stepReg, addStepRes);
        latch->Instruction_list.push_back(addStep);
        latch->Instruction_list.push_back(br);
    }
    
    // 4. 在header中插入phi
    auto* phiNew = new PhiInstruction(BasicInstruction::I32, res, std::vector<std::pair<Operand, Operand>>{
        {GetNewLabelOperand(preheader->block_id), startReg},
        {GetNewLabelOperand(latch ? latch->block_id : header->block_id), addStepRes}
    });
    header->Instruction_list.push_front(phiNew);
    
    LSR_DEBUG_PRINT(std::cerr << "[LSR] 成功处理复杂递推式" << std::endl);
    return true;
}

void LoopStrengthReducePass::Execute() {
    LSR_DEBUG_PRINT(std::cerr << "[LSR] 开始循环强度削弱优化" << std::endl);
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        LSR_DEBUG_PRINT(std::cerr << "[LSR] 处理函数: " << cfg->function_def->GetFunctionName() << std::endl);
        LoopStrengthReduce(cfg);
    }
    LSR_DEBUG_PRINT(std::cerr << "[LSR] 循环强度削弱优化完成" << std::endl);
}

//  递归判断SCEV是否完全loop invariant
bool isSCEVLoopInvariant(SCEV* expr) {
    if (auto *u = dynamic_cast<SCEVUnknown*>(expr)) {
        return u->isLoopInvariant;
    }
    if (auto *c = dynamic_cast<SCEVConstant*>(expr)) {
        return true;
    }
    if (auto *add = dynamic_cast<SCEVAddExpr*>(expr)) {
        for (auto *op : add->getOperands()) {
            if (!isSCEVLoopInvariant(op)) return false;
        }
        return true;
    }
    if (auto *mul = dynamic_cast<SCEVMulExpr*>(expr)) {
        for (auto *op : mul->getOperands()) {
            if (!isSCEVLoopInvariant(op)) return false;
        }
        return true;
    }
    // 其它类型默认不invariant
    return false;
}

// 判断op是否等价于GEP的基址
bool isGEPPtrVal(SCEV* op, GetElementptrInstruction* GEP) {
    if (auto* u = dynamic_cast<SCEVUnknown*>(op)) {
        return u->getValue() == GEP->GetPtrVal();
    }
    return false;
}

void LoopStrengthReducePass::LoopStrengthReduce(CFG* cfg) {
    // 获取SCEV分析信息
    ScalarEvolution *SE = cfg->getSCEVInfo();
    if (!SE) {
        LSR_DEBUG_PRINT(std::cerr << "[LSR] 警告: CFG没有SCEV信息，跳过" << std::endl);
        return;
    }
    LoopInfo *LI = cfg->getLoopInfo();
    if (!LI) {
        LSR_DEBUG_PRINT(std::cerr << "[LSR] 警告: CFG没有LoopInfo，跳过" << std::endl);
        return;
    }
    
    LSR_DEBUG_PRINT(std::cerr << "[LSR] 开始分析函数 " << cfg->function_def->GetFunctionName() << " 的循环" << std::endl);
    
    // 遍历所有循环
    for (auto &loop_pair : LI->getTopLevelLoops()) {
        Loop *L = loop_pair;
        LSR_DEBUG_PRINT(std::cerr << "[LSR] 发现顶层循环，header block_id=" << L->getHeader()->block_id << std::endl);
        
        // 递归处理所有循环
        std::vector<Loop *> worklist = {L};
        while (!worklist.empty()) {
            Loop *curLoop = worklist.back();
            worklist.pop_back();
            
            LSR_DEBUG_PRINT(std::cerr << "[LSR] 处理循环，header block_id=" << curLoop->getHeader()->block_id << std::endl);
            
            // 1. 收集header中归纳变量的Phi指令相关的寄存器（不需要削弱）
            std::set<Operand> noReduceSet;
            LLVMBlock header = curLoop->getHeader();
			LLVMBlock preheader = curLoop->getPreheader();
			LLVMBlock latch = nullptr;
			if (!curLoop->getLatches().empty()) latch = *curLoop->getLatches().begin();
			
            for (Instruction headerInst : header->Instruction_list) {
                if (auto *PN = dynamic_cast<PhiInstruction *>(headerInst)) {
                    // 只保护归纳变量的Phi指令
                    if (isInductionVariable(PN->GetResult(), curLoop)) {
                        // 归纳变量的Phi指令结果寄存器不需要削弱
                        noReduceSet.insert(PN->GetResult());
                        LSR_DEBUG_PRINT(std::cerr << "[LSR] 收集归纳变量Phi结果寄存器: " << PN->GetResult()->GetFullName() << std::endl);
                        // 归纳变量的Phi指令内部使用的所有寄存器也不需要削弱
                        for (auto &[label, val] : PN->GetPhiList()) {
                            noReduceSet.insert(val);
                            LSR_DEBUG_PRINT(std::cerr << "[LSR] 收集归纳变量Phi内部寄存器: " << val->GetFullName() << std::endl);
                        }
                    } else {
                        LSR_DEBUG_PRINT(std::cerr << "[LSR] 非归纳变量Phi指令: " << PN->GetResult()->GetFullName() << "，可以进行强度削弱" << std::endl);
                    }
                }
                //  Load和Call的结果也视为循环变量
                if (dynamic_cast<LoadInstruction *>(headerInst) || dynamic_cast<CallInstruction *>(headerInst)) {
                    Operand res = headerInst->GetResult();
                    if (res) {
                        noReduceSet.insert(res);
                        LSR_DEBUG_PRINT(std::cerr << "[LSR] 收集Load/Call结果寄存器: " << res->GetFullName() << std::endl);
                    }
                }
            }
            
            LSR_DEBUG_PRINT(std::cerr << "[LSR] 不需要削弱的寄存器集合大小: " << noReduceSet.size() << std::endl);
            
            // 2. 遍历循环内所有指令，寻找需要削弱的归纳变量
            std::unordered_set<Instruction> toDelete;
            for (LLVMBlock BB : curLoop->getBlocks()) {
                LSR_DEBUG_PRINT(std::cerr << "[LSR] 分析基本块 block_id=" << BB->block_id << std::endl);
                for (Instruction Inst : BB->Instruction_list) {
                    // 跳过归纳变量的Phi指令
                    if (auto *PN = dynamic_cast<PhiInstruction *>(Inst)) {
                        if (noReduceSet.find(PN->GetResult()) != noReduceSet.end()) {
                            LSR_DEBUG_PRINT(std::cerr << "[LSR] 跳过归纳变量Phi指令: " << PN->GetResult()->GetFullName() << std::endl);
                            continue;
                        } else {
                            LSR_DEBUG_PRINT(std::cerr << "[LSR] 处理非归纳变量Phi指令: " << PN->GetResult()->GetFullName() << std::endl);
                        }
                    }

                    // 跳过加法指令 (本身就不需要强度削弱)
					// 可能 c = 2 * i + 1, 就涉及加法，这种情况是需要削弱的
					// i = {0, +, 1}, c = {1, +, 2}, 虽然c的definst是加法指令，但是仍然需要削弱
                    if (dynamic_cast<ArithmeticInstruction *>(Inst)) {
                        if (Inst->GetOpcode() == BasicInstruction::ADD || Inst->GetOpcode() == BasicInstruction::SUB) {
                            LSR_DEBUG_PRINT(std::cerr << "[LSR] 跳过加法/减法指令: " << Inst->GetResult()->GetFullName() << std::endl);
                            continue;
                        }
                    }
                    
                    // 处理GEP指令
                    if (auto *GEP = dynamic_cast<GetElementptrInstruction *>(Inst)) {
                        Operand gepRes = GEP->GetResult();
                        SCEV *gepSCEV = SE->getSCEV(gepRes, curLoop);
                        SCEV *simplifiedGepSCEV = SE->simplify(gepSCEV);
						// simplifiedGepSCEV = SE->fixLoopInvariantUnknowns(simplifiedGepSCEV, curLoop);
						// simplifiedGepSCEV = SE->simplify(simplifiedGepSCEV);
                        LSR_DEBUG_PRINT(std::cerr << "[LSR] GEP结果SCEV: "; if(simplifiedGepSCEV) simplifiedGepSCEV->print(std::cerr); std::cerr << std::endl);
                        
                        SCEVAddExpr* addExpr = nullptr;
                        if ((addExpr = dynamic_cast<SCEVAddExpr*>(simplifiedGepSCEV))) {
                            SCEVAddRecExpr *addRec = nullptr;
                            bool allBaseInvariant = true;
                            for (auto *op : addExpr->getOperands()) {
                                if (op->getType() == scAddRecExpr) addRec = static_cast<SCEVAddRecExpr*>(op);
                                else {
                                    if (!isSCEVLoopInvariant(op)) {
                                        allBaseInvariant = false;
                                        LSR_DEBUG_PRINT(std::cerr << "[LSR] 不能合并到base: "; op->print(std::cerr); std::cerr << std::endl;);
                                    }
                                }
                            }
                            // addRec为归纳变量，baseOffsetExpr为所有非invariant项
                            if (addRec && allBaseInvariant) { 
                                // 1. 递归生成偏移表达式，包括归纳变量的起始值
                                SCEV* baseOffsetExpr = nullptr;
                                for (auto *op : addExpr->getOperands()) {
                                    if (op->getType() != scAddRecExpr && !isGEPPtrVal(op, GEP)) {
                                        if (!baseOffsetExpr) baseOffsetExpr = op;
                                        else baseOffsetExpr = new SCEVAddExpr({baseOffsetExpr, op});
                                    }
                                }
                                // 将归纳变量的起始值也加入到baseOffsetExpr中
                                if (addRec->getStart()) {
                                    if (!baseOffsetExpr) baseOffsetExpr = addRec->getStart();
                                    else baseOffsetExpr = new SCEVAddExpr({baseOffsetExpr, addRec->getStart()});
                                }
								LSR_DEBUG_PRINT(
								    std::cerr << "[LSR] baseOffsetExpr: ";
								    if (baseOffsetExpr) baseOffsetExpr->print(std::cerr);
								    else std::cerr << "(null)";
								    std::cerr << std::endl;
								);
                                
                                //  检查baseOffsetExpr中的所有操作数是否可以被安全移动到preheader
                                if (!canMoveSCEVToPreheader(baseOffsetExpr, preheader, curLoop, toDelete)) {
                                    LSR_DEBUG_PRINT(std::cerr << "[LSR] 警告: baseOffsetExpr中的操作数不能被安全移动到preheader，跳过GEP优化" << std::endl);
                                    continue;
                                }
                                
                                //  将baseOffsetExpr中的操作数定义移动到preheader
                                moveSCEVDefsToPreheader(baseOffsetExpr, preheader, curLoop, toDelete);
                                
                                // 检查是否为简单递推式（step是常量）
                                if (auto* stepC = dynamic_cast<SCEVConstant*>(addRec->getStep())) {
                                    int step = stepC->getValue()->GetIntImmVal();
                                    // 获取start值
                                    Operand startVal = nullptr;
                                    if (auto* c = dynamic_cast<SCEVConstant*>(addRec->getStart())) {
                                        startVal = c->getValue();
                                    } else if (auto* u = dynamic_cast<SCEVUnknown*>(addRec->getStart())) {
                                        startVal = u->getValue();
                                    }
                                    
                                    // 2. 生成偏移reg，保证顺序：先插入add/mul链，再插入GEP
                                    Instruction br = preheader->Instruction_list.back();
                                    preheader->Instruction_list.pop_back();
                                    Operand offsetReg = buildOffsetExpr(baseOffsetExpr, preheader, cfg, curLoop, toDelete);
                                    Operand gepInitRes = GetNewRegOperand(++cfg->max_reg);
                                    auto* gepInit = new GetElementptrInstruction(GEP->GetType(), gepInitRes, GEP->GetPtrVal(), offsetReg, BasicInstruction::I32);
                                    preheader->Instruction_list.push_back(gepInit);
                                    preheader->Instruction_list.push_back(br);
                                    // 4. latch插入步进GEP（新reg）
                                    Operand gepStepRes = GetNewRegOperand(++cfg->max_reg);
                                    if (latch) {
                                        br = latch->Instruction_list.back();
                                        latch->Instruction_list.pop_back();
                                        auto* gepStep = new GetElementptrInstruction(GEP->GetType(), gepStepRes, gepRes, new ImmI32Operand(step), BasicInstruction::I32);
                                        latch->Instruction_list.push_back(gepStep);
                                        latch->Instruction_list.push_back(br);
                                    }
                                    // 5. header插入phi（用原reg）
                                    auto* gepPhi = new PhiInstruction(BasicInstruction::PTR, gepRes, std::vector<std::pair<Operand, Operand>>{
                                        {GetNewLabelOperand(preheader->block_id), gepInitRes},
                                        {GetNewLabelOperand(latch ? latch->block_id : header->block_id), gepStepRes}
                                    });
                                    header->Instruction_list.push_front(gepPhi);
                                    // 标记原GEP指令待删除
                                    toDelete.insert(GEP);
                                } else {
                                    // 处理复杂递推式（step不是常量）
                                    LSR_DEBUG_PRINT(std::cerr << "[LSR] GEP发现复杂递推式，尝试处理" << std::endl);
                                    
                                    // 检查step是否可以被安全移动
                                    if (!canMoveComplexStepToPreheader(addRec->getStep(), preheader, curLoop, toDelete)) {
                                        LSR_DEBUG_PRINT(std::cerr << "[LSR] 警告: GEP的step表达式不能被安全移动到preheader，跳过GEP优化" << std::endl);
                                        continue;
                                    }
                                    
                                    // 1. 在preheader中计算baseOffsetExpr
                                    Instruction br = preheader->Instruction_list.back();
                                    preheader->Instruction_list.pop_back();
                                    Operand offsetReg = buildOffsetExpr(baseOffsetExpr, preheader, cfg, curLoop, toDelete);
                                    Operand gepInitRes = GetNewRegOperand(++cfg->max_reg);
                                    auto* gepInit = new GetElementptrInstruction(GEP->GetType(), gepInitRes, GEP->GetPtrVal(), offsetReg, BasicInstruction::I32);
                                    preheader->Instruction_list.push_back(gepInit);
                                    preheader->Instruction_list.push_back(br);
                                    
                                    // 2. 在preheader中计算step值
                                    Operand stepReg = buildComplexStepExpr(addRec->getStep(), preheader, cfg, curLoop, toDelete);
                                    
                                    // 3. 在latch中计算递增量（使用stepReg）
                                    Operand gepStepRes = GetNewRegOperand(++cfg->max_reg);
                                    if (latch) {
                                        br = latch->Instruction_list.back();
                                        latch->Instruction_list.pop_back();
                                        auto* gepStep = new GetElementptrInstruction(GEP->GetType(), gepStepRes, gepRes, stepReg, BasicInstruction::I32);
                                        latch->Instruction_list.push_back(gepStep);
                                        latch->Instruction_list.push_back(br);
                                    }
                                    
                                    // 4. header插入phi（用原reg）
                                    auto* gepPhi = new PhiInstruction(BasicInstruction::PTR, gepRes, std::vector<std::pair<Operand, Operand>>{
                                        {GetNewLabelOperand(preheader->block_id), gepInitRes},
                                        {GetNewLabelOperand(latch ? latch->block_id : header->block_id), gepStepRes}
                                    });
                                    header->Instruction_list.push_front(gepPhi);
                                    
                                    // 标记原GEP指令待删除
                                    toDelete.insert(GEP);
                                }
                            } else {
                                continue;
                            }
                        } else if (auto *ar = dynamic_cast<SCEVAddRecExpr*>(simplifiedGepSCEV)) {
                            SCEVAddRecExpr* addRec = ar;
                            // 暂不支持该结构的strength reduction，直接continue
                            continue;
                        } else {
                            continue;
                        }
                        continue;
                    }
                    
                    // 检查非PHI指令的结果是否为归纳变量且需要削弱
                    if (Inst->GetResult() && noReduceSet.find(Inst->GetResult()) == noReduceSet.end()) {
                        Operand res = Inst->GetResult();
                        SCEV *resSCEV = SE->getSCEV(res, curLoop);
                        SCEV *simplifiedResSCEV = SE->simplify(resSCEV);
						// simplifiedResSCEV = SE->fixLoopInvariantUnknowns(simplifiedResSCEV, curLoop);
						// simplifiedResSCEV = SE->simplify(simplifiedResSCEV);
                        LSR_DEBUG_PRINT(std::cerr << "[LSR] 指令结果 " << res->GetFullName() << " 的SCEV: "; if(simplifiedResSCEV) simplifiedResSCEV->print(std::cerr); std::cerr << std::endl);
                        
                        if (simplifiedResSCEV->getType() == scAddRecExpr) {
                            // 这是一个需要削弱的归纳变量
                            auto* addRec = static_cast<SCEVAddRecExpr*>(simplifiedResSCEV);
                            LSR_DEBUG_PRINT(std::cerr << "[LSR] 发现非PHI归纳变量: " << res->GetFullName() << std::endl);
                            
                            LLVMBlock preheader = curLoop->getPreheader();
                            LLVMBlock latch = nullptr;
                            if (!curLoop->getLatches().empty()) latch = *curLoop->getLatches().begin();
                            
                            // 检查是否为简单递推式（step是常量）
                            if (auto* stepC = dynamic_cast<SCEVConstant*>(addRec->getStep())) {
                                int step = stepC->getValue()->GetIntImmVal();
                                Operand startVal = nullptr;
                                if (auto* c = dynamic_cast<SCEVConstant*>(addRec->getStart())) {
                                    startVal = c->getValue();
                                    LSR_DEBUG_PRINT(std::cerr << "[LSR] 使用常量start: " << startVal->GetFullName() << std::endl);
                                } else if (auto* u = dynamic_cast<SCEVUnknown*>(addRec->getStart())) {
                                    startVal = u->getValue();
                                    LSR_DEBUG_PRINT(std::cerr << "[LSR] 使用未知start: " << startVal->GetFullName() << std::endl);
                                }
                                if (startVal) {
                                    //  检查startVal是否可以被安全移动到preheader
                                    if (!canMoveToPreheader(startVal, preheader, curLoop, toDelete)) {
                                        LSR_DEBUG_PRINT(std::cerr << "[LSR] 警告: startVal " << startVal->GetFullName() << " 不能被安全移动到preheader，跳过归纳变量优化" << std::endl);
                                        continue;
                                    }
                                    
                                    //  将startVal的定义移动到preheader
                                    moveOperandDefToPreheader(startVal, preheader, curLoop, toDelete);
                                    
                                    // 1. preheader插入初值（新reg）
                                    Operand addInitRes = GetNewRegOperand(++cfg->max_reg);
                                    Instruction br = preheader->Instruction_list.back();
                                    preheader->Instruction_list.pop_back();
                                    auto* addInit = new ArithmeticInstruction(BasicInstruction::ADD, BasicInstruction::I32, startVal, new ImmI32Operand(0), addInitRes);
                                    preheader->Instruction_list.push_back(addInit);
                                    preheader->Instruction_list.push_back(br);
                                    // 2. latch插入add（新reg）
                                    Operand addStepRes = GetNewRegOperand(++cfg->max_reg);
                                    if (latch) {
                                        br = latch->Instruction_list.back();
                                        latch->Instruction_list.pop_back();
                                        auto* addStep = new ArithmeticInstruction(BasicInstruction::ADD, BasicInstruction::I32, res, new ImmI32Operand(step), addStepRes);
                                        latch->Instruction_list.push_back(addStep);
                                        latch->Instruction_list.push_back(br);
                                    }
                                    // 3. header插入phi（用原reg）
                                    auto* phiNew = new PhiInstruction(BasicInstruction::I32, res, std::vector<std::pair<Operand, Operand>>{
                                        {GetNewLabelOperand(preheader->block_id), addInitRes},
                                        {GetNewLabelOperand(latch ? latch->block_id : header->block_id), addStepRes}
                                    });
                                    header->Instruction_list.push_front(phiNew);
                                    // 替换原指令结果
                                    Inst->SetResult(res);
                                    // 标记原归纳变量指令待删除
                                    toDelete.insert(Inst);
                                } else {
                                    LSR_DEBUG_PRINT(std::cerr << "[LSR] 警告: 无法获取归纳变量的start值" << std::endl);
                                }
                            } else {
                                // 处理复杂递推式（step不是常量）
                                LSR_DEBUG_PRINT(std::cerr << "[LSR] 发现复杂递推式，尝试处理" << std::endl);
                                if (handleComplexAddRecExpr(addRec, res, preheader, header, latch, cfg, curLoop, toDelete)) {
                                    // 替换原指令结果
                                    Inst->SetResult(res);
                                    // 标记原归纳变量指令待删除
                                    toDelete.insert(Inst);
                                } else {
                                    LSR_DEBUG_PRINT(std::cerr << "[LSR] 警告: 无法处理复杂递推式" << std::endl);
                                }
                            }
                        }
                    }
                }
            }
            // 3. 批量删除所有待删除指令
            for (LLVMBlock BB : curLoop->getBlocks()) {
                for (auto it = BB->Instruction_list.begin(); it != BB->Instruction_list.end(); ) {
                    if (toDelete.count(*it)) it = BB->Instruction_list.erase(it);
                    else ++it;
                }
            }
            
			#if LSR_SUBLOOP == 1
            // 递归处理子循环
            for (auto &subloop_pair : curLoop->getSubLoops()) {
                LSR_DEBUG_PRINT(std::cerr << "[LSR] 发现子循环，加入工作队列" << std::endl);
                worklist.push_back(subloop_pair);
            }
			#endif
        }
    }
}

bool isInductionVariable(Operand op, Loop* loop) {
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

/*
GEP指令的强度削弱分为两部分：
    - 中端：识别模式并转换为getelementptr ... i32 1格式
    - 后端：识别getelementptr ... i32 1格式并优化为addi指令
仅考虑两类：
    1. gep指令的indexes完全由立即数组成
    2. gep指令的indexes仅最后一位是RegOperand，其余均为立即数
*/
int GepIndexTypeIdentify(GetElementptrInstruction* inst){//0: 不属于任何类型  1：属于类型1  2：属于类型2
    auto dims=inst->GetDims();
    auto indexes=inst->GetIndexes();
    int size=indexes.size();
    if(dims.size()!=(size-1)){
        return 0;
    }
    // 1类型最短长度：dims:1  indexes 2
    // 2类型最短长度: dims:0  indexes 1

    for(int i=0;i<size;i++){
        if(i!=(size-1)&&indexes[i]->GetOperandType()!=BasicOperand::IMMI32){
            return 0;
        }else if(i==(size-1)){
            if(indexes[i]->GetOperandType()==BasicOperand::IMMI32){
                if(size>1){return 1;}
                return 0;
            }else if(indexes[i]->GetOperandType()==BasicOperand::REG){
                return 2;
            }
        }
    }
    return 0;
}

struct ImmGepInfo{
    GetElementptrInstruction* gep;
    int index;//根据gep的indexes计算出的总偏移
    int res_regno;
    int block_id;
    ImmGepInfo(GetElementptrInstruction* &inst, int blockid):gep(inst),block_id(blockid){
        res_regno=gep->GetDefRegno();
        index=gep->ComputeIndex();
    }
};
struct RegGepInfo{
    GetElementptrInstruction* gep;
    Instruction def_inst;//gep指令最后一个reg_index的定义指令
    int index_regno;//gep指令indexes中最后一个RegOperand的regno
    int res_regno;  //gep指令的res
    int block_id;   //gep指令所在blockid
    bool changed;
    RegGepInfo(GetElementptrInstruction* &inst, Instruction def,int index,int blockid):
                gep(inst),def_inst(def),index_regno(index),block_id(blockid),changed(false){
        res_regno=gep->GetDefRegno();
    }
};

bool isNearbyGeps(RegGepInfo* info1, RegGepInfo* info2){
    auto inst1=info1->gep;
    auto inst2=info2->gep;
    //[1]判断前面的imm_indexes是否完全一致
    auto indexes1=inst1->GetIndexes();
    auto indexes2=inst2->GetIndexes();
    if(indexes1.size()!=indexes2.size()){
        return false;
    }
    int size=indexes1.size();
    for(auto i=0;i<size-1;i++){
        if(((ImmI32Operand*)indexes1[i])->GetIntImmVal()!=((ImmI32Operand*)indexes2[i])->GetIntImmVal()){
            return false;
        }
    }
    //[2]判断最后一个reg_index是否相邻
    //[2.1]直接相邻 + 
    if(info2->def_inst->GetOpcode()==BasicInstruction::ADD){
        ArithmeticInstruction* def_inst=(ArithmeticInstruction*)info2->def_inst;
        if(def_inst->GetOperand1()->GetOperandType()==BasicOperand::REG //inst2的reg_index是在inst1的reg_index上偏移而来
           &&def_inst->GetOperand2()->GetOperandType()==BasicOperand::IMMI32
           &&((RegOperand*)def_inst->GetOperand1())->GetRegNo()==info1->index_regno){
            
            return true;
        }
    }
    //[2.2]间接相邻 - 

    return false;
}


void LoopStrengthReducePass::GepStrengthReduce(){
    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        std::unordered_map<std::string,std::vector<ImmGepInfo*>>imm_geps;
        std::unordered_map<std::string,std::vector<RegGepInfo*>>reg_geps;

        //【1】收集gep指令信息
        for(auto &[id,block]:*(cfg->block_map)){
            for(auto &inst:block->Instruction_list){
                if(inst->GetOpcode()==BasicInstruction::GETELEMENTPTR){
                    auto gep=(GetElementptrInstruction*)inst;
                    int type=GepIndexTypeIdentify(gep);
                    if(type==1){
                        auto info=new ImmGepInfo(gep,id);
                        imm_geps[gep->GetPtrVal()->GetFullName()].push_back(info);
                    }else if(type==2){
                        auto indexes=gep->GetIndexes();
                        Operand reg_index=indexes[indexes.size()-1];
                        int index_regno=((RegOperand*)reg_index)->GetRegNo();
                        Instruction def_inst=cfg->def_map[index_regno];
                        if(def_inst==nullptr){continue;}//可能是函数参数，一定不会满足我们的要求
                        auto info=new RegGepInfo(gep,def_inst,index_regno,id);
                        reg_geps[gep->GetPtrVal()->GetFullName()].push_back(info);
                    }
                }
            }
        }

        //【2】进行强度削弱
        //【2.1】imm类型
        for(auto &[ptrname,infos]:imm_geps){
            //std::cout<<"------- Ptr: "<<ptrname<<" ------------ "<<std::endl;
            for(int i=1;i<infos.size();i++){
                int gap=infos[i]->index-infos[i-1]->index;
                //std::cout<<"gap = "<<gap<<std::endl;
                if(gap>0){
                    
                    if(((DominatorTree*)cfg->DomTree)->dominates(infos[i-1]->block_id,infos[i]->block_id)){
                        auto inst = new GetElementptrInstruction(infos[i]->gep->GetType(),GetNewRegOperand(infos[i]->res_regno),
                                                    GetNewRegOperand(infos[i-1]->res_regno),new ImmI32Operand(gap),BasicInstruction::LLVMType::I32);
                        infos[i]->gep=inst;
                        //std::cout<<"replaced!"<<std::endl;
                    }
                }
            }
        }
        //【2.2】reg类型
        std::unordered_set<int> index_regno_todlt;
        for(auto &[ptrname,infos]:reg_geps){
            //std::cout<<"------- Ptr: "<<ptrname<<" ------------ "<<std::endl;
            for(int i=0;i<infos.size()-1;i++){
                if(infos[i]->changed){continue;}
                for(int j=i+1;j<infos.size();j++){
                    if (!((DominatorTree*)cfg->DomTree)->dominates(infos[i]->block_id, infos[j]->block_id)){continue;}//新增
                    if(isNearbyGeps(infos[i],infos[j])){
                        int gap=((ImmI32Operand*)((ArithmeticInstruction*)infos[j]->def_inst)->GetOperand2())->GetIntImmVal();
                        //std::cout<<"gap = "<<gap<<std::endl;
                        auto inst = new GetElementptrInstruction(infos[j]->gep->GetType(),GetNewRegOperand(infos[j]->res_regno),
                                                    GetNewRegOperand(infos[i]->res_regno),new ImmI32Operand(gap),BasicInstruction::LLVMType::I32);
                        infos[j]->gep=inst;
                        infos[j]->changed=true;
                        int def_regno=infos[j]->def_inst->GetDefRegno();
                        if(cfg->use_map[def_regno].size()==1){//若此def指令定义的reg仅这一处使用，那么可删除此指令
                            index_regno_todlt.insert(infos[j]->def_inst->GetDefRegno());
                        }
                        //std::cout<<"replaced!"<<std::endl;
                    }
                }
            }
        }

        //【3】指令替换
        for(auto &[id,block]:*(cfg->block_map)){
            for(auto it=block->Instruction_list.begin();it!=block->Instruction_list.end();){
                auto &inst=*it;
                if(inst->GetOpcode()==BasicInstruction::GETELEMENTPTR){
                    Operand ptr=((GetElementptrInstruction*)inst)->GetPtrVal();
                    int type=GepIndexTypeIdentify((GetElementptrInstruction*)inst);
                    if(type==1){
                        for(auto &info:imm_geps[ptr->GetFullName()]){
                            if(info->res_regno==((GetElementptrInstruction*)inst)->GetDefRegno()){
                                assert(info->gep!=nullptr);
                                inst = info->gep;
                                //std::cout<<"the inst with res_regno "<<info->res_regno<<" is replaced!"<<std::endl;
                                break;
                            }
                        }
                    }else if(type==2){
                        for(auto &info:reg_geps[ptr->GetFullName()]){
                            if(info->res_regno==((GetElementptrInstruction*)inst)->GetDefRegno()){
                                inst = info->gep;
                                //std::cout<<"the inst with res_regno "<<info->res_regno<<" is replaced!"<<std::endl;
                                break;
                            }
                        }
                    }
                }else if(inst->GetOpcode()==BasicInstruction::ADD){ //删除偏移计算指令
                    if(index_regno_todlt.count(inst->GetDefRegno())){
                        it=block->Instruction_list.erase(it);
                        continue;
                    }
                }
                ++it;
            }
        }
    }
    return ;
}