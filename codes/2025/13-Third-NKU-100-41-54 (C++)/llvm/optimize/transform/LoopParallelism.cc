#include "LoopParallelism.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#define LOOP_PARALLELISM_DEBUG 1

#if LOOP_PARALLELISM_DEBUG
#define PARALLEL_DEBUG(x) do { x; } while(0)
#else
#define PARALLEL_DEBUG(x) do {} while(0)
#endif

extern std::map<CFG *, DominatorTree *> DomInfo;

void LoopParallelismPass::Execute() {
    PARALLEL_DEBUG(std::cout << "=== 开始自动并行化Pass ===" << std::endl);

    AddRuntimeLibraryDeclarations();

    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        PARALLEL_DEBUG(std::cout << "处理函数: " << cfg->function_def->GetFunctionName() << std::endl);
        
        ProcessFunction(cfg);
        
        // 重新构建CFG
        cfg->BuildCFG();
        cfg->getDomTree()->BuildDominatorTree();
    }
    
    PARALLEL_DEBUG(std::cout << "=== 自动并行化Pass完成 ===" << std::endl);
}

void LoopParallelismPass::ProcessFunction(CFG* cfg) {
    auto loop_info = cfg->getLoopInfo();
    if (!loop_info) return;
    
    ScalarEvolution* SE = cfg->getSCEVInfo();
    if (!SE) return;
    
    for (auto loop : loop_info->getTopLevelLoops()) {
        PARALLEL_DEBUG(std::cout << "分析循环，header block_id=" << loop->getHeader()->block_id << std::endl);
        
        ProcessLoop(loop, cfg, SE);
    }
}

void LoopParallelismPass::ProcessLoop(Loop* loop, CFG* cfg, ScalarEvolution* SE) {
    // 首先检查当前循环是否可以并行化
    if (CanParallelizeLoop(loop, cfg, SE)) {
        PARALLEL_DEBUG(std::cout << "开始并行化循环" << std::endl);
        
        // 提取循环体到函数
        ExtractLoopBodyToFunction(loop, cfg, SE);
        
        // 如果外层循环可以并行化，就不处理内层循环
        return;
    } else {
        PARALLEL_DEBUG(std::cout << "循环不可并行化" << std::endl);
        
        // 如果外层循环不能并行化，则递归处理子循环
        for (auto sub_loop : loop->getSubLoops()) {
            ProcessLoop(sub_loop, cfg, SE);
        }
    }
}

bool LoopParallelismPass::CanParallelizeLoop(Loop* loop, CFG* cfg, ScalarEvolution* SE) {
    // 检查是否是简单循环
    if (!IsSimpleLoop(loop, cfg)) {
        PARALLEL_DEBUG(std::cout << "不是简单循环" << std::endl);
        return false;
    }
    
    // 检查是否有循环携带依赖
    if (!loop_dependence_analyser || loop_dependence_analyser->getLoopDependenceResult(loop)) {
        PARALLEL_DEBUG(std::cout << "存在循环携带依赖" << std::endl);
        return false;
    }
    
	// 检查迭代次数是否为常数或者循环不变量
    if (!IsConstantIterationCount(loop, cfg, SE)) {
        PARALLEL_DEBUG(std::cout << "迭代次数不是常数" << std::endl);
        return false;
    }
    
    // 检查循环外是否使用了循环内定义的寄存器
    if (HasLoopExternalUses(loop, cfg)) {
        PARALLEL_DEBUG(std::cout << "循环外使用了循环内的寄存器" << std::endl);
        return false;
    }

    return true;
}

bool LoopParallelismPass::IsSimpleLoop(Loop* loop, CFG* cfg) {
    // 检查循环出口是否唯一
    if (loop->getExits().size() != 1 || loop->getExitings().size() != 1) {
        return false;
    }
    
    // 检查循环头部是否是条件跳转
    LLVMBlock header = loop->getHeader();
    if (header->Instruction_list.empty()) return false;
    
    Instruction last_inst = header->Instruction_list.back();
    if (last_inst->GetOpcode() != BasicInstruction::BR_COND) {
        return false;
    }
    
    // 检查循环条件是否是简单比较
    BrCondInstruction* br_cond = (BrCondInstruction*)last_inst;
    Operand cond = br_cond->GetCond();
    
    Instruction cond_inst = nullptr;
    if (cond->GetOperandType() == BasicOperand::REG) {
        int regno = ((RegOperand*)cond)->GetRegNo();
        cond_inst = cfg->def_map[regno];
    }
    if (!cond_inst || cond_inst->GetOpcode() != BasicInstruction::ICMP) {
        return false;
    }
    
    return true;
}



bool LoopParallelismPass::IsConstantIterationCount(Loop* loop, CFG* cfg, ScalarEvolution* SE) {
    // 使用extractLoopParams来检查循环参数
    auto loop_params = SE->extractLoopParams(loop, cfg);
    
    // 检查是否为简单循环且迭代次数为常数
    return loop_params.is_simple_loop && loop_params.count > 0;
}

void LoopParallelismPass::ExtractLoopBodyToFunction(Loop* loop, CFG* cfg, ScalarEvolution* SE) {
    std::string func_name = GenerateFunctionName(cfg, loop);
    
	CollectLoopExternalVariables(loop, cfg);
    CreateParallelFunction(loop, cfg, func_name, SE);
    CreateConditionalParallelization(loop, cfg, func_name, SE);
}

std::string LoopParallelismPass::GenerateFunctionName(CFG* cfg, Loop* loop) {
    std::string base_name = "___parallel_loop_" + cfg->function_def->GetFunctionName() + "_" + 
                           std::to_string(loop->getHeader()->block_id);
    
    return GenerateUniqueName(base_name);
}

std::string LoopParallelismPass::GenerateUniqueName(const std::string& base_name) {
    if (name_counter.find(base_name) == name_counter.end()) {
        name_counter[base_name] = 0;
    }
    
    std::stringstream ss;
    ss << base_name << "_" << std::setw(3) << std::setfill('0') << name_counter[base_name]++;
    
    return ss.str();
}

void LoopParallelismPass::CreateParallelFunction(Loop* loop, CFG* cfg, const std::string& func_name, ScalarEvolution* SE) {
    auto func_def = new FunctionDefineInstruction(BasicInstruction::LLVMType::VOID, func_name);
    
    // 添加函数参数：void* args
    func_def->InsertFormal(BasicInstruction::LLVMType::PTR);
    
    llvmIR->NewFunction(func_def);
	llvmIR->function_max_reg[func_def] = 0;  // ptr arg is %r0
	llvmIR->function_max_label[func_def] = -1;
	llvmIR->FunctionNameTable[func_name] = func_def;
    
    std::map<int, int> reg_mapping;
    
    AddFunctionParameters(func_def, reg_mapping);
    AddThreadRangeCalculation(func_def, reg_mapping);
	CopyLoopBodyInstructions(loop, func_def, reg_mapping);
	BuildCFGforParallelFunction(func_def);  
    
    PARALLEL_DEBUG(std::cout << "创建并行化函数: " << func_name << std::endl);
}

void LoopParallelismPass::AddFunctionParameters(FunctionDefineInstruction* func_def, std::map<int, int>& reg_mapping) {
    // 解析void* args参数
    // args[0] = thread_id, args[1] = start, args[2] = end

    // 创建寄存器来存储解析的参数
	int& max_reg = llvmIR->function_max_reg[func_def];
	int& max_label = llvmIR->function_max_label[func_def];
    auto thread_id_gep_reg = GetNewRegOperand(++max_reg);
	auto thread_id_reg = GetNewRegOperand(++max_reg);
	thread_id_regNo = max_reg;
	auto start_gep_reg = GetNewRegOperand(++max_reg);
    auto start_reg = GetNewRegOperand(++max_reg);
	start_regNo = max_reg;
	auto end_gep_reg = GetNewRegOperand(++max_reg);
    auto end_reg = GetNewRegOperand(++max_reg);
	end_regNo = max_reg;
	
	auto func_block = llvmIR->NewBlock(func_def, ++max_label);
    
    // 获取函数参数
    auto args_param = func_def->GetNonResultOperands()[0];
    
    // 创建参数解析指令
    // thread_id = ((int*)args)[0]
    auto thread_id_gep = new GetElementptrInstruction(
        BasicInstruction::LLVMType::I32, thread_id_gep_reg, args_param, BasicInstruction::LLVMType::I32);
    thread_id_gep->push_idx_imm32(0);
    func_block->Instruction_list.push_back(thread_id_gep);
    
    auto thread_id_load = new LoadInstruction(BasicInstruction::LLVMType::I32, thread_id_gep_reg, thread_id_reg);
    func_block->Instruction_list.push_back(thread_id_load);
    
    // start = ((int*)args)[1]
    auto start_gep = new GetElementptrInstruction(
        BasicInstruction::LLVMType::I32, start_gep_reg, args_param, BasicInstruction::LLVMType::I32);
    start_gep->push_idx_imm32(1);
    func_block->Instruction_list.push_back(start_gep);
    
    auto start_load = new LoadInstruction(BasicInstruction::LLVMType::I32, start_gep_reg, start_reg);
    func_block->Instruction_list.push_back(start_load);
    
    // end = ((int*)args)[2]
    auto end_gep = new GetElementptrInstruction(
        BasicInstruction::LLVMType::I32, end_gep_reg, args_param, BasicInstruction::LLVMType::I32);
    end_gep->push_idx_imm32(2);
    func_block->Instruction_list.push_back(end_gep);
    
    auto end_load = new LoadInstruction(BasicInstruction::LLVMType::I32, end_gep_reg, end_reg);
    func_block->Instruction_list.push_back(end_load);
    
    // 处理循环外部变量
    int bias = 3; // thread_id, start, end 之后
	// std::cout << "i32set.size(): " << i32set.size() << std::endl;
	// std::cout << "i64set.size(): " << i64set.size() << std::endl;
    
    // 加载I32和FLOAT32类型的外部变量
    for (int regno : i32set) {
        auto var_gep_reg = GetNewRegOperand(++max_reg);
        // 使用新的GEP构造函数，直接传入偏移量
        auto var_gep = new GetElementptrInstruction(
            BasicInstruction::LLVMType::I32, var_gep_reg, args_param, new ImmI32Operand(bias), BasicInstruction::LLVMType::I32);
        func_block->Instruction_list.push_back(var_gep);
        
        auto var_reg = GetNewRegOperand(++max_reg);
        auto load_type = float32set.count(regno) ? BasicInstruction::LLVMType::FLOAT32 : BasicInstruction::LLVMType::I32;
        auto var_load = new LoadInstruction(load_type, var_gep_reg, var_reg);
        func_block->Instruction_list.push_back(var_load);
        
        // 将变量添加到映射中，供后续寄存器映射替换使用
        reg_mapping[regno] = max_reg;
		// std::cout << "[arg_i32] regno: " << regno << ", max_reg: " << max_reg << std::endl;
		bias += 1;
    }
    
    // 加载PTR类型的外部变量
    for (int regno : i64set) {
        auto var_gep_reg = GetNewRegOperand(++max_reg);
        auto var_gep = new GetElementptrInstruction(
            BasicInstruction::LLVMType::I32, var_gep_reg, args_param, new ImmI32Operand(bias), BasicInstruction::LLVMType::I32);
        func_block->Instruction_list.push_back(var_gep);
        
        auto var_reg = GetNewRegOperand(++max_reg);
        auto var_load = new LoadInstruction(BasicInstruction::LLVMType::PTR, var_gep_reg, var_reg);
        func_block->Instruction_list.push_back(var_load);
        
        // 将变量添加到映射中
        reg_mapping[regno] = max_reg;
		// std::cout << "[arg_ptr] regno: " << regno << ", max_reg: " << max_reg << std::endl;
		bias += 2; // PTR变量占用8字节
    }
}

void LoopParallelismPass::AddThreadRangeCalculation(FunctionDefineInstruction* func_def, std::map<int, int>& reg_mapping) {
    // 计算每个线程的循环范围
    // part = (end - start) / 4
    // my_start = part * thread_id + start
    // my_end = part * (thread_id + 1) + start
    
    int& max_reg = llvmIR->function_max_reg[func_def];
    int& max_label = llvmIR->function_max_label[func_def];
    auto func_block = llvmIR->function_block_map[func_def][0];
    
    // 获取之前解析的参数
	// r0 - ((int*)args), r2 - ((int*)args)[0], r4 - ((int*)args)[1], r6 - ((int*)args)[2]
    auto thread_id_reg = GetNewRegOperand(thread_id_regNo);
    auto start_reg = GetNewRegOperand(start_regNo);
    auto end_reg = GetNewRegOperand(end_regNo);
    
    // part = (end - start) / 4
    auto diff_reg = GetNewRegOperand(++max_reg);
    auto sub_inst = new ArithmeticInstruction(BasicInstruction::SUB, BasicInstruction::LLVMType::I32, end_reg, start_reg, diff_reg);
    func_block->Instruction_list.push_back(sub_inst);
    
	auto part_reg = GetNewRegOperand(++max_reg);
    auto div_inst = new ArithmeticInstruction(BasicInstruction::DIV, BasicInstruction::LLVMType::I32, diff_reg, new ImmI32Operand(4), part_reg);
    func_block->Instruction_list.push_back(div_inst);
    
    // my_start = part * thread_id + start
	auto mid_reg = GetNewRegOperand(++max_reg);
    auto mul_inst = new ArithmeticInstruction(BasicInstruction::MUL, BasicInstruction::LLVMType::I32, part_reg, thread_id_reg, mid_reg);
    func_block->Instruction_list.push_back(mul_inst);
    
	auto my_start_reg = GetNewRegOperand(++max_reg);
	my_start_regNo = max_reg;
    auto add_inst = new ArithmeticInstruction(BasicInstruction::ADD, BasicInstruction::LLVMType::I32, mid_reg, start_reg, my_start_reg);
    func_block->Instruction_list.push_back(add_inst);
    
    // 计算 my_end，需要处理最后一个线程的特殊情况
    // my_end = part * (thread_id + 1) + start = my_start + part
    auto temp_reg = GetNewRegOperand(++max_reg);
    auto add2_inst = new ArithmeticInstruction(BasicInstruction::ADD, BasicInstruction::LLVMType::I32, my_start_reg, part_reg, temp_reg);
    func_block->Instruction_list.push_back(add2_inst);
    
    // 创建条件判断：if (thread_id == 3) my_end = end; else my_end = temp_reg;
    auto cmp_reg = GetNewRegOperand(++max_reg);
    auto cmp_inst = new IcmpInstruction(BasicInstruction::LLVMType::I32, thread_id_reg, new ImmI32Operand(3), IcmpInstruction::eq, cmp_reg);
    func_block->Instruction_list.push_back(cmp_inst);
    
    auto branch_block = llvmIR->NewBlock(func_def, ++max_label); // my_end = end
    auto merge_block = llvmIR->NewBlock(func_def, ++max_label); // my_end = temp_reg
    
    auto br_cond_inst = new BrCondInstruction(cmp_reg, GetNewLabelOperand(branch_block->block_id), GetNewLabelOperand(merge_block->block_id));
    func_block->Instruction_list.push_back(br_cond_inst);

	std::cout << "branch_block->block_id: " << branch_block->block_id << std::endl;
	std::cout << "merge_block->block_id: " << merge_block->block_id << std::endl;
    
    // branch_block: my_end = end
    auto br_uncond1 = new BrUncondInstruction(GetNewLabelOperand(merge_block->block_id));
    branch_block->Instruction_list.push_back(br_uncond1);

    // 在merge_block中使用PHI指令汇总结果
    auto my_end_reg = GetNewRegOperand(++max_reg);
	my_end_regNo = max_reg;
    std::vector<std::pair<Operand, Operand>> phi_list;
    phi_list.push_back({GetNewLabelOperand(branch_block->block_id), end_reg});
    phi_list.push_back({GetNewLabelOperand(func_block->block_id), temp_reg});
    auto phi_inst = new PhiInstruction(BasicInstruction::LLVMType::I32, my_end_reg, phi_list);
    merge_block->Instruction_list.push_back(phi_inst);

	// auto preheader_block = llvmIR->NewBlock(func_def, ++max_label);
	auto br_uncond2 = new BrUncondInstruction(GetNewLabelOperand(max_label + 1));
	merge_block->Instruction_list.push_back(br_uncond2);
	new_preheader_block_id = max_label;
	new_header_block_id = max_label + 1;
}

void LoopParallelismPass::CopyLoopBodyInstructions(Loop* loop, FunctionDefineInstruction* func_def, std::map<int, int>& reg_mapping) {
    // 循环外的变量需要与参数读取时的顺序对应，循环内的reg则需要与新建的reg不冲突 (此处处理循环内)
    for (auto bb : loop->getBlocks()) {
        for (auto I : bb->Instruction_list) {
			auto res_op = I->GetResult();
			if (res_op && res_op->GetOperandType() == BasicOperand::REG) {
				auto r = (RegOperand*)res_op;
				int regno = r->GetRegNo();
				if (reg_mapping.find(regno) == reg_mapping.end()) {
					int& max_reg = llvmIR->function_max_reg[func_def];
					int new_regno = ++max_reg;
					reg_mapping[regno] = new_regno;
				}
			}

            for (auto op : I->GetNonResultOperands()) {
                if (op->GetOperandType() == BasicOperand::REG) {
                    auto r = (RegOperand*)op;
                    int regno = r->GetRegNo();
                    if (reg_mapping.find(regno) == reg_mapping.end()) {
                        int& max_reg = llvmIR->function_max_reg[func_def];
                        int new_regno = ++max_reg;
                        reg_mapping[regno] = new_regno;
                    }
                }
            }
        }
    }

	// show reg_mapping
	for (auto [regno, new_regno] : reg_mapping) {
		std::cout << "regno: " << regno << ", new_regno: " << new_regno << std::endl;
	}

    // 创建新的基本块映射
    std::map<int, int> labelreplace_map;
    
    // 为循环中的每个基本块创建新的基本块
	int& max_label = llvmIR->function_max_label[func_def];
    for (auto block : loop->getBlocks()) {
        auto new_block = llvmIR->NewBlock(func_def, ++max_label);
        labelreplace_map[block->block_id] = new_block->block_id;
		LLVMBlock old_latch = *(loop->getLatches().begin());
		if (block->block_id == old_latch->block_id) {
			new_latch_block_id = new_block->block_id;
		}
        
        for (auto inst : block->Instruction_list) {
            Instruction cloned_inst = CloneInstruction(inst, func_def, reg_mapping);
            if (cloned_inst) {
                new_block->Instruction_list.push_back(cloned_inst);
            }
        }
    }

	// exit、preheader 需要特殊处理，这是循环中唯一出现的循环外标签
	// 循环跳转到 exit 表示循环运行结束，此处函数封装循环，exit 表示函数返回
	auto func_exit_block = llvmIR->NewBlock(func_def, ++max_label);
	LLVMBlock exit = *loop->getExits().begin();
	labelreplace_map[exit->block_id] = func_exit_block->block_id;
	auto preheader = loop->getPreheader();
	labelreplace_map[preheader->block_id] = new_preheader_block_id;

	auto ret_void_inst = new RetInstruction(BasicInstruction::LLVMType::VOID, nullptr);
	func_exit_block->Instruction_list.push_back(ret_void_inst);
    
    // 更新跳转指令中的标签
    for (auto [id, bb] : llvmIR->function_block_map[func_def]) {
		if (id <= new_preheader_block_id) {
			continue;
		}
        for (auto inst : bb->Instruction_list) {
            if (inst->GetOpcode() == BasicInstruction::BR_UNCOND) {
                BrUncondInstruction* br_inst = (BrUncondInstruction*)inst;
                Operand dest_label = br_inst->GetDestLabel();
                if (dest_label->GetOperandType() == BasicOperand::LABEL) {
                    int old_label = ((LabelOperand*)dest_label)->GetLabelNo();
                    if (labelreplace_map.find(old_label) != labelreplace_map.end()) {
                        int new_label = labelreplace_map[old_label];
                        br_inst->SetTarget(GetNewLabelOperand(new_label));
                    }
                }
            } else if (inst->GetOpcode() == BasicInstruction::BR_COND) {
                BrCondInstruction* br_inst = (BrCondInstruction*)inst;
                Operand true_label = br_inst->GetTrueLabel();
                Operand false_label = br_inst->GetFalseLabel();
                
                if (true_label->GetOperandType() == BasicOperand::LABEL) {
                    int old_label = ((LabelOperand*)true_label)->GetLabelNo();
                    if (labelreplace_map.find(old_label) != labelreplace_map.end()) {
                        int new_label = labelreplace_map[old_label];
                        br_inst->SetTrueLabel(GetNewLabelOperand(new_label));
                    }
                }
                
                if (false_label->GetOperandType() == BasicOperand::LABEL) {
                    int old_label = ((LabelOperand*)false_label)->GetLabelNo();
                    if (labelreplace_map.find(old_label) != labelreplace_map.end()) {
                        int new_label = labelreplace_map[old_label];
                        br_inst->SetFalseLabel(GetNewLabelOperand(new_label));
                    }
                }
            } else if (inst->GetOpcode() == BasicInstruction::PHI) {
				PhiInstruction* phi_inst = (PhiInstruction*)inst;
				auto phi_list = phi_inst->GetPhiList();
				std::vector<std::pair<Operand, Operand>> new_phi_list;
				for (auto [label, value] : phi_list) {
					if (label->GetOperandType() == BasicOperand::LABEL) {
						int old_label = ((LabelOperand*)label)->GetLabelNo();
						if (labelreplace_map.find(old_label) != labelreplace_map.end()) {
							int new_label = labelreplace_map[old_label];
							label = GetNewLabelOperand(new_label);
						}
					}
					new_phi_list.push_back({label, value});
				}
				phi_inst->SetPhiList(new_phi_list);
			}
        }
    }

	// std::cout << "new_preheader_block_id: " << new_preheader_block_id << std::endl;
	// std::cout << "new_latch_block_id: " << new_latch_block_id << std::endl;

	// 处理 preheader 和 condinst, 修改上下界
	LLVMBlock new_header_block = llvmIR->function_block_map[func_def][new_header_block_id];
	int icmp_inst_idx = new_header_block->Instruction_list.size() - 2;
	auto icmp_inst = dynamic_cast<IcmpInstruction*>(new_header_block->Instruction_list[icmp_inst_idx]);

	// 1. 获取icmp的两个操作数
	auto icmp_op0 = icmp_inst->GetOp1();
	auto icmp_op1 = icmp_inst->GetOp2();

	// 2. 预先准备start和end操作数
	Operand start_operand = GetNewRegOperand(my_start_regNo);
	Operand end_operand = GetNewRegOperand(my_end_regNo);

	// 3. 查找循环变量的PHI指令，并重写其phi列表
	PhiInstruction* loop_phi = nullptr;
	Operand loop_var = nullptr;
	Operand loop_init = nullptr;
	Operand loop_next = nullptr;

	for (auto inst : new_header_block->Instruction_list) {
		if (inst->GetOpcode() != BasicInstruction::PHI) continue;
		PhiInstruction* phi = dynamic_cast<PhiInstruction*>(inst);
		if (!phi) continue;

		// 判断该PHI是否为循环变量
		if (phi->GetResult() == icmp_op0 || phi->GetResult() == icmp_op1) {
			loop_phi = phi;
			loop_var = phi->GetResult();
			std::vector<std::pair<Operand, Operand>> new_phi_list;
			for (auto [label, value] : phi->GetPhiList()) {
				if (label->GetOperandType() == BasicOperand::LABEL) {
					int label_no = ((LabelOperand*)label)->GetLabelNo();
					if (label_no == new_preheader_block_id) {
						// preheader分支，初值设为start
						loop_init = start_operand;
						new_phi_list.push_back({label, start_operand});
					} else if (label_no == new_latch_block_id) {
						// latch分支，保持原有next
						loop_next = value;
						new_phi_list.push_back({label, value});
					} else {
						new_phi_list.push_back({label, value});
					}
				} else {
					new_phi_list.push_back({label, value});
				}
			}
			phi->SetPhiList(new_phi_list);
			break; 
		}
	}

	// 4. 修正icmp的比较边界
	if (icmp_inst && loop_var) {
		// 假设循环变量和边界分别在op0/op1
		if (icmp_op0 == loop_var) {
			icmp_inst->SetOp2(end_operand);
		} else if (icmp_op1 == loop_var) {
			icmp_inst->SetOp1(end_operand);
		}
	}
}

Instruction LoopParallelismPass::CloneInstruction(Instruction inst, FunctionDefineInstruction* func_def, std::map<int, int>& reg_mapping) {
    // 使用CopyInstruction方法
    Instruction cloned_inst = inst->Clone();
    
    if (!cloned_inst) {
        return nullptr;
    }
    
	// 处理非结果操作数
	auto newNonresultOperands = std::vector<Operand>();
	for (auto op : cloned_inst->GetNonResultOperands()) {
		if (op->GetOperandType() == BasicOperand::REG) {
			auto r = (RegOperand*)op;
			int regno = r->GetRegNo();
			if (reg_mapping.find(regno) != reg_mapping.end()) {
				auto new_reg = GetNewRegOperand(reg_mapping[regno]);
				newNonresultOperands.push_back(new_reg);
			} else {
				newNonresultOperands.push_back(op);
			}
		} else {
			newNonresultOperands.push_back(op);
		}
	}
	cloned_inst->SetNonResultOperands(newNonresultOperands);

	// 处理结果操作数，避免递归替换
	auto result_op = cloned_inst->GetResult();
	if (result_op && result_op->GetOperandType() == BasicOperand::REG) {
		auto r = (RegOperand*)result_op;
		int regno = r->GetRegNo();
		if (reg_mapping.find(regno) != reg_mapping.end()) {
			auto new_result_reg = GetNewRegOperand(reg_mapping[regno]);
			cloned_inst->SetResult(new_result_reg);
		}
	}
    
    return cloned_inst;
}

void LoopParallelismPass::CreateConditionalParallelization(Loop* loop, CFG* cfg, const std::string& func_name, 
                                                          ScalarEvolution* SE) {
    // 方案二：创建分支结构
    // if (iterations < threshold) {
    //     // 执行原循环（串行）
    // } else {
    //     // 执行并行化版本
    // }
    
    // 使用extractLoopParams获取循环参数
    auto loop_params = SE->extractLoopParams(loop, cfg);
    
    const int PARALLEL_THRESHOLD = 50;
    
    if (loop_params.is_simple_loop && loop_params.count >= PARALLEL_THRESHOLD) {
        // 创建并行化调用
        CreateParallelCall(loop, cfg, func_name, SE);
    }
}

void LoopParallelismPass::CreateParallelCall(Loop* loop, CFG* cfg, const std::string& func_name, ScalarEvolution* SE) {

    auto loop_params = SE->extractLoopParams(loop, cfg);

    // 创建参数列表
    std::vector<std::pair<BasicInstruction::LLVMType, Operand>> params;
    
    // 函数指针参数
    auto func_ptr = GetNewGlobalOperand(func_name);
    params.push_back({BasicInstruction::LLVMType::PTR, func_ptr});
    
    // 循环起始和结束
    params.push_back({BasicInstruction::LLVMType::I32, new ImmI32Operand(loop_params.start_val)});
    params.push_back({BasicInstruction::LLVMType::I32, new ImmI32Operand(loop_params.bound_val)});
    
    // I32变量数量和PTR变量数量
    params.push_back({BasicInstruction::LLVMType::I32, new ImmI32Operand(i32set.size())});
    params.push_back({BasicInstruction::LLVMType::I32, new ImmI32Operand(i64set.size())});
    
    LLVMBlock preheader = loop->getPreheader();
	preheader->Instruction_list.pop_back();  // pop br_uncond_inst

    // 添加I32类型的外部变量
    for (auto rn : i32set) {
        if (i32set.find(rn) != i32set.end()) {
            // FLOAT32以无符号整数形式传递，需要转换
			auto real_rn = rn;
			if (float32set.find(rn) != float32set.end()) {
				auto bitcastI = new BitCastInstruction(GetNewRegOperand(++cfg->max_reg), GetNewRegOperand(rn), BasicInstruction::LLVMType::FLOAT32, BasicInstruction::LLVMType::I32);
				real_rn = cfg->max_reg;
				// 将bitcast指令插入到preheader中
				LLVMBlock preheader = loop->getPreheader();
				if (preheader) {
					preheader->Instruction_list.push_back(bitcastI);
				}
			}
			params.push_back({BasicInstruction::LLVMType::I32, GetNewRegOperand(real_rn)});
        } else {
            // I32类型直接传递
            params.push_back({BasicInstruction::LLVMType::I32, GetNewRegOperand(rn)});
        }
    }
    
    // 添加PTR类型的外部变量
    for (auto rn : i64set) {
        params.push_back({BasicInstruction::LLVMType::PTR, GetNewRegOperand(rn)});
    }

    auto call_inst = new CallInstruction(BasicInstruction::LLVMType::VOID, nullptr, "___parallel_loop_constant_100", params);
    
    // 将调用指令插入到循环头部之前
    if (preheader) {
        preheader->Instruction_list.push_back(call_inst);
    }
	auto exit_block = *loop->getExits().begin();
	auto br_uncond_inst = new BrUncondInstruction(GetNewLabelOperand(exit_block->block_id));
	preheader->Instruction_list.push_back(br_uncond_inst);
	
    // PARALLEL_DEBUG(std::cout << "创建并行化调用: " << func_name << std::endl);
}

void LoopParallelismPass::AddRuntimeLibraryDeclarations() {
    // 添加运行时库函数声明
    // void ___parallel_loop_constant_100(void* func_ptr, int start, int end, int len1, int len2, ...);
    
    // 创建函数声明
    auto decl = new FunctionDeclareInstruction(BasicInstruction::LLVMType::VOID, "___parallel_loop_constant_100");
    decl->InsertFormal(BasicInstruction::LLVMType::PTR);  // func_ptr
    decl->InsertFormal(BasicInstruction::LLVMType::I32);  // start
    decl->InsertFormal(BasicInstruction::LLVMType::I32);  // end
    decl->InsertFormal(BasicInstruction::LLVMType::I32);  // len1 (I32变量数量)
    decl->InsertFormal(BasicInstruction::LLVMType::I32);  // len2 (PTR变量数量)
    
    // 添加到全局函数表
    llvmIR->function_declare.push_back(decl);
    
    PARALLEL_DEBUG(std::cout << "添加运行时库函数声明" << std::endl);
}

void LoopParallelismPass::CollectLoopExternalVariables(Loop* loop, CFG* cfg) {

    // 获取循环中使用的寄存器映射
    std::map<int, Instruction> ResultMap;
    for (auto [id, bb] : *cfg->block_map) {
        for (auto I : bb->Instruction_list) {
            Operand v = I->GetResult();
            if (v && v->GetOperandType() == BasicOperand::REG) {
                auto v_reg = (RegOperand*)v;
                ResultMap[v_reg->GetRegNo()] = I;
            }
        }
    }
    
    // 打印循环包含的块
    std::cout << "Loop blocks: ";
    for (auto bb : loop->getBlocks()) {
        std::cout << bb->block_id << " ";
    }
    std::cout << std::endl;
    
    // 识别循环中使用但在循环外定义的变量
    for (auto bb : loop->getBlocks()) {
        for (auto I : bb->Instruction_list) {
            for (auto op : I->GetNonResultOperands()) {
                if (op->GetOperandType() == BasicOperand::REG) {
                    auto r = (RegOperand*)op;
                    int regno = r->GetRegNo();
                    if (ResultMap.find(regno) != ResultMap.end()) {
                        auto outloop_defI = ResultMap[regno];
                        auto def_bbid = ResultMap[regno]->GetBlockID();
                        auto def_bb = (*cfg->block_map)[def_bbid];
                        if (!loop->contains(def_bb)) {
							std::cout << "regno: " << regno << std::endl;
							std::cout << "def_bbid: " << def_bbid << std::endl;
							std::cout << "outloop_defI: "; outloop_defI->PrintIR(std::cout);
                            auto type = outloop_defI->GetType();
                            if (type == BasicInstruction::LLVMType::I32) {
                                i32set.insert(regno);
                            } else if (type == BasicInstruction::LLVMType::PTR) {
                                i64set.insert(regno);
                            } else if (type == BasicInstruction::LLVMType::FLOAT32) {
								i32set.insert(regno);
								float32set.insert(regno);
							}
                        } 
                    } else {  // func_arg no def_bb
						auto type = cfg->function_def->formals[regno];
						if (type == BasicInstruction::LLVMType::I32) {
							i32set.insert(regno);
						} else if (type == BasicInstruction::LLVMType::PTR) {
							i64set.insert(regno);
						} else if (type == BasicInstruction::LLVMType::FLOAT32) {
							float32set.insert(regno);
							i32set.insert(regno);
						}
					}
                }
            }
        }
    }
	std::cout << "i32set: ";
	for (auto regno : i32set) {
		std::cout << regno << " ";
	}
	std::cout << std::endl;
	std::cout << "i64set: ";
	for (auto regno : i64set) {
		std::cout << regno << " ";
	}
	std::cout << std::endl;
	std::cout << "float32set: ";
	for (auto regno : float32set) {
		std::cout << regno << " ";
	}
	std::cout << std::endl;
}

bool LoopParallelismPass::HasLoopExternalUses(Loop* loop, CFG* cfg) {
    // 1. 统计循环内的defset
    std::set<int> loop_def_set;
    for (auto bb : loop->getBlocks()) {
        for (auto I : bb->Instruction_list) {
            auto res_op = I->GetResult();
            if (res_op && res_op->GetOperandType() == BasicOperand::REG) {
                auto r = (RegOperand*)res_op;
                loop_def_set.insert(r->GetRegNo());
            }
        }
    }
    
    // 2. 遍历循环外指令的use(nonresop)，观察是否用到循环内def
    for (auto [id, bb] : *cfg->block_map) {
        // 跳过循环内的块
        if (loop->contains(bb)) {
            continue;
        }
        
        for (auto I : bb->Instruction_list) {
            for (auto op : I->GetNonResultOperands()) {
                if (op->GetOperandType() == BasicOperand::REG) {
                    auto r = (RegOperand*)op;
                    int regno = r->GetRegNo();
                    if (loop_def_set.find(regno) != loop_def_set.end()) {
                        PARALLEL_DEBUG(std::cout << "循环外使用了循环内的寄存器: " << regno << std::endl);
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

void LoopParallelismPass::BuildCFGforParallelFunction(FunctionDefineInstruction* func_def) {
	CFG *cfg = new CFG();

    cfg->block_map = &llvmIR->function_block_map[func_def];
    cfg->function_def = func_def;
    cfg->max_reg = llvmIR->function_max_reg[func_def];
    cfg->max_label = llvmIR->function_max_label[func_def];

    llvmIR->llvm_cfg[func_def] = cfg;

    cfg->BuildCFG();
	DominatorTree* dom_tree = new DominatorTree(cfg);
    dom_tree->BuildDominatorTree();
	DomInfo[cfg] = dom_tree;
}

