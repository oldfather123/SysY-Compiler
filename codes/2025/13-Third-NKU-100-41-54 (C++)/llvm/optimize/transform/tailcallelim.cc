#include "tailcallelim.h"

bool TailCallElimPass::IsTailCallFunc(CFG *C) {
	// backend need more spill to solve this case
	if(C->function_def->formals.size() > 5) {
		return false;
	}

	// location varry alloca at block_in, may create mistake. 00007_DP.sy
	auto block_in = (*C->block_map)[0];
	auto block_in_id = block_in->block_id;
	std::unordered_set<int> formals_reg; 
	for(auto inst : block_in->Instruction_list) {
		if (auto store_inst = dynamic_cast<StoreInstruction*>(inst)) {
			// store <ty> <value>, ptr<pointer>, ops {<pointer>, <value>}
			std::vector<Operand> ops = store_inst->GetNonResultOperands();
			auto pointer_op = (RegOperand*) ops[0];
			formals_reg.insert(pointer_op->GetRegNo());
		}
	}

	for(auto inst : block_in->Instruction_list) {
		if (auto alloca_inst = dynamic_cast<AllocaInstruction*>(inst)) {
			auto res_op = (RegOperand*) alloca_inst->GetResult();
			if(!formals_reg.count(res_op->GetRegNo())) {
				return false;
			}
		}
	}


    for (auto [block_id, curr_block] : *C->block_map) {
        for (auto it = curr_block->Instruction_list.begin(); it != curr_block->Instruction_list.end(); it++) {
			auto inst = *it;
            if (auto call_inst = dynamic_cast<CallInstruction*>(inst)) {
                if (call_inst->GetFunctionName() == C->function_def->GetFunctionName()) {
                    auto next_inst = *next(it);
                    if (!next_inst || dynamic_cast<RetInstruction*>(next_inst)) {
						return true;
                    }
                } 
            }
        }
    }
    return false;
}

void TailCallElimPass::TailCallElim(CFG *C) {
	// std::cerr << "call TailCallElim()" << std::endl;
	if(!IsTailCallFunc(C)) return;
	// std::cerr << "IsTailCallFunc() = true, functionName = " + C->function_def->GetFunctionName() << std::endl;

	/*  args that is not ptr, originally, auto create a ptr to store its value.
		define float @DFS(i32 %r0,float %r1,float %r2,float %r3,ptr %r4)
		L0: 
		...
		%r5 = alloca i32
		store i32 %r0, ptr %r5
		...
	*/
	auto block_in = (*C->block_map)[0];
	auto block_in_id = block_in->block_id;
	std::unordered_map<int, int> alloca_map; 
	for(auto inst : block_in->Instruction_list) {
		if (auto store_inst = dynamic_cast<StoreInstruction*>(inst)) {
			// store <ty> <value>, ptr<pointer>, ops {<pointer>, <value>}
			std::vector<Operand> ops = store_inst->GetNonResultOperands();
			auto pointer_op = (RegOperand*) ops[0];
			auto value_op = (RegOperand*) ops[1];
			alloca_map[value_op->GetRegNo()] = pointer_op->GetRegNo();
		}
	}


	/* check ptr, mem2reg need %ptr only define one time 
	so, we can't %r2 = getelementptr %r20, to get new ptr value.
	we shold get a new reg to store ptr at label 1.
	> %ptr_storage = alloca ptr
	> store ptr %initial_ptr, ptr %ptr_storage
	when we visit ptr, we visit ptr_storage.
	> %res = getelementptr i32, ptr %initial_ptr  |  > %new_ptr = load ptr, ptr %ptr_storage
												  |  > %res = getelementptr i32, ptr %new_ptr
	*/
	std::unordered_map<int, int> ptr_map; 
	FuncDefInstruction funcdef = C->function_def;
	for(int i = 0; i < funcdef->formals.size(); i++) {
		auto type = funcdef->formals[i];
		auto op = (RegOperand*)funcdef->formals_reg[i];
		if(type == BasicInstruction::LLVMType::PTR) {
			ptr_map.insert(std::make_pair(op->GetRegNo(), ++C->max_reg));
			auto ptr_storage = GetNewRegOperand(C->max_reg);
			auto allca_inst = new AllocaInstruction(BasicInstruction::LLVMType::PTR, ptr_storage);
			auto store_inst = new StoreInstruction(BasicInstruction::LLVMType::PTR, ptr_storage, op);
			block_in->InsertInstruction(0, store_inst);
			block_in->InsertInstruction(0, allca_inst);
		}
	}

	for (auto [block_id, curr_block] : *C->block_map) {
        for (auto it = curr_block->Instruction_list.begin(); it != curr_block->Instruction_list.end(); it++) {
			auto inst = *it;
			
            if (auto call_inst = dynamic_cast<CallInstruction*>(inst)) {
				auto next_inst = *next(it);
				if (call_inst->GetFunctionName() != C->function_def->GetFunctionName()) { continue; }
				if (!next_inst || !dynamic_cast<RetInstruction*>(next_inst)) { continue; }
				auto para_list = call_inst->GetParameterList();
				/*  origin 											  | 	optimize			
				    %r16 = sub i32 %r14,%r15						  | 
					%r17 = load i32, ptr %r4						  | 
					%r18 = load i32, ptr %r3						  | 
					%r19 = mul i32 %r17,%r18						  | 	store i32 %r16, ptr %r3
					%r20 = getelementptr i32, ptr %r2				  | 	store i32 %r19, ptr %r4
					%r21 = call i32 @fib(i32 %r16,i32 %r19,ptr %r20)  | 	store ptr %r20, ptr %r22
					ret i32 %r21									  | 	br label %L1
				*/

				// erase call_inst & ret_inst
				curr_block->Instruction_list.erase(it);
				curr_block->Instruction_list.erase(next(it));
				
				for(int i = 0; i < para_list.size(); i++) {
					auto para_type = para_list[i].first;
					auto para_op = (RegOperand*) para_list[i].second;
					auto formal_op = (RegOperand*) funcdef->formals_reg[i];
					StoreInstruction* store_inst;
					if(para_type == BasicInstruction::LLVMType::PTR) {
						int ptr_storage_regNo = ptr_map[formal_op->GetRegNo()];
						store_inst = new StoreInstruction(para_type, GetNewRegOperand(ptr_storage_regNo), para_op);
					} else {
						int ptr_storage_regNo = alloca_map[formal_op->GetRegNo()];
						store_inst = new StoreInstruction(para_type, GetNewRegOperand(ptr_storage_regNo), para_op);
					}
					curr_block->InsertInstruction(1, store_inst);
				}

				// insert br_inst
				auto br_inst = new BrUncondInstruction(GetNewLabelOperand(1));
				curr_block->InsertInstruction(1, br_inst);
				break;
            } else if (auto gep_inst = dynamic_cast<GetElementptrInstruction*>(inst)) {
				// > %res = getelementptr i32, ptr %initial_ptr  |  > %new_ptr = load ptr, ptr %ptr_storage
				// 							 					 |  > %res = getelementptr i32, ptr %new_ptr
				auto initial_ptr_op = (RegOperand*)gep_inst->GetPtrVal();
				if(ptr_map.count(initial_ptr_op->GetRegNo())) {
					size_t offset = it - curr_block->Instruction_list.begin();
					auto ptr_storage_reg = GetNewRegOperand(ptr_map[initial_ptr_op->GetRegNo()]);
					auto new_ptr_reg = GetNewRegOperand(++C->max_reg);
					auto load_inst = new LoadInstruction(BasicInstruction::LLVMType::PTR, ptr_storage_reg, new_ptr_reg);
					curr_block->Instruction_list.insert(it, load_inst);     // iterator is invalid when insert	
					it = curr_block->Instruction_list.begin() + offset + 1; // recover
					gep_inst->SetNonResultOperands({new_ptr_reg});
				}
			}
        }
    }
}

void TailCallElimPass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
		TailCallElim(cfg);
		cfg->BuildCFG();
    }
}