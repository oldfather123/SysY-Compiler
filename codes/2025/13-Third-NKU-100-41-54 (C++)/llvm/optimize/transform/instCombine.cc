#include "instCombine.h"

void InstCombinePass::Execute(){
    // for(auto &[defI,cfg]:llvmIR->llvm_cfg){
    //     for (auto [id, bb] : *(cfg->block_map)) {
    //         bool changed = true;
    //         while (changed) {
    //             changed = false;
    //             for (auto it = bb->Instruction_list.begin(); it != bb->Instruction_list.end(); ++it) {
    //                 changed |= ApplyRules(cfg, bb->Instruction_list, it);
    //             }
    //         }
    //     }
    // }

    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        for(auto &[id,block]:*(cfg->block_map)){
            AlgebraSimplify1(block);
            AlgebraSimplify2(block);
        }
    }
    
}

int GetOpRegNo(ArithmeticInstruction* inst, int pos){
    if(pos==1){
        if(inst->GetOperand1()->GetOperandType()==BasicOperand::REG){
            return ((RegOperand*)inst->GetOperand1())->GetRegNo();
        }else{
            return -1;
        }
    }else if(pos==2){
        if(inst->GetOperand2()->GetOperandType()==BasicOperand::REG){
            return ((RegOperand*)inst->GetOperand2())->GetRegNo();
        }else{
            return -1;
        }
    }else{
        assert(0);
    }
}


/*
res = a-(a+b)+b   ==>  res = 0
    %r405 = add i32 %r348,%r373           %r405 = add i32 %r348,%r373
    %r400 = sub i32 %r348,%r405
    %r401 = add i32 %r400,%r373    ==> 
    %r402 = sub i32 %r401,%r405           %r402 = sub i32 0 ,%r405
*/

void InstCombinePass::AlgebraSimplify1(LLVMBlock block){
    auto old_block=block->Instruction_list;
    block->Instruction_list.clear();
    auto it=old_block.begin();
    while(it!=old_block.end()){
        auto inst1=*it;
        if(inst1->GetOpcode()==BasicInstruction::ADD){
            //一次取4条指令
            auto it2 = std::next(it);
            auto it3 = (it2 != old_block.end()) ? std::next(it2) : old_block.end();
            auto it4 = (it3 != old_block.end()) ? std::next(it3) : old_block.end();
            if(it2!=old_block.end() && it3!=old_block.end() && it4!=old_block.end()){
                auto inst2=*it2; auto inst3=*it3; auto inst4=*it4;
                if(inst2->GetOpcode()==BasicInstruction::SUB &&
                   inst3->GetOpcode()==BasicInstruction::ADD &&
                   inst4->GetOpcode()==BasicInstruction::SUB &&
                   GetOpRegNo((ArithmeticInstruction*)inst1,1)==GetOpRegNo((ArithmeticInstruction*)inst2,1) &&
                   GetOpRegNo((ArithmeticInstruction*)inst1,2)==GetOpRegNo((ArithmeticInstruction*)inst3,2) &&
                   GetOpRegNo((ArithmeticInstruction*)inst2,2)==inst1->GetDefRegno() &&
                   GetOpRegNo((ArithmeticInstruction*)inst3,1)==inst2->GetDefRegno() &&
                   GetOpRegNo((ArithmeticInstruction*)inst4,1)==inst3->GetDefRegno()){

                    block->Instruction_list.push_back(inst1);
                    auto last_inst = (ArithmeticInstruction*)inst4;
                    last_inst->SetOperand1(new ImmI32Operand(0));
                    block->Instruction_list.push_back(last_inst);
                    it = std::next(it4);
                    continue;
                }
            }
        } 
        block->Instruction_list.push_back(inst1);
        ++it;
    }
}

/*
res = (0-a)+b   ==>  res = b-a
    %r402 = sub i32 0,%r405
   (%r410 = add i32 %r348,%r373 )  =>  (%r410 = add i32 %r348,%r373)
    %r420 = add i32 %r402,%r410         %r420 = sub i32 %r410,%r405 
*/

void InstCombinePass::AlgebraSimplify2(LLVMBlock block){
    auto old_block=block->Instruction_list;
    block->Instruction_list.clear();
    auto it=old_block.begin();
    while(it!=old_block.end()){
        auto inst1=*it;
        if(inst1->GetOpcode()==BasicInstruction::SUB && GetOpRegNo((ArithmeticInstruction*)inst1,1)==-1){
            //一次取2~3条指令
            auto it2 = std::next(it);
            auto it3 = (it2 != old_block.end()) ? std::next(it2) : old_block.end();
            if(it2!=old_block.end() && 
                (*it2)->GetOpcode()==BasicInstruction::ADD &&
                GetOpRegNo((ArithmeticInstruction*)(*it2),1)==inst1->GetDefRegno()){
                    auto ainst1=(ArithmeticInstruction*)inst1;
                    auto ainst2=(ArithmeticInstruction*)(*it2);
                    if((ainst1)->GetOperand1()->GetOperandType()==BasicOperand::IMMI32 &&
                        GetOpRegNo(ainst1,2)!=-1){
                        auto op1=(ainst1)->GetOperand1();
                        if(((ImmI32Operand*)op1)->GetIntImmVal()==0){
                            auto sub_op1=ainst2->GetOperand2();
                            auto sub_op2=ainst1->GetOperand2();
                            ainst2->SetOperand1(sub_op1->Clone());
                            ainst2->SetOperand2(sub_op2->Clone());
                            ainst2->SetOpcode(BasicInstruction::LLVMIROpcode::SUB);
                            block->Instruction_list.push_back(ainst2);
                            it=std::next(it2);
                            continue;
                        }
                    }
            }else if(it2!=old_block.end() && it3!=old_block.end() &&
                    (*it3)->GetOpcode()==BasicInstruction::ADD &&
                    GetOpRegNo((ArithmeticInstruction*)(*it3),1)==inst1->GetDefRegno()){
                    auto ainst1=(ArithmeticInstruction*)inst1;
                    auto ainst3=(ArithmeticInstruction*)(*it3);
                    if((ainst1)->GetOperand1()->GetOperandType()==BasicOperand::IMMI32 &&
                        GetOpRegNo(ainst1,2)!=-1){
                        auto op1=(ainst1)->GetOperand1();
                        if(((ImmI32Operand*)op1)->GetIntImmVal()==0){
                            auto sub_op1=ainst3->GetOperand2();
                            auto sub_op2=ainst1->GetOperand2();
                            ainst3->SetOperand1(sub_op1->Clone());
                            ainst3->SetOperand2(sub_op2->Clone());
                            ainst3->SetOpcode(BasicInstruction::LLVMIROpcode::SUB);
                            block->Instruction_list.push_back(*it2);
                            block->Instruction_list.push_back(ainst3);
                            it=std::next(it3);
                            continue;
                        }
                    }
            }
            
        } 
        block->Instruction_list.push_back(inst1);
        ++it;
    }
}