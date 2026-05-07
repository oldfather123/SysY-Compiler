#include "adce.h"

Instruction ADCEPass::GetTerminal(CFG *C, int label){
    Instruction intr = (*C->block_map)[label]->Instruction_list.back();
    return intr;
}

void ADCEPass::ADCE(CFG *C){
    // std::cout<<" blocks rest:"<<C->block_map->size()<<" 个 。";
    // for(auto &[id, block]: *(C->block_map)){
    //     std::cout<<id<<" ";
    // }std::cout<<std::endl;

    std::set<Instruction> worklist;
    std::map<Instruction, int> intr_bbid_map; // intr --> block_id
    std::map<int, Instruction> defmap; //def_regno --> intr

    // 过滤出有用的指令
    for(int i=0; i<C->block_map->size(); i++){
        if((*C->block_map)[i]==nullptr) // 在之前的cfg基础优化中会删除块，所以要检查是否为空
            continue;
        for(auto& intr: (*C->block_map)[i]->Instruction_list){
            intr_bbid_map[intr] = i;
            // 将所有有效指令放入worklist
            if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::STORE
                || intr->GetOpcode()==BasicInstruction::LLVMIROpcode::RET
                || intr->GetOpcode()==BasicInstruction::LLVMIROpcode::CALL){
                worklist.insert(intr);
            }
            // 收集所有def
            else{
                int regno = intr->GetDefRegno();
                if(regno!=-1)
                    defmap[regno]=intr;
            }
        }
    }

    while(!worklist.empty()){
        auto iter = worklist.begin();
        Instruction intr = *iter;

        worklist.erase(iter);
        live.insert(intr); // 添加活跃指令
        live_block.insert(intr_bbid_map[intr]); // 添加活跃块

        // phi指令的每一条前驱都应该被当作活跃的:前驱block和前驱block的最后一条指令br
        if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::PHI){
            auto phiI = (PhiInstruction*)intr;
            for(auto& phi_pair: phiI->GetPhiList()){
                int label = ((LabelOperand*)phi_pair.first)->GetLabelNo();
                // std::cout<<"get terminal: "<<label<<std::endl;
                // if(C->block_map->find(label)==C->block_map->end()){
                //     std::cout<<"label not found: "<<label<<std::endl;
                //     continue;
                // }
                Instruction terminal = GetTerminal(C, label);
                if(worklist.find(terminal)==worklist.end() && live.find(terminal)==live.end()){
                    worklist.insert(terminal);
                }
                live_block.insert(label);
            }
        }

        // 加入当前块的全部控制依赖前驱（利用CDG）
        for(auto& bbid: domtrees->GetDomTree(C)->DF_map[intr_bbid_map[intr]]){
            Instruction terminal = GetTerminal(C, bbid);
            if(worklist.find(terminal)==worklist.end() && live.find(terminal)==live.end()){
                worklist.insert(terminal);
            }
        }

        // 对每个use的变量，将其def加入worklist
        std::set<int> use_list = intr->GetUseRegno();
        for(auto& use_regno: use_list){
            if(defmap.find(use_regno)!=defmap.end() && live.find(defmap[use_regno])==live.end()){
                worklist.insert(defmap[use_regno]);
            }
        }
    }
}

void ADCEPass::CleanUnlive(CFG *C){
    // 从block_map中删除不活跃的块
    live_block.insert(0); // 保证第一个块永远存在
    std::set<int> unlive_block;
    for(auto &[id, block]: *(C->block_map)){
        if(live_block.find(id)==live_block.end()){
            unlive_block.insert(id);
        }
    }
    for(auto& ulb: unlive_block){
        C->block_map->erase(ulb);
    }

    // 删除不活跃的指令
    for(auto &[id, block]: *(C->block_map)){
        std::deque<Instruction> old_Intrs = block->Instruction_list;
        block->Instruction_list.clear();

        for(auto& intr: old_Intrs){
            // 检查跳转label对应块是否会被删除，如果是则需要寻找后继块补充，跳转指令不可以缺失
            if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::BR_COND){
                BrCondInstruction* br_intr = (BrCondInstruction*)intr;
                int true_label = ((LabelOperand*)br_intr->GetTrueLabel())->GetLabelNo();
                int false_label = ((LabelOperand*)br_intr->GetFalseLabel())->GetLabelNo();
                // 对应块被删除， 在live_block中找不到
                if(live_block.find(true_label)==live_block.end()){
                    while(live_block.find(true_label)==live_block.end()){
                        true_label = domtrees->GetDomTree(C)->sdom_map[true_label];
                    }
                    br_intr->ChangeTrueLabel(GetNewLabelOperand(true_label));
                }
                if(live_block.find(false_label)==live_block.end()){
                    while(live_block.find(false_label)==live_block.end()){
                        false_label = domtrees->GetDomTree(C)->sdom_map[false_label];
                    }
                    br_intr->ChangeFalseLabel(GetNewLabelOperand(false_label));
                }

                // 检查两个label是否相同，相同就把指令替换为uncond
                if(true_label==false_label){
                    Instruction new_br_uncond = new BrUncondInstruction(GetNewLabelOperand(true_label));
                    block->Instruction_list.push_back(new_br_uncond);
                }
                else{
                    block->Instruction_list.push_back(intr);
                }
            }
            else if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::BR_UNCOND){
                BrUncondInstruction* br_intr = (BrUncondInstruction*)intr;
                int dest_label = ((LabelOperand*)br_intr->GetDestLabel())->GetLabelNo();
                // 对应块被删除， 在live_block中找不到
                if(live_block.find(dest_label)==live_block.end()){
                    while(live_block.find(dest_label)==live_block.end()){
                        dest_label = domtrees->GetDomTree(C)->sdom_map[dest_label];
                    }
                    br_intr->ChangeDestLabel(GetNewLabelOperand(dest_label));
                }
                block->Instruction_list.push_back(intr);
            }
            else{
                // 发现是活跃的指令
                if(live.find(intr)!=live.end()){
                    block->Instruction_list.push_back(intr);
                }
            }
        }
    }
}

void ADCEPass::Execute(){
    for (auto [defI, cfg] : llvmIR->llvm_cfg){
        live.clear();
        live_block.clear();
        ADCE(cfg);
        CleanUnlive(cfg);
		cfg->BuildCFG();
    }
}


void ADCEPass::EliminateSameInsts(){
    for(auto [defI, cfg] : llvmIR->llvm_cfg){
        std::unordered_set<int> regnos;
        for(auto &[id,block]: *(cfg->block_map)){
            std::unordered_set<Instruction> inst_todlt;
            for(auto it=block->Instruction_list.begin();it!=block->Instruction_list.end();it++){
                Instruction &inst=*it;
                if(inst_todlt.count(inst)){continue;}
                if(inst->GetOpcode()==BasicInstruction::MUL ||
                   inst->GetOpcode()==BasicInstruction::ADD ||
                   inst->GetOpcode()==BasicInstruction::GETELEMENTPTR){
                    int def_regno=inst->GetDefRegno();
                    //std::cout<<"the result_regno is "<<def_regno<<std::endl;
                    for(auto other=std::next(it);other!=block->Instruction_list.end();other++){
                        Instruction &other_inst = *other;
                        if(other_inst->GetOpcode()!=inst->GetOpcode()){continue;}
                        if(other_inst->isSame(inst)){
                            regnos.insert(def_regno);
                            int def_regno_todlt=other_inst->GetDefRegno();
                            auto use_insts=cfg->use_map[def_regno_todlt];
                            //std::cout<<" we delete result_regno: "<<def_regno_todlt<<std::endl;
                            
                            for(auto &use_inst:use_insts){
                                if(use_inst==nullptr){continue;}
                                auto operands=use_inst->GetNonResultOperands();
                                for(auto &operand:operands){
                                    if(operand->GetOperandType()==BasicOperand::REG && 
                                    ((RegOperand*)operand)->GetRegNo()==def_regno_todlt){
                                        operand = GetNewRegOperand(def_regno);
                                        cfg->use_map[def_regno].push_back(use_inst);
                                    }
                                } 
                                use_inst->SetNonResultOperands(operands);
                                
                            }
                            cfg->use_map.erase(def_regno_todlt);
                            inst_todlt.insert(*other);
                        }
                    }
                }
            }
            auto old_list=block->Instruction_list;
            block->Instruction_list.clear();
            for(auto &inst:old_list){
                if(!inst_todlt.count(inst)){
                    block->Instruction_list.push_back(inst);
                }
            }
        }

        // 重复指令替换后产生冗余重复的phi指令，统一为同一源
        for(auto &[id,block]: *(cfg->block_map)){
            std::unordered_set<Instruction> inst_todlt;
            for(auto it=block->Instruction_list.begin();it!=block->Instruction_list.end();it++){
                if((*it)->GetOpcode()!=BasicInstruction::PHI){break;}
                if(inst_todlt.count(*it)){continue;}
                PhiInstruction* inst=(PhiInstruction*) (*it);
                //if(inst->GetOpcode()==BasicInstruction::PHI){
                    for(auto &regno:regnos){
                        if(inst->GetUseRegno().count(regno)){
                            int regno_tobesame=inst->GetDefRegno();
                            //std::cout<<"this regno is "<<regno<<" and the result_regno is "<<regno_tobesame<<std::endl;
                            for(auto other=std::next(it);other!=block->Instruction_list.end();other++){
                                if((*other)->GetOpcode()==BasicInstruction::PHI &&
                                   (PhiInstruction*)(*other)->GetUseRegno().count(regno)){
                                    int regno_todlt=(*other)->GetDefRegno();
                                    auto use_insts=cfg->use_map[regno_todlt];
                                    //std::cout<<" we delete result_regno: "<<regno_todlt<<std::endl;
                                    for(auto &use_inst:use_insts){
                                        if(use_inst==nullptr){continue;}
                                        auto operands=use_inst->GetNonResultOperands();
                                        for(auto &operand:operands){
                                            if(operand->GetOperandType()==BasicOperand::REG && 
                                            ((RegOperand*)operand)->GetRegNo()==regno_todlt){
                                                operand = GetNewRegOperand(regno_tobesame);
                                                cfg->use_map[regno_tobesame].push_back(use_inst);
                                            }
                                        }
                                        use_inst->SetNonResultOperands(operands); 
                                    }
                                    cfg->use_map.erase(regno_todlt);
                                    inst_todlt.insert(*other);
                                }
                            }
                        }
                    }
                //}
            }
            auto old_list=block->Instruction_list;
            block->Instruction_list.clear();
            for(auto &inst:old_list){
                if(!inst_todlt.count(inst)){
                    block->Instruction_list.push_back(inst);
                }
            }
        }
    }
}

// 循环削弱后增加了一些同一基本块内的冗余重复的乘/加/块内GEP指令，删除
void ADCEPass::ESI(){
    EliminateSameInsts();
    EliminateSameInsts();
    // 删除块间相同的GEP指令【有效性待测试】
    for(auto [defI, cfg] : llvmIR->llvm_cfg){
        //收集所有的gep指令信息
        std::vector<std::pair<GetElementptrInstruction*,int>> gep_id_map;
        for(auto &[id,block]: *(cfg->block_map)){
            for(auto &inst:block->Instruction_list){
                if(inst->GetOpcode()==BasicInstruction::GETELEMENTPTR){
                    gep_id_map.push_back(std::pair((GetElementptrInstruction*)inst,id));
                }
            }
        }

        std::unordered_set<Instruction> insts;
        std::unordered_set<int> blockids;
        for(auto it=gep_id_map.begin();it!=gep_id_map.end();it++){
            if(insts.count(it->first)){continue;}

            GetElementptrInstruction* &inst=it->first;
            int regno=inst->GetDefRegno();
            auto same_block=cfg->GetBlockWithId(it->second);
            if (!same_block) continue; // 跳过无效的基本块
            for(auto other=std::next(it);other!=gep_id_map.end();other++){
                if(other->first->isSame(inst)){
                    auto other_block = cfg->GetBlockWithId(other->second);
                    if (!other_block) continue; // 跳过无效的基本块
                    auto Dominators=((DominatorTree*)(cfg->DomTree))->getDominators(other_block);
                    if(!Dominators.count(same_block)){continue;}// 不被支配，无法删除替代
                    insts.insert(other->first);
                    blockids.insert(other->second);
                    int regno_todlt=other->first->GetDefRegno();

                    auto use_insts=cfg->use_map[regno_todlt];
                    for(auto &use_inst:use_insts){
                        if(use_inst==nullptr){continue;}
                        auto operands=use_inst->GetNonResultOperands();
                        for(auto &operand:operands){
                            if(operand->GetOperandType()==BasicOperand::REG && 
                            ((RegOperand*)operand)->GetRegNo()==regno_todlt){
                                operand = GetNewRegOperand(regno);
                                cfg->use_map[regno].push_back(use_inst);
                            }
                        } 
                        use_inst->SetNonResultOperands(operands);
                    }
                    cfg->use_map.erase(regno_todlt);
                }
            }
        }
            
        // 删除块间相同GEP指令
        for(auto &id:blockids){
            auto block=cfg->GetBlockWithId(id);
            if (!block) continue; // 跳过无效的基本块
            auto old_list=block->Instruction_list;
            block->Instruction_list.clear();
            for(auto &inst:old_list){
                if(!insts.count(inst)){
                    block->Instruction_list.push_back(inst);
                }
            }
        }
    }

}
