#include "licm.h"
// #define running_info
// #define result_info

void LoopInvariantCodeMotionPass::Execute() {
	// store cfg info by function name.
	for(auto [defI, cfg] : llvmIR->llvm_cfg){
		std::string funcName = defI->GetFunctionName();
		cfgTable[funcName] = cfg;
    }
    for(auto [defI, cfg] : llvmIR->llvm_cfg){
        LoopInfo *loopInfo = cfg->getLoopInfo();
        DominatorTree *dt = cfg->getDomTree();
        for (auto loop : loopInfo->getTopLevelLoops()) {
            RunLICMOnLoop(loop, dt, cfg);
        }
    }
}

void LoopInvariantCodeMotionPass::getInloopDefUses(Loop* loop) {
    inloop_defs.clear();
    inloop_uses.clear(); 

    for (auto loop_bb : loop->getBlocks()) {
        for (auto inst : loop_bb->Instruction_list) {
            // 1. 记录def（指令定义的变量）
            auto resOp = inst->GetResult();
            if (resOp != nullptr) {
                inloop_defs[resOp] = inst;
            }

            // 2. 记录use（指令使用的操作数）
            for (auto op : inst->GetNonResultOperands()) {
                if (op->GetOperandType() == BasicOperand::REG) {
                    inloop_uses[op].insert(inst);
                }
            }
        }
    }
}

void LoopInvariantCodeMotionPass::updateInloopDefUses(Instruction inst) {
	// 1. 如果 inst 定义了某个寄存器/变量，从 inloop_defs 中移除
	Operand def = inst->GetResult();
	if (def && inloop_defs.count(def)) {
		inloop_defs.erase(def);
	}

	// 2. inst 可能 use 了多个操作数，需要从 inloop_uses 的每个 use 列表中移除 inst
	for (auto op : inst->GetNonResultOperands()) {
		if (inloop_uses.count(op)) {
			auto& uses = inloop_uses[op];
			uses.erase(inst);
			// 如果该 use 列表为空，也可以选择移除该 key
			if (uses.empty()) {
				inloop_uses.erase(op);
			}
		}
	}
}

void LoopInvariantCodeMotionPass::eraseBlockOnLoop(Loop* loop) {
	for(auto loop_bb : loop->getBlocks()) {
		auto tmp_Instruction_list = loop_bb->Instruction_list;
        loop_bb->Instruction_list.clear();
        for (auto I : tmp_Instruction_list) {
            if (eraseInsts.find(I) == eraseInsts.end()) 
				loop_bb->InsertInstruction(1, I);    
        }
	}

	eraseInsts.clear();
}

void LoopInvariantCodeMotionPass::getInloopWriteInsts(Loop* loop) {
    writeInsts.clear();

    for (auto bb : loop->getBlocks()) {
        for (auto inst : bb->Instruction_list) {
			if (auto store = dynamic_cast<StoreInstruction*>(inst)) {
				writeInsts.push_back(inst);
            } else if (auto call = dynamic_cast<CallInstruction*>(inst)) {
				if (cfgTable.find(call->GetFunctionName()) == cfgTable.end()) {
					writeInsts.push_back(inst);
				} else {
					auto currCfg = cfgTable[call->GetFunctionName()];
					RWInfo rwinfo = (aa->GetRWMap())[currCfg];
					if (rwinfo.WriteRoots.size() != 0) {
						writeInsts.push_back(inst);
					}
				}
			}
        }
    }
}

void LoopInvariantCodeMotionPass::updateWriteInsts(Instruction* inst) {
    // // 检查是否为store指令
    // if (dynamic_cast<StoreInstruction*>(inst)) {
    //     writeInsts.erase(std::remove(writeInsts.begin(), writeInsts.end(), inst), writeInsts.end());
    //     return;
    // }
    // // 检查是否为call指令且有写内存
    // if (auto call = dynamic_cast<CallInstruction*>(inst)) {
    //     auto callFuncName = call->GetFunctionName();
    //     if (cfgTable.find(callFuncName) == cfgTable.end()) {
    //         // 动态库call，原本就算写，直接移除
    //         writeInsts.erase(std::remove(writeInsts.begin(), writeInsts.end(), inst), writeInsts.end());
    //         return;
    //     }
    //     auto callCfg = cfgTable[callFuncName];
    //     auto& rwmap = aa->GetRWMap();
    //     auto it = rwmap.find(callCfg);
    //     if (it != rwmap.end() && !it->second.WriteRoots.empty()) {
    //         // 有写内存，移除
    //         writeInsts.erase(std::remove(writeInsts.begin(), writeInsts.end(), inst), writeInsts.end());
    //     }
    // }
}


void LoopInvariantCodeMotionPass::RunLICMOnLoop(Loop* loop, DominatorTree* dt, CFG* cfg) {
    for (auto sub : loop->getSubLoops()) {
        RunLICMOnLoop(sub, dt, cfg);
    }
	
	getInloopDefUses(loop);
	getInloopWriteInsts(loop);

    LLVMBlock preheader = loop->getPreheader();
    if (preheader == nullptr) { return; }

    // 1. 收集循环不变量候选
    std::vector<Instruction> candidates;
    for (auto bb : loop->getBlocks()) {
        bool dflag = dominatesAllExits(bb, loop, dt);
        for (auto inst : bb->Instruction_list) {
#ifdef running_info
            std::cerr << "[LICM] 分析指令: ";
            inst->PrintIR(std::cerr);
            std::cerr << "  isInvariant: " << (isInvariant(inst, loop, cfg) ? "true" : "false");
            std::cerr << ", dominatesAllExits: " << (dflag ? "true" : "false") << std::endl;
#endif
            if (isInvariant(inst, loop, cfg) && dflag) {
                candidates.push_back(inst);
            }
        }
    }

    std::stack<Instruction, std::vector<Instruction>> work(candidates);
	auto br_inst = preheader->Instruction_list.back();
	preheader->Instruction_list.pop_back();

    while (!work.empty()) {
        Instruction inst = work.top();
        work.pop();

#ifdef running_info
        std::cerr << "[LICM] 处理候选: ";
        inst->PrintIR(std::cerr);
        std::cerr << "  eraseInsts.count: " << eraseInsts.count(inst)
                  << ", isInvariant: " << isInvariant(inst, loop, cfg)
                  << ", dominatesAllExits: " << dominatesAllExits(dt->getBlockByID(inst->GetBlockID()), loop, dt)
                  << std::endl;
#endif
        if (eraseInsts.count(inst)) continue;
        if (!isInvariant(inst, loop, cfg)) continue; 
		if (!dominatesAllExits(dt->getBlockByID(inst->GetBlockID()), loop, dt)) continue;

#ifdef running_info
        std::cerr << "[LICM] 移动指令到preheader: ";
        inst->PrintIR(std::cerr);
#endif
        // 实际移动指令到 preheader, 提前把 br 指令弹出
        preheader->InsertInstruction(1, inst);
        eraseInsts.insert(inst);  // same to insts that moved.
		updateInloopDefUses(inst);
        // updateWriteInsts(inst);

        // 可能有新的候选可以处理，重新加入工作列表
        for (auto userInst : inloop_uses[inst->GetResult()]) {
#ifdef running_info
            std::cerr << "[LICM] 新增候选: ";
            userInst->PrintIR(std::cerr);
#endif
			work.push(userInst);
        }
    }

	#ifdef result_info
		if(!eraseInsts.empty()) {
			std::cerr << "loop with header[" + std::to_string(loop->getHeader()->block_id) + "] in " 
			+ cfg->function_def->GetFunctionName() + "()'s eraseInsts: " << std::endl;
			for(auto inst : eraseInsts) {
				inst->PrintIR(std::cerr);
			}
		}
	#endif

	preheader->Instruction_list.push_back(br_inst);
	eraseBlockOnLoop(loop);
}

bool LoopInvariantCodeMotionPass::isInvariant(Instruction inst, Loop* loop, CFG* cfg) {
    // 1. 忽略非计算指令
    switch(inst->GetOpcode()) {
        case BasicInstruction::LLVMIROpcode::BR_COND:
        case BasicInstruction::LLVMIROpcode::BR_UNCOND:
        case BasicInstruction::LLVMIROpcode::RET:
        case BasicInstruction::LLVMIROpcode::PHI:
            return false;
		case BasicInstruction::LLVMIROpcode::STORE:
		case BasicInstruction::LLVMIROpcode::LOAD:
		case BasicInstruction::LLVMIROpcode::CALL:
			return false;
        default:
            break;
    }

    // 2. 检查操作数是否都来自循环外或循环不变量 (可以保留内存相关指令)
    for (auto op : inst->GetNonResultOperands()) {
		if (op->GetOperandType() != BasicOperand::REG) { continue; }
        if (inloop_defs.count(op) && !eraseInsts.count(inloop_defs[op])) { return false; }
    }

    // 3. 内存操作需要额外检查
	if (inst->mayReadFromMemory() || inst->mayWriteToMemory()) {
		if (!canHoistWithAlias(inst, loop, cfg)) {
			return false;
		}
	}

    return true;
}

bool LoopInvariantCodeMotionPass::dominatesAllExits(LLVMBlock bb, Loop* loop, DominatorTree* dt) {
    for (auto exit_bb : loop->getExits()) {
        if (!dt->dominates(bb, exit_bb)) {
            return false;
        }
    }
    return true;
}

/* 前文确保了 store 的值是循环不变的
   1. 对于 store 指令， 需要保证读取的 ptr 和循环内所有写指令的 mod ptr 不产生别名
   2. 对于 load 指令， 需要保证读取的 ptr 和循环内所有写指令的 mod ptr 不产生别名
   3. 对于 call 指令，需要保证以下三点：
      3.1 非外部函数调用 (动态链接库)
	  3.2 调用的函数无副作用 （无指针写入、无循环调用）
	  3.3 loop call func, func 需要保证读入的 ptr 和 loop 内的所有写指令的 mod ptr 不产生别名
*/
bool LoopInvariantCodeMotionPass::canHoistWithAlias(Instruction inst, Loop* loop, CFG* cfg) {
    /*
    store外提判定：
    1. 存入的地址（指针）必须是循环外或循环不变量（即其定义已外提）。
    2. 存入的值必须是常量或循环不变量。
    3. 循环内所有写指令（store/call）都不能与本store的地址must-alias。
    */
    if (auto store = dynamic_cast<StoreInstruction*>(inst)) {
        Operand storePtr = store->GetPointer();
        Operand storeVal = store->GetValue();

        // (1) 存入的地址必须是循环外或循环不变量
        if (inloop_defs.count(storePtr) && !eraseInsts.count(inloop_defs[storePtr])) return false;

        // (2) 存入的值必须是常量或循环不变量
        if (storeVal->GetOperandType() == BasicOperand::REG) {
            if (inloop_defs.count(storeVal) && !eraseInsts.count(inloop_defs[storeVal])) return false;
        }

        // (3) 检查循环内所有写指令，不能有must-alias
        for (auto writeInst : writeInsts) {
            if (writeInst == inst) continue;
            Operand writePtr = nullptr;
            if (auto store2 = dynamic_cast<StoreInstruction*>(writeInst)) {
                writePtr = store2->GetPointer();
            } else if (auto call = dynamic_cast<CallInstruction*>(writeInst)) {
                // call指令的写指针集合
                auto callFuncName = call->GetFunctionName();
                if (cfgTable.find(callFuncName) != cfgTable.end()) {
                    auto callCfg = cfgTable[callFuncName];
                    auto& rwmap = aa->GetRWMap();
                    auto it = rwmap.find(callCfg);
                    if (it != rwmap.end()) {
                        for (const auto& modPtr : it->second.WriteRoots) {
                            if (aa->QueryAlias(storePtr, modPtr, cfg) == MustAlias) {
                                return false;
                            }
                        }
                    }
                }
                continue;
            }
            if (writePtr && aa->QueryAlias(storePtr, writePtr, cfg) == MustAlias) {
                return false;
            }
        }
        return true;
    }

    // 2. load指令
    if (auto load = dynamic_cast<LoadInstruction*>(inst)) {
        Operand loadPtr = load->GetPointer();
        // 遍历循环内所有写指令
        for (auto writeInst : writeInsts) {
            Operand writePtr = nullptr;
            if (auto store = dynamic_cast<StoreInstruction*>(writeInst)) {
                writePtr = store->GetPointer();
            } else if (auto call = dynamic_cast<CallInstruction*>(writeInst)) {
                // call指令的写指针集合
                auto callFuncName = call->GetFunctionName();
                if (cfgTable.find(callFuncName) != cfgTable.end()) {
                    auto callCfg = cfgTable[callFuncName];
                    auto& rwmap = aa->GetRWMap();
                    auto it = rwmap.find(callCfg);
                    if (it != rwmap.end()) {
                        for (const auto& modPtr : it->second.WriteRoots) {
                            if (aa->QueryAlias(loadPtr, modPtr, cfg) == MustAlias) {
                                return false;
                            }
                        }
                    }
                }
                continue;
            }
            if (writePtr && aa->QueryAlias(loadPtr, writePtr, cfg) == MustAlias) {
                return false;
            }
        }
        return true;
    }

    // 3. call指令
    if (auto call = dynamic_cast<CallInstruction*>(inst)) {
        auto callFuncName = call->GetFunctionName();
        // 3.1 动态链接库（找不到cfg）不做提升
        if (cfgTable.find(callFuncName) == cfgTable.end()) {
            return false;
        }
        auto callCfg = cfgTable[callFuncName];
        auto& rwmap = aa->GetRWMap();
        auto it = rwmap.find(callCfg);
        if (it == rwmap.end()) {
            return false;
        }
        const RWInfo& rwinfo = it->second;
        // 3.2 有副作用（有写）不做提升
        if (!rwinfo.WriteRoots.empty() || rwinfo.has_lib_func_call) {
            return false;
        }
        // 3.2.1 递归调用自身不做提升
        if (callCfg == cfg) {
            return false;
        }
        // 3.3 读指针和循环内所有写指针不产生别名
        for (const auto& readPtr : rwinfo.ReadRoots) {
            for (auto writeInst : writeInsts) {
                Operand writePtr = nullptr;
                if (auto store = dynamic_cast<StoreInstruction*>(writeInst)) {
                    writePtr = store->GetPointer();
                } else if (auto call2 = dynamic_cast<CallInstruction*>(writeInst)) {
                    auto call2FuncName = call2->GetFunctionName();
                    if (cfgTable.find(call2FuncName) != cfgTable.end()) {
                        auto call2Cfg = cfgTable[call2FuncName];
                        auto it2 = rwmap.find(call2Cfg);
                        if (it2 != rwmap.end()) {
                            for (const auto& modPtr : it2->second.WriteRoots) {
                                if (aa->QueryAlias(readPtr, modPtr, cfg) == MustAlias) {
                                    return false;
                                }
                            }
                        }
                    }
                    continue;
                }
                if (writePtr && aa->QueryAlias(readPtr, writePtr, cfg) == MustAlias) {
                    return false;
                }
            }
        }
        return true;
    }

    // 其他类型指令，默认不提升
    return false;
}