#include "functioninline.h"
#include <functional>

/*
For : 记录调用图callGraph和函数大小funcSize
    - callGraph构建
    - calleeReturn: block_id --> RetInstruction
    - funcSize
*/
void FunctionInlinePass::buildCallGraph(FuncDefInstruction caller) {
    //for (auto &func : llvmIR->function_block_map) {
        //FuncDefInstruction caller = func.first;//调用者
        auto func = llvmIR->function_block_map[caller];
        for (auto &block : func) {
            for (auto &inst : block.second->Instruction_list) {
                if (inst->GetOpcode() == BasicInstruction::LLVMIROpcode::CALL) {
                    CallInstruction* callInst = (CallInstruction*)inst;
                    FuncDefInstruction callee = llvmIR->FunctionNameTable[callInst->GetFunctionName()];//记录call中func name 对应函数的FuncDefInstruction
                    callGraph[caller].insert(callee);//记录调用关系
                }else if(inst->GetOpcode() == BasicInstruction::LLVMIROpcode::RET){
                    RetInstruction* retInst = (RetInstruction*)inst;
                    auto id_ret = std::make_pair(block.first, retInst);
                    calleeReturn[caller] = id_ret;//记录被调用函数的ret block id与ret inst 【需更新】
                }else if(inst->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI){
                    PhiInstruction *phi = (PhiInstruction*)inst;
                    auto phi_list = phi->GetPhiList();
                    for(auto &[label,reg]:phi_list){
                        int label_no=((LabelOperand*)label)->GetLabelNo();
                        phiGraph[caller][label_no].insert(phi); // 为phi指令记录block_id 与以此为源的phi insts
                    }
                }
            }
        }
    //}
}

bool FunctionInlinePass::shouldInline(FuncDefInstruction caller, FuncDefInstruction callee) {
    // 如果函数太大，不进行内联
    if (funcSize[callee] > INLINE_THRESHOLD) {
        return false;
    }
    // 如果存在递归调用，不进行内联
    if(caller->GetFunctionName()==callee->GetFunctionName()){
        return false;
    }
    std::set<FuncDefInstruction> visited;
    std::function<bool(FuncDefInstruction)> hasRecursion = [&](FuncDefInstruction func) {
        if (visited.count(func)) return true;//直接调用自己
        visited.insert(func);
        for (auto &called : callGraph[func]) {//间接调用自己
            if (hasRecursion(called)) return true;
        }
        visited.erase(func);
        return false;
    };
    return !hasRecursion(callee);
}

/*
若已存在映射，取
否则，新建
*/
int FunctionInlinePass::renameRegister(FuncDefInstruction caller,int oldReg, std::unordered_map<int, int>& regMapping) {
    if (regMapping.count(oldReg)) {
        return regMapping[oldReg];
    }
    llvmIR->function_max_reg[caller]+=1;//必须超过调用者的regno范围
    int newReg = llvmIR->function_max_reg[caller];
    regMapping[oldReg] = newReg;
    return newReg;
}

int FunctionInlinePass::renameLabel(FuncDefInstruction caller,int oldLabel, std::unordered_map<int, int>& labelMapping) {
    if (labelMapping.count(oldLabel)) {
        return labelMapping[oldLabel];
    }
    llvmIR->function_max_label[caller]+=1;
    int newLabel = llvmIR->function_max_label[caller];//必须超过调用者的label范围
    labelMapping[oldLabel] = newLabel;
    return newLabel;
}

LLVMBlock FunctionInlinePass::copyBasicBlock(FuncDefInstruction caller, LLVMBlock origBlock, std::unordered_map<int, int>& regMapping, std::unordered_map<int, int>& labelMapping) {
    int newLabel = renameLabel(caller,origBlock->block_id, labelMapping);
    LLVMBlock newBlock = new BasicBlock(newLabel);
    //std::cout<<" inline block "<<origBlock->block_id<<" into block "<<newLabel<<std::endl;
    
    //newBlock->comment = callee->GetFunctionName()+"__L"+ std::to_string(origBlock->block_id) +"   " + origBlock->comment;
    // 复制指令并重命名寄存器和标签
    for (auto &inst : origBlock->Instruction_list) {
        Instruction newInst = inst->Clone();
        // 重命名结果寄存器
        if (inst->GetResult()&& inst->GetResult()->GetOperandType() == BasicOperand::REG) {
            int ResRegNo =((RegOperand*)(inst->GetResult()))->GetRegNo();
            Operand newResOp = GetNewRegOperand(renameRegister(caller,ResRegNo, regMapping));
            newInst->SetResult(newResOp);
        }
        // 重命名操作数寄存器
        auto OldOperands = newInst->GetNonResultOperands();
        std::vector<Operand> nonResultOps{};
        nonResultOps.reserve(OldOperands.size());
        for (auto &op : OldOperands) {
            if (op->GetOperandType() == BasicOperand::REG) {//reg
                int oldReg = ((RegOperand*)op)->GetRegNo();
                Operand newOp = GetNewRegOperand(renameRegister(caller,oldReg, regMapping));
                nonResultOps.emplace_back(newOp);
            } else if (op->GetOperandType() == BasicOperand::LABEL) {//label（主要是br的）
                int oldLabel = ((LabelOperand*)op)->GetLabelNo();
                Operand newOp = GetNewLabelOperand(renameLabel(caller,oldLabel, labelMapping));
                nonResultOps.emplace_back(newOp);
            }else{//imm
                nonResultOps.emplace_back(op);
            }
        }
        newInst->SetNonResultOperands(nonResultOps);
        //重命名phi指令的源label
        if(newInst->GetOpcode()==BasicInstruction::LLVMIROpcode::PHI){
            auto phi_list=((PhiInstruction*)newInst)->GetPhiList();
            for(int i=0;i<phi_list.size();i++){
                int ori_label=((LabelOperand*)(phi_list[i].first))->GetLabelNo();
                Operand new_label=GetNewLabelOperand(renameLabel(caller,ori_label,labelMapping));
                auto new_phi_pair=std::make_pair(new_label,phi_list[i].second);
                ((PhiInstruction*)newInst)->ChangePhiPair(i,new_phi_pair);
            }
        }
        newBlock->Instruction_list.push_back(newInst);
    }
    return newBlock;
}


void FunctionInlinePass::inlineFunction(int callerBlockId, LLVMBlock callerBlock,FuncDefInstruction caller, FuncDefInstruction callee, CallInstruction* callInst) {
    std::unordered_map<int, int> regMapping;
    std::unordered_map<int, int> labelMapping;
    
    // 【2】校正首尾对应关系
    // 【2.1】将被调函数形参的regno替换为调用者实参中的regno
    std::deque<Instruction> fparams_insts;//若call时用imm传实参，则通过add指令先放到寄存器里
    auto rargs = callInst->GetNonResultOperands();//实参列表
    auto fargs = callee->GetNonResultOperands();//形参列表
    for (int i=0;i< rargs.size(); i++) {
        int f_paramReg =  ((RegOperand*)fargs[i])->GetRegNo();//形参regno

        if (rargs[i]->GetOperandType() == BasicOperand::REG) {//实参为reg
            int r_paramReg = ((RegOperand*)rargs[i])->GetRegNo();
            regMapping[f_paramReg] = r_paramReg;
        }else{//实参为imm，通过add指令转换为reg
            Instruction add_inst;
            int result_regno=(++llvmIR->function_max_reg[caller]);//必须超过调用者的regno范围
            RegOperand* paramReg= GetNewRegOperand(result_regno);
            if(rargs[i]->GetOperandType() == BasicOperand::IMMI32){
                add_inst=new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::ADD,BasicInstruction::LLVMType::I32,
                         rargs[i],new ImmI32Operand(0),paramReg);
            }else if(rargs[i]->GetOperandType() == BasicOperand::IMMF32){
                add_inst=new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::FADD,BasicInstruction::LLVMType::FLOAT32,
                         rargs[i],new ImmF32Operand(0),paramReg);
            }
            fparams_insts.push_back(add_inst);
            regMapping[f_paramReg] = result_regno;
        }
    }

    // 【1】复制被调用函数的所有基本块
    for (auto &block : llvmIR->function_block_map[callee]){
        LLVMBlock newblock =copyBasicBlock(caller,block.second, regMapping, labelMapping);
        llvmIR->function_block_map[caller][newblock->block_id]=newblock;
    }
    
    // 【2】校正首尾对应关系
    // 【2.2】修改调用指令所在基本块的跳转关系  caller_block ---> Inline_Function_EntryBlock
    auto temp_list = callerBlock->Instruction_list;
    callerBlock->Instruction_list.clear();
    std::deque<Instruction> caller_back_list;//callInstruction所在block的后半段
    for(int i=0;i< temp_list.size(); i++) {
        Instruction inst = temp_list[i];
        if (inst == callInst) {
            // (1)保存结果寄存器，删除调用指令
            RegOperand * resultReg ;
            if(callInst->GetResult() && callInst->GetResult()->GetOperandType() == BasicOperand::REG) {
                resultReg = (RegOperand*)callInst->GetResult();
            } else {
                resultReg = nullptr;
            }
            // (2)添加将call中实参转换为reg类型的指令
            for(auto &add_inst:fparams_insts){
                callerBlock->Instruction_list.push_back(add_inst);
            }fparams_insts.clear();

            // (3)添加跳转到内联函数入口块的指令
            BrUncondInstruction* brInst = new BrUncondInstruction(GetNewLabelOperand(labelMapping[0]));// 0是入口块
            callerBlock->Instruction_list.push_back(brInst);
            // (4)保存后续指令，结束此块
            for(int j = i + 1; j < temp_list.size(); j++) {
                caller_back_list.push_back(temp_list[j]);
            }
            break;
        }else{
            callerBlock->Instruction_list.push_back(inst);
        }
    }

    // 【2.3】处理内联函数的返回值，合并尾块  Inline_Function_ExitBlock ---> caller_block
    auto [return_blockid,ret] = calleeReturn[callee];
    int tail_blockid=labelMapping[return_blockid];
    LLVMBlock retBlock = llvmIR->function_block_map[caller][tail_blockid];
    auto tailInst_list=retBlock->Instruction_list;//assert:最后一条指令必为ret
    //（1）删除inlined function的原ret指令
    tailInst_list.pop_back();
    //（2）将原ret指令的ret_val赋值给call_inst的result_reg（add 0)
    if(callInst->GetRetType()!=BasicInstruction::LLVMType::VOID){
        Operand RetValue;
        Operand ori_RetValue=ret->GetRetVal();
        if(ori_RetValue->GetOperandType()==BasicOperand::REG){
            int ori_regno=((RegOperand*)ori_RetValue)->GetRegNo();
            RetValue=GetNewRegOperand(regMapping[ori_regno]);
        }else{
            RetValue=ori_RetValue;
        }
        Operand CallResult = callInst->GetResult();
        int callres_regno = ((RegOperand*)CallResult)->GetRegNo();
        Instruction newAddInstr;
        if(ret->GetType()==BasicInstruction::LLVMType::I32){
            newAddInstr=new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::ADD,
                        BasicInstruction::LLVMType::I32,RetValue,new ImmI32Operand(0),GetNewRegOperand(callres_regno));
        }else if(ret->GetType()==BasicInstruction::LLVMType::FLOAT32){
            newAddInstr=new ArithmeticInstruction(BasicInstruction::LLVMIROpcode::FADD,
                        BasicInstruction::LLVMType::FLOAT32,RetValue,new ImmF32Operand(0),GetNewRegOperand(callres_regno));
        }
        tailInst_list.push_back(newAddInstr);
        // std::cout<<"function [ " <<callee->GetFunctionName()<<" ] is inlined to funciton [ "<<caller->GetFunctionName()<<
        //     " ], with ret value be assigned to callres : "<<callres_regno<<std::endl;
    }
    //（3）将原caller_block的call指令后的剩余指令添加到inlined_function的ret_block中
    for(auto &inst:caller_back_list){
        tailInst_list.push_back(inst);
    }
    retBlock->Instruction_list=tailInst_list;

    // 【2.4】处理caller后续phi指令的 源block_id
    int old_phi_ori=callerBlock->block_id;
    int new_phi_ori=labelMapping[return_blockid];
    //std::cout<<"old: "<<old_phi_ori<<" new: "<<new_phi_ori<<std::endl;
    for(auto &[id,phis]:phiGraph[caller]){
        if(id==old_phi_ori){
            //std::cout<<"change following phis: from caller's "<<old_phi_ori<<" block to new "<<new_phi_ori<<" block"<<std::endl;
            for(auto &phi:phis){
                //将source_blockid 改为新的
                auto old_phi_list=phi->GetPhiList();
                std::vector<std::pair<Operand, Operand>> new_phi_list;
                new_phi_list.reserve(old_phi_list.size());
                for(auto &phi_pair:phi->GetPhiList()){
                    if(((LabelOperand*)phi_pair.first)->GetLabelNo()==old_phi_ori){
                        new_phi_list.emplace_back(std::make_pair(GetNewLabelOperand(new_phi_ori),phi_pair.second));
                    }else{
                        new_phi_list.emplace_back(phi_pair);
                    }
                }
                phi->SetPhiList(new_phi_list);
            }
        }
    }
    
    // 同步max_label和max_reg信息到CFG对象
    if (llvmIR->llvm_cfg.find(caller) != llvmIR->llvm_cfg.end()) {
        llvmIR->llvm_cfg[caller]->max_label = llvmIR->function_max_label[caller];
        llvmIR->llvm_cfg[caller]->max_reg = llvmIR->function_max_reg[caller];
    }
}

void FunctionInlinePass::recombineGEP(){
    for(auto &[defI,cfg]:llvmIR->llvm_cfg){
        std::unordered_map<int,GetElementptrInstruction*> half_geps;//res_ptr->GEP
        std::unordered_map<int,std::unordered_set<GetElementptrInstruction*>> gep_use_map;//use_ptr->GEP

        //【1】收集信息
        for(auto &[id,block]:*(cfg->block_map)){
            for(auto &inst:block->Instruction_list){
                if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::GETELEMENTPTR){
                    GetElementptrInstruction* gep=(GetElementptrInstruction*)inst;
                    // 1.记录ptr-->gep的映射
                    Operand ptr=gep->GetPtrVal();
                    if(ptr->GetOperandType()==BasicOperand::REG){
                        int ptr_regno=((RegOperand*)ptr)->GetRegNo();
                        gep_use_map[ptr_regno].insert(gep);
                    }
                    
                    // 2.找出二次引用的数组GEP，记录res-->gep的映射
                    if((gep->GetDims().size()+1)>gep->GetIndexes().size()){
                        int res_regno=gep->GetDefRegno();
                        half_geps[res_regno]=gep;
                    }
                }
            }
        }
        //【2】重组GEP  TODO:后续要删除gep1死代码
        for(auto &[regno,gep1]:half_geps){
            auto gep2s=gep_use_map[regno];

            Operand ptr1=gep1->GetPtrVal();
            auto dims1=gep1->GetDims();
            auto indexes1=gep1->GetIndexes();

            for(auto &gep2:gep2s){
                gep2->set_dims(dims1);
                gep2->set_ptr(ptr1->Clone());
                
                std::vector<Operand> new_indexes=indexes1;
                for(auto &index2:gep2->GetIndexes()){
                    new_indexes.push_back(index2->Clone());
                }
                gep2->set_indexes(new_indexes);

            }
        }
    }
}

void FunctionInlinePass::Execute() {
    //【1】初始化：遍历所有函数，构建Graph
    for(auto &func : llvmIR->function_block_map){
        FuncDefInstruction caller = func.first;//调用者
        
        buildCallGraph(caller);

        // 计算函数大小
        int size = 0;
        for (auto &block : func.second) {
            size += block.second->Instruction_list.size();
        }
        funcSize[caller] = size;
    }
    
    //【2】执行
    bool changed;
    do {
        changed = false;
        for (auto &func : llvmIR->function_block_map) {
            FuncDefInstruction caller = func.first;
            for (auto &block : func.second) {
                auto it = block.second->Instruction_list.begin();
                while (it != block.second->Instruction_list.end()) {
                    // 如果是函数调用指令
                    if ((*it)->GetOpcode() == BasicInstruction::LLVMIROpcode::CALL) {
                        CallInstruction* callInst = (CallInstruction*)(*it);
                        if(lib_function_names.count(callInst->GetFunctionName())){//非自定义函数不Inline
                            ++it;   continue;
                        }
                        FuncDefInstruction callee = llvmIR->FunctionNameTable[callInst->GetFunctionName()];//被调用者
                        if (shouldInline(caller, callee)) {
                            //std::cout<<"Inlining function: " << callee->GetFunctionName() << " into " << caller->GetFunctionName() << std::endl;
                            int caller_blockId= block.first;
                            LLVMBlock callerBlock = block.second;
                            inlineFunction(caller_blockId,callerBlock,caller, callee, callInst);//执行内联

                            changed = true;
                            inlined_function_names.insert(callee);//标记信息

                            callGraph.erase(caller);
                            calleeReturn.erase(caller);
                            phiGraph.erase(caller);
                            buildCallGraph(caller);//重新遍历，更新图
                            break;
                        }
                    }
                    ++it;
                }
                if (changed) break;
            }
            if (changed) break;
        }
    } while (changed);



    //【3】清理llvmIR->function_block_map中被内联的函数
    auto it = llvmIR->function_block_map.begin();
    while (it != llvmIR->function_block_map.end()) {
        if (inlined_function_names.count(it->first)) {
            //std::cout<<"delete function: "<<it->first->GetFunctionName()<<std::endl;
            auto this_def=it->first;
            it = llvmIR->function_block_map.erase(it);
            llvmIR->llvm_cfg.erase(this_def);
            llvmIR->FunctionNameTable.erase(this_def->GetFunctionName());
            llvmIR->function_max_label.erase(this_def);
            llvmIR->function_max_reg.erase(this_def);
            delete this_def;
        } else {
            //std::cout<<"donot delete this function!"<<std::endl;
            ++it;
        }
    }

    //【4】重组GEP
    recombineGEP();
    
    //【5】同步max_label和max_reg信息到CFG对象
    llvmIR->SyncMaxInfo();
}
