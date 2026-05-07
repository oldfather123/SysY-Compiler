#include "peephole.h"
#include <functional>
#include "../analysis/ScalarEvolution.h"

void ImmResultReplace(CFG *C);
static void NegMulAddToSub(CFG *C);

void PeepholePass::Execute() {
	DeadArgElim();
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        // add all implemented pass
		SrcEqResultInstEliminate(cfg);
        IdentitiesEliminate(cfg);
		ImmResultReplace(cfg);
        NegMulAddToSub(cfg);
    }
}

void PeepholePass::NegMulAddToSubExecute(){
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        NegMulAddToSub(cfg);
    }
}

void PeepholePass::SrcEqResultInstEliminateExecute(){
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        SrcEqResultInstEliminate(cfg);
    }
}

void PeepholePass::ImmResultReplaceExecute(){
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        ImmResultReplace(cfg);
    }
}

void PeepholePass::IdentitiesEliminateExecute(){
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        IdentitiesEliminate(cfg);
    }
}


//常量折叠
void ImmResultReplace(CFG *C){
    std::set<Instruction> EraseSet;
    std::map<Operand, Operand> replacemap;
    bool changed = true;
    while(changed){
        changed = false;
        EraseSet.clear();
        replacemap.clear();
        for(auto [id, bb] : *C->block_map){
            for(auto I : bb->Instruction_list){
                auto opcode = I->GetOpcode();
                if(opcode == BasicInstruction::ADD 
                        || opcode == BasicInstruction::SUB 
                        || opcode == BasicInstruction::MUL 
                        || opcode == BasicInstruction::DIV 
                        || opcode == BasicInstruction::MOD 
                        || opcode == BasicInstruction::BITXOR
                        || opcode == BasicInstruction::FADD 
                        || opcode == BasicInstruction::FSUB 
                        || opcode == BasicInstruction::FMUL 
                        || opcode == BasicInstruction::FDIV){
                    auto ArithI = (ArithmeticInstruction*)I;
                    auto resultop = ArithI->GetResult();
                    auto op1 = ArithI->GetOperand1();
                    auto op2 = ArithI->GetOperand2();
                    if(op1->GetOperandType() != BasicOperand::REG && op2->GetOperandType() != BasicOperand::REG){
                        EraseSet.insert(I);
                        auto type = ArithI->GetDataType();
                        changed = 1;
                        if(type == BasicInstruction::I32){
                            auto imm1 = ((ImmI32Operand*)op1)->GetIntImmVal();
                            auto imm2 = ((ImmI32Operand*)op2)->GetIntImmVal();
                            int ans = 0;
                            switch (opcode)
                            {
                            case BasicInstruction::ADD:
                                ans = imm1 + imm2;
                                break;
                            case BasicInstruction::SUB:
                                ans = imm1 - imm2;
                                break;
                            case BasicInstruction::MUL:
                                ans = imm1 * imm2;
                                break;
                            case BasicInstruction::DIV:
                                ans = imm1 / imm2;
                                break;
                            case BasicInstruction::MOD:
                                ans = imm1 % imm2;
                                break;
                            case BasicInstruction::BITXOR:
                                ans = imm1 ^ imm2;
                                break;
                            default:
                                break;
                            }
                            replacemap[resultop] = new ImmI32Operand(ans);
                        }else {
                            auto imm1 = ((ImmF32Operand*)op1)->GetFloatVal();
                            auto imm2 = ((ImmF32Operand*)op2)->GetFloatVal();
                            float ans = 0;
                            switch (opcode)
                            {
                            case BasicInstruction::FADD:
                                ans = imm1 + imm2;
                                break;
                            case BasicInstruction::FSUB:
                                ans = imm1 - imm2;
                                break;
                            case BasicInstruction::FMUL:
                                ans = imm1 * imm2;
                                break;
                            case BasicInstruction::FDIV:
                                ans = imm1 / imm2;
                                break;
                            default:
                                break;
                            }
                            replacemap[resultop] = new ImmF32Operand(ans);
                        }
                    }
                }else if(I->GetOpcode() == BasicInstruction::ICMP){
                    auto icmpI = (IcmpInstruction*)I;
                    auto resultop = icmpI->GetResult();
                    auto op1 = icmpI->GetOp1();
                    auto op2 = icmpI->GetOp2();
                    auto cond = icmpI->GetCond();
                    if(op1->GetOperandType() != BasicOperand::REG && op2->GetOperandType() != BasicOperand::REG){
                        EraseSet.insert(I);
                        changed = 1;
                        int ans = 0;
                        auto imm1 = ((ImmI32Operand*)op1)->GetIntImmVal();
                        auto imm2 = ((ImmI32Operand*)op2)->GetIntImmVal();
                        switch (cond)
                        {
                        case BasicInstruction::eq:
                            ans = imm1 == imm2;
                            break;
                        case BasicInstruction::ne:
                            ans = imm1 != imm2;
                            break;
                        case BasicInstruction::ugt:
                            ans = imm1 > imm2;
                            break;
                        case BasicInstruction::uge:
                            ans = imm1 >= imm2;
                            break;
                        case BasicInstruction::ult:
                            ans = imm1 < imm2;
                            break;
                        case BasicInstruction::ule:
                            ans = imm1 <= imm2;
                            break;
                        case BasicInstruction::sgt:
                            ans = imm1 > imm2;
                            break;
                        case BasicInstruction::sge:
                            ans = imm1 >= imm2;
                            break;
                        case BasicInstruction::slt:
                            ans = imm1 < imm2;
                            break;
                        case BasicInstruction::sle:
                            ans = imm1 <= imm2;
                            break;
                        default:
                            break;
                        }
                        replacemap[resultop] = new ImmI32Operand(ans);
                    }
                }else if(I->GetOpcode() == BasicInstruction::FCMP){
                    auto fcmpI = (FcmpInstruction*)I;
                    auto resultop = fcmpI->GetResult();
                    auto op1 = fcmpI->GetOp1();
                    auto op2 = fcmpI->GetOp2();
                    auto cond = fcmpI->GetCond();
                    if(op1->GetOperandType() != BasicOperand::REG && op2->GetOperandType() != BasicOperand::REG){
                        EraseSet.insert(I);
                        changed = 1;
                        int ans = 0;
                        auto imm1 = ((ImmF32Operand*)op1)->GetFloatVal();
                        auto imm2 = ((ImmF32Operand*)op2)->GetFloatVal();
                        switch (cond)
                        {
                        case BasicInstruction::FALSE:
                            ans = 0;
                            break;
                        case BasicInstruction::OEQ:
                        case BasicInstruction::UEQ:
                            ans = imm1 == imm2;
                            break;
                        case BasicInstruction::OGT:
                        case BasicInstruction::UGT:
                            ans = imm1 > imm2;
                            break;
                        case BasicInstruction::OGE:
                        case BasicInstruction::UGE:
                            ans = imm1 >= imm2;
                            break;
                        case BasicInstruction::OLT:
                        case BasicInstruction::ULT:
                            ans = imm1 < imm2;
                            break;
                        case BasicInstruction::OLE:
                        case BasicInstruction::ULE:
                            ans = imm1 <= imm2;
                            break;
                        case BasicInstruction::ONE:
                        case BasicInstruction::UNE:
                            ans = imm1 != imm2;
                            break;
                        case BasicInstruction::ORD:
                            ans = 1;
                            break;
                        case BasicInstruction::UNO:
                            ans = 1;
                            break;
                        case BasicInstruction::TRUE:
                            ans = 1;
                            break;
                        default:
                            break;
                        }
                        replacemap[resultop] = new ImmI32Operand(ans);
                    }
                }
            }
        }
        for (auto [id, bb] : *C->block_map) {
            auto tmp_Instruction_list = bb->Instruction_list;
            bb->Instruction_list.clear();
            for (auto &I : tmp_Instruction_list) {
                if (EraseSet.find(I) != EraseSet.end()) {
                    // I->PrintIR(std::cerr);
                    continue;
                }
                auto opvec = I->GetNonResultOperands();
                // if(I->GetResult()!=nullptr && I->GetResult()->GetFullName() == "%r45"){
                //     I->PrintIR(std::cerr);
                // }
                for(auto &op : opvec){
                    if(op->GetOperandType() == BasicOperand::REG && replacemap.find(op) != replacemap.end()){
                        op = replacemap[op];
                    }
                }
                I->SetNonResultOperands(opvec);
                
                bb->InsertInstruction(1, I);
            }
        }
    }
}

// 将如下模式进行合并：
//   %t1 = mul i32 %x, -1
//   %t2 = add i32 %t1, 2147483647
// =>
//   %t2 = sub i32 2147483647, %x
static void NegMulAddToSub(CFG *C) {
    ScalarEvolution* SE = C->getSCEVInfo();
    if (!SE) return;

    struct TransformItem {
        Instruction addI;
        Instruction mulI;
        Operand x_reg;     // 被取负的寄存器 %x
        Operand base_op;   // add 的另一个操作数（可以是寄存器或立即数）
    };
    std::vector<TransformItem> worklist;

    // 收集所有满足条件的 add
    for (auto &[id, bb] : *C->block_map) {
        for (auto I : bb->Instruction_list) {
            if (!I || I->GetOpcode() != BasicInstruction::ADD) continue;
            auto addAI = (ArithmeticInstruction*)I;
            if (addAI->GetDataType() != BasicInstruction::I32) continue;

            Operand a1 = addAI->GetOperand1();
            Operand a2 = addAI->GetOperand2();

            auto try_collect = [&](Operand regTmp, Operand base_op) -> bool {
                if (!regTmp || regTmp->GetOperandType() != BasicOperand::REG) return false;
                Instruction defI = SE->getDef(regTmp);
                if (!defI || defI->GetOpcode() != BasicInstruction::MUL) return false;
                auto mulAI = (ArithmeticInstruction*)defI;
                if (mulAI->GetDataType() != BasicInstruction::I32) return false;

                Operand m1 = mulAI->GetOperand1();
                Operand m2 = mulAI->GetOperand2();
                Operand x_reg = nullptr;
                bool is_neg1_mul = false;
                if (m1 && m1->GetOperandType() == BasicOperand::REG &&
                    m2 && m2->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)m2)->GetIntImmVal() == -1) {
                    x_reg = m1; is_neg1_mul = true;
                } else if (m2 && m2->GetOperandType() == BasicOperand::REG &&
                           m1 && m1->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)m1)->GetIntImmVal() == -1) {
                    x_reg = m2; is_neg1_mul = true;
                }
                if (!is_neg1_mul || !x_reg) return false;

                int mul_res_regno = -1;
                if (mulAI->GetResult() && mulAI->GetResult()->GetOperandType() == BasicOperand::REG) {
                    mul_res_regno = ((RegOperand*)mulAI->GetResult())->GetRegNo();
                } else { return false; }

                int use_cnt = 0;
                for (auto &[bid, bblk] : *C->block_map) {
                    for (auto I2 : bblk->Instruction_list) {
                        if (!I2) continue;
                        auto uses = I2->GetUseRegno();
                        if (uses.count(mul_res_regno)) ++use_cnt;
                    }
                }
                // 允许被多处使用：只替换 add，不强制删除 mul；若仅此处使用则顺带删除 mul
                worklist.push_back({addAI, mulAI, x_reg, base_op});
                return true;
            };

            bool matched = false;
            // 1) imm(2147483647) + (-x) => 2147483647 - x
            if (a1 && a1->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)a1)->GetIntImmVal() == 2147483647 &&
                a2 && a2->GetOperandType() == BasicOperand::REG) {
                matched = try_collect(a2, a1);
            }
            if (!matched && a2 && a2->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)a2)->GetIntImmVal() == 2147483647 &&
                a1 && a1->GetOperandType() == BasicOperand::REG) {
                matched = try_collect(a1, a2);
            }

            // 2) y + (-x) => y - x （一般情形）
            if (!matched && a1 && a1->GetOperandType() == BasicOperand::REG && a2 && a2->GetOperandType() == BasicOperand::REG) {
                // a1 + (def as neg) a2
                matched = try_collect(a2, a1);
                if (!matched) {
                    // (def as neg) a1 + a2
                    matched = try_collect(a1, a2);
                }
            }
        }
    }

    // 应用变换：add -> sub，删除 mul
    std::unordered_set<Instruction> removed;
    for (auto &item : worklist) {
        if (removed.count(item.mulI)) continue;

        // 替换 add
        auto addAI = (ArithmeticInstruction*)item.addI;
        Operand sub_res = addAI->GetResult();
        auto *subI = new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::SUB,
                                               BasicInstruction::I32,
                                               item.base_op,
                                               item.x_reg,
                                               sub_res);

        // 在所在基本块中用 sub 替换 add
        for (auto &[id, bb] : *C->block_map) {
            for (auto it = bb->Instruction_list.begin(); it != bb->Instruction_list.end(); ++it) {
                if (*it == item.addI) {
                    *it = subI;
                    break;
                }
            }
        }

        // 若 mul 结果的使用已减少到 0，则删除 mul 指令
        int mul_res_regno = -1;
        auto mulAI = (ArithmeticInstruction*)item.mulI;
        if (mulAI->GetResult() && mulAI->GetResult()->GetOperandType() == BasicOperand::REG) {
            mul_res_regno = ((RegOperand*)mulAI->GetResult())->GetRegNo();
        }
        if (mul_res_regno != -1) {
            int use_cnt = 0;
            for (auto &[bid, bblk] : *C->block_map) {
                for (auto I2 : bblk->Instruction_list) {
                    if (!I2) continue;
                    auto uses = I2->GetUseRegno();
                    if (uses.count(mul_res_regno)) ++use_cnt;
                }
            }
            if (use_cnt == 0) {
                for (auto &[id2, bb2] : *C->block_map) {
                    for (auto it = bb2->Instruction_list.begin(); it != bb2->Instruction_list.end(); ++it) {
                        if (*it == item.mulI) {
                            bb2->Instruction_list.erase(it);
                            removed.insert(item.mulI);
                            break;
                        }
                    }
                    if (removed.count(item.mulI)) break;
                }
            }
        }
    }
}

/*eliminate the instructions like
%rx = %ry + 0(replace all the use of %rx with %ry) %ry can be i32 or float
%rx = %ry - 0(replace all the use of %rx with %ry) %ry can be i32 or float
%rx = %ry * 1(replace all the use of %rx with %ry) %ry can be i32 or float
%rx = %ry / 1(replace all the use of %rx with %ry) %ry can be i32 or float

I->ReplaceRegByMap(), I->GetNonResultOperands(), I->SetNonResultOperands() is Useful
*/
void PeepholePass::SrcEqResultInstEliminate(CFG* C) {
	std::unordered_map<int,Operand> regno_map; // rx_regno -> ry_regno
    //冗余指令删除与信息记录
    for(auto &[id,block]:*(C->block_map)){
        for(auto it=block->Instruction_list.begin(); it!=block->Instruction_list.end();){
            auto inst=*it;
            if(inst->GetOpcode() == BasicInstruction::ADD){
                auto arith_inst = (ArithmeticInstruction*)inst;
                auto result_op = arith_inst->GetResult();
                auto op1 = arith_inst->GetOperand1();
                auto op2 = arith_inst->GetOperand2();
                if(op1->GetOperandType() == BasicOperand::REG && op2->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)op2)->GetIntImmVal() == 0){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op1;
                    it = block->Instruction_list.erase(it);
                    continue;
                }else if(op1->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)op1)->GetIntImmVal() == 0 && op2->GetOperandType() == BasicOperand::REG){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op2;
                    it = block->Instruction_list.erase(it);
                    continue;
                }
                else{
                    ++it;
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::FADD){
                auto farith_inst = (ArithmeticInstruction*)inst;
                auto result_op = farith_inst->GetResult();
                auto op1 = farith_inst->GetOperand1();
                auto op2 = farith_inst->GetOperand2();
                if(op1->GetOperandType() == BasicOperand::REG && op2->GetOperandType() == BasicOperand::IMMF32 && ((ImmF32Operand*)op2)->GetFloatVal() == 0.0f){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op1;
                    it = block->Instruction_list.erase(it);
                    continue;
                }else if(op1->GetOperandType() == BasicOperand::IMMF32 && ((ImmF32Operand*)op1)->GetFloatVal() == 0.0f && op2->GetOperandType() == BasicOperand::REG){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op2;
                    it = block->Instruction_list.erase(it);
                    continue;
                }
                else{
                    ++it;
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::SUB){
                auto arith_inst = (ArithmeticInstruction*)inst;
                auto result_op = arith_inst->GetResult();
                auto op1 = arith_inst->GetOperand1();
                auto op2 = arith_inst->GetOperand2();
                if(op1->GetOperandType() == BasicOperand::REG && op2->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)op2)->GetIntImmVal() == 0){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op1;
                    it = block->Instruction_list.erase(it);
                    continue;
                }else{
                    ++it;
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::FSUB){
                auto farith_inst = (ArithmeticInstruction*)inst;
                auto result_op = farith_inst->GetResult();
                auto op1 = farith_inst->GetOperand1();
                auto op2 = farith_inst->GetOperand2();
                if(op1->GetOperandType() == BasicOperand::REG && op2->GetOperandType() == BasicOperand::IMMF32 && ((ImmF32Operand*)op2)->GetFloatVal() == 0.0f){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op1;
                    it = block->Instruction_list.erase(it);
                    continue;
                }else{
                    ++it;
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::MUL){
                auto arith_inst = (ArithmeticInstruction*)inst;
                auto result_op = arith_inst->GetResult();   
                auto op1 = arith_inst->GetOperand1();
                auto op2 = arith_inst->GetOperand2();
                if(op1->GetOperandType() == BasicOperand::REG && op2->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)op2)->GetIntImmVal() == 1){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op1;
                    it = block->Instruction_list.erase(it);
                    continue;
                }else if(op1->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)op1)->GetIntImmVal() == 1 && op2->GetOperandType() == BasicOperand::REG){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op2;
                    it = block->Instruction_list.erase(it);
                    continue;
                }else{  
                    ++it;
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::FMUL){
                auto farith_inst = (ArithmeticInstruction*)inst;
                auto result_op = farith_inst->GetResult();   
                auto op1 = farith_inst->GetOperand1();
                auto op2 = farith_inst->GetOperand2();
                if(op1->GetOperandType() == BasicOperand::REG && op2->GetOperandType() == BasicOperand::IMMF32 && ((ImmF32Operand*)op2)->GetFloatVal() == 1.0f){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op1;
                    it = block->Instruction_list.erase(it);
                    continue;
                }else if(op1->GetOperandType() == BasicOperand::IMMF32 && ((ImmF32Operand*)op1)->GetFloatVal() == 1.0f && op2->GetOperandType() == BasicOperand::REG){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op2;
                    it = block->Instruction_list.erase(it);
                    continue;
                }else{  
                    ++it;
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::DIV){
                auto arith_inst = (ArithmeticInstruction*)inst;
                auto result_op = arith_inst->GetResult();
                auto op1 = arith_inst->GetOperand1();
                auto op2 = arith_inst->GetOperand2();
                if(op1->GetOperandType() == BasicOperand::REG && op2->GetOperandType() == BasicOperand::IMMI32 && ((ImmI32Operand*)op2)->GetIntImmVal() == 1){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op1;
                    it = block->Instruction_list.erase(it);
                    continue;
                }else{  
                    ++it;
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::FDIV){
                auto farith_inst = (ArithmeticInstruction*)inst;
                auto result_op = farith_inst->GetResult();
                auto op1 = farith_inst->GetOperand1();
                auto op2 = farith_inst->GetOperand2();
                if(op1->GetOperandType() == BasicOperand::REG && op2->GetOperandType() == BasicOperand::IMMF32 && ((ImmF32Operand*)op2)->GetFloatVal() == 1.0f){
                    regno_map[((RegOperand*)result_op)->GetRegNo()] = op1;
                    it = block->Instruction_list.erase(it);
                    continue;
                }else{
                    ++it;
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::GETELEMENTPTR){//如果将普通元素的ptr替换为数组基址的ptr，是否含义变了？
                // //%r34 = getelementptr i32, ptr @sorted_array, i32 0
                // //%r10 = getelementptr [10000 x i32], ptr @sorted_array, i32 0
                // //均可用@sorted_array直接替换
                // auto gep_inst=(GetElementptrInstruction*)inst;
                // auto indexes=gep_inst->GetIndexes();
                // if(indexes.size()==1&&indexes[0]->GetFullName()=="0"){
                //     Operand ptr=gep_inst->GetPtrVal();
                //     int res_regno=((RegOperand*)gep_inst->GetResult())->GetRegNo();
                //     regno_map[res_regno] = ptr;
                //     it = block->Instruction_list.erase(it);
                //     continue;
                // }else{
                //     ++it;
                //     continue;
                // }
            }
            ++it;
        }   
    }

    //后续指令的reg替换
    for(auto &[id, block]:*(C->block_map)){
        for(auto &inst : block->Instruction_list){
            auto operands = inst->GetNonResultOperands();
            for(auto &op : operands){
                if(op->GetOperandType() == BasicOperand::REG){
                    int regno = ((RegOperand*)op)->GetRegNo();
                    if(regno_map.count(regno)){
                        Operand op_torpl = regno_map[regno];
                        while(op_torpl->GetOperandType()==BasicOperand::REG) {
                            int new_regno=((RegOperand*)op_torpl)->GetRegNo();
                            if(regno_map.count(new_regno)){
                                op_torpl = regno_map[new_regno];
                            }else{
                                break;
                            }
                        }
                        op = op_torpl->Clone();
                    }
                }
            }
            inst->SetNonResultOperands(operands);
        }
    }
}



void PeepholePass::DeadArgElim() {
	// find function with dead args
	// deadArg_funcs.name() -> deadArg_funcs.left_arg_id
	std::unordered_map<std::string, std::vector<int>> deadArg_funcs;
	for (auto [defI, cfg] : llvmIR->llvm_cfg) {
		// formals_regNo -> formal_id = arg_id
		std::unordered_map<int, int> formals_reg_map;
		std::unordered_set<int> argReg_occured;
		for(int i = 0; i < defI->formals.size(); i++) {
			int curr_regNo = ((RegOperand*)defI->formals_reg[i])->GetRegNo();
			formals_reg_map[curr_regNo] = i;
		}
		for(auto [bbid, bb] : *(cfg->block_map)) {
			for(auto inst : bb->Instruction_list) {
				for(int useRegno : inst->GetUseRegno()) {
					if(formals_reg_map.empty()) break;
					if(formals_reg_map.count(useRegno)) {
						argReg_occured.insert(useRegno);
						formals_reg_map.erase(useRegno);
					}
				}
			}
		}

		if(!formals_reg_map.empty()) { // not all args have been used.
			std::string func_name = defI->GetFunctionName();
			// std::cerr << "fucntion: " + func_name + "not all args have been used.\n";
			deadArg_funcs[func_name] = std::vector<int> {};
			std::vector<BasicInstruction::LLVMType> new_formals;
    		std::vector<Operand> new_formals_reg;

			for(int i = 0; i < defI->formals.size(); i++) {
				int curr_regNo = ((RegOperand*)defI->formals_reg[i])->GetRegNo();
				if(argReg_occured.count(curr_regNo)) {
					new_formals.push_back(defI->formals[i]);
					new_formals_reg.push_back(defI->formals_reg[i]);
					deadArg_funcs[func_name].push_back(i);
				}
			}

			defI->formals = new_formals;
			defI->formals_reg = new_formals_reg;
		}
    }

	// change all deadArg_funcs call
	for (auto [defI, cfg] : llvmIR->llvm_cfg) {
		for(auto [bbid, bb] : *(cfg->block_map)) {
			for(auto inst : bb->Instruction_list) {
				if (auto call_inst = dynamic_cast<CallInstruction*>(inst)) {
					std::string call_func_name = call_inst->GetFunctionName();
					if(deadArg_funcs.count(call_func_name)) {
						typedef std::vector<std::pair<BasicInstruction::LLVMType, Operand>> argList;
						argList new_args;
						argList args = call_inst->GetParameterList();
						for(int left_arg_id : deadArg_funcs[call_func_name]) {
							new_args.push_back(args[left_arg_id]);
						}
						call_inst->SetParameterList(new_args);
					}
				}
			}
		}
	}
}


/*
%rx = %ry - %ry   ---> %rx = 0
%rx = %ry / %ry   ---> %rx = 1
%rx = %ry % %ry   ---> %rx = 0
*/
bool Op1EqOp2(ArithmeticInstruction* inst){
    auto op1=inst->GetOperand1();
    auto op2=inst->GetOperand2();
    auto op1_type=op1->GetOperandType();
    auto op2_type=op2->GetOperandType();
    if(op1_type!=op2_type){ return false; }

    if(op1_type==BasicOperand::REG){
        int regno1=((RegOperand*)op1)->GetRegNo();
        int regno2=((RegOperand*)op2)->GetRegNo();
        if(regno1==regno2){return true;}

    }else if(op1_type==BasicOperand::IMMI32){
        int value1=((ImmI32Operand*)op1)->GetIntImmVal();
        int value2=((ImmI32Operand*)op2)->GetIntImmVal();
        if(value1==value2){return true;}

    }else if(op1_type==BasicOperand::IMMF32){
        float value1=((ImmF32Operand*)op1)->GetFloatVal();
        float value2=((ImmF32Operand*)op2)->GetFloatVal();
        if(value1==value2){return true;}
    }
    return false;
}

void PeepholePass::IdentitiesEliminate(CFG* C){
    std::unordered_map<int,Operand> regno_map; // rx_regno -> result_operand
    //冗余指令删除与信息记录
    for(auto &[id,block]:*(C->block_map)){
        for(auto it=block->Instruction_list.begin(); it!=block->Instruction_list.end();){
            auto inst=*it;
            if(inst->GetOpcode() == BasicInstruction::SUB){
                if(Op1EqOp2((ArithmeticInstruction*)inst)){
                    int res_regno=inst->GetDefRegno();
                    regno_map[res_regno]=new ImmI32Operand(0);
                    it = block->Instruction_list.erase(it);
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::DIV){
                if(Op1EqOp2((ArithmeticInstruction*)inst)){
                    int res_regno=inst->GetDefRegno();
                    regno_map[res_regno]=new ImmI32Operand(1);
                    it = block->Instruction_list.erase(it);
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::MOD){
                if(Op1EqOp2((ArithmeticInstruction*)inst)){
                    int res_regno=inst->GetDefRegno();
                    regno_map[res_regno]=new ImmI32Operand(0);
                    it = block->Instruction_list.erase(it);
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::FSUB){
                if(Op1EqOp2((ArithmeticInstruction*)inst)){
                    int res_regno=inst->GetDefRegno();
                    regno_map[res_regno]=new ImmF32Operand(0);
                    it = block->Instruction_list.erase(it);
                    continue;
                }
            }else if(inst->GetOpcode() == BasicInstruction::FDIV){
                if(Op1EqOp2((ArithmeticInstruction*)inst)){
                    int res_regno=inst->GetDefRegno();
                    regno_map[res_regno]=new ImmF32Operand(1);
                    it = block->Instruction_list.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    //后续指令的reg替换
    for(auto &[id, block]:*(C->block_map)){
        for(auto &inst : block->Instruction_list){
            auto operands = inst->GetNonResultOperands();
            for(auto &op : operands){
                if(op->GetOperandType() == BasicOperand::REG){
                    int regno = ((RegOperand*)op)->GetRegNo();
                    if(regno_map.count(regno)){
                        Operand op_torpl = regno_map[regno];
                        while(op_torpl->GetOperandType()==BasicOperand::REG) {
                            int new_regno=((RegOperand*)op_torpl)->GetRegNo();
                            if(regno_map.count(new_regno)){
                                op_torpl = regno_map[new_regno];
                            }else{
                                break;
                            }
                        }
                        op = op_torpl->Clone();
                    }
                }
            }
            inst->SetNonResultOperands(operands);
        }
    }
}