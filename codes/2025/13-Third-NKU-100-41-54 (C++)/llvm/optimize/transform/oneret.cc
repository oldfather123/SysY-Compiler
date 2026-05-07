#include "oneret.h"

//除了main函数外，其余函数只能有一个ret块

void OneRetPass::Execute(){
    for(auto &[defI,bbmap]:llvmIR->function_block_map){
        if(defI->GetFunctionName()!="main"){
            //【1】遍历函数，记录ret的block_id和operand 
            int ret_num=0;
            BasicInstruction::LLVMType ret_type=BasicInstruction::LLVMType::VOID;
            for(auto &[id,block]:bbmap){
                auto last_inst = block->Instruction_list.back();
                if(last_inst->GetOpcode()==BasicInstruction::LLVMIROpcode::RET){
                    RetInstruction* ret = (RetInstruction*)last_inst;
                    ++ret_num;
                    ret_type=ret->GetType();
                    if(ret_type!=BasicInstruction::LLVMType::VOID){
                        Operand retval=ret->GetRetVal();
                        auto phi_pair=std::make_pair(GetNewLabelOperand(id),retval->Clone());
                        ret_phi_list[defI].push_back(phi_pair);
                    }
                }
            }
            //【2】统一exit block:当ret不止一个时
            if(ret_num>1){
                int exit_block_id=(++llvmIR->function_max_label[defI]);
                //【2.1】将所有原ret改为br_uncond
                for(auto &[id,block]:bbmap){
                    auto last_inst = block->Instruction_list.back();
                    if(last_inst->GetOpcode()==BasicInstruction::LLVMIROpcode::RET){
                        //删除ret，添加br_uncond
                        block->Instruction_list.pop_back();
                        BrUncondInstruction* br=new BrUncondInstruction(GetNewLabelOperand(exit_block_id));
                        block->Instruction_list.push_back(br);
                    }
                }
                //【2.2】设置统一的exit_block
                LLVMBlock exit_block=new BasicBlock(exit_block_id);
                bbmap[exit_block_id]=exit_block;
                //若 !ret_void，需添加phi指令
                if(ret_type==BasicInstruction::LLVMType::VOID){
                    exit_block->Instruction_list.push_back(new RetInstruction(BasicInstruction::LLVMType::VOID,nullptr));
                }else{
                    llvmIR->function_max_reg[defI]+=1;
                    int ret_regno=llvmIR->function_max_reg[defI];
                    PhiInstruction* phi=new PhiInstruction(ret_type,GetNewRegOperand(ret_regno),ret_phi_list[defI]);
                    exit_block->Instruction_list.push_back(phi);
                    exit_block->Instruction_list.push_back(new RetInstruction(ret_type,GetNewRegOperand(ret_regno)));
                }
            }
        }
    }
}