#include "simplify_cfg.h"
#include "mem2reg.h"
#include "assert.h"
#include <algorithm>
#include <stack>
void SimplifyCFGPass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        EliminateUnreachedBlocksInsts(cfg);
    }
}
void SimplifyCFGPass::EOBB() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        EliminateUnreachedBlocksInsts(cfg);
        EliminateOneBrUncondBlocks(cfg);//含重建CFG
        //重建支配树 （正向）
        delete DomInfo[cfg];
        DomInfo[cfg] = new DominatorTree(cfg);
        DomInfo[cfg]->BuildDominatorTree(false);
        cfg->DomTree = DomInfo[cfg]; 
    }
}
void SimplifyCFGPass::EOPPhi() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        for(auto &[bbid, bb] : *(cfg->block_map)) {
            bb->dfs_id = 0;
        }
        std::unordered_set<int> regno_tobedeleted{};
        LLVMBlock zero_block = (*(cfg->block_map))[0];
        EliminateOnePredPhi(cfg,zero_block ,regno_tobedeleted);
    }
}
void SimplifyCFGPass::TOPPhi() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        TransformOnePredPhi(cfg);
    }
}
//1. 删除不可达基本块（不可达指令已在build cfg中删除）
void SimplifyCFGPass::EliminateUnreachedBlocksInsts(CFG *C) {
    //(1)删除不可达基本块
    for (auto it = C->block_map->begin(); it != C->block_map->end(); ) {
        if (!(it->second->dfs_id)) {
            it=C->block_map->erase(it); // 删除当前元素
        }else{
            C->block_ids.insert(it->first); // 记录当前块id
            ++it; // 仅在未删除元素时移向下一个
        }
    }
}
void SimplifyCFGPass::EliminateNotExistPhi(CFG *C) {
    //(2)清理不可达块后继中的phi指令:前提：需要保证当前phi指令中的冗余label皆是不可达块
    for (auto &[bbid, bb] : *(C->block_map)) {
        auto old_intr_list = bb->Instruction_list;
        bb->Instruction_list.clear();
        for (auto &intr : old_intr_list) {
            if (intr->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
                PhiInstruction *phi_intr = (PhiInstruction *)intr;
                auto phi_list = phi_intr->GetPhiList();
                std::vector<std::pair<Operand, Operand>> new_phi_list;
                for (const auto &pair : phi_list) {
                    int label_no = ((LabelOperand *)pair.first)->GetLabelNo();
                    if (C->block_ids.count(label_no) > 0) {
                        // 如果label存在于当前CFG的块中，则保留
                        new_phi_list.push_back(pair);
                    }
                }
                // 剩余phi_list不可能为0，否则其为不可达块，也不存在了
                assert(new_phi_list.size()>0);
                bool still_phi=false;
                //[1] 剩余phi_list>=2，且不同
                if(new_phi_list.size()>=2){
                    Operand ave_operand=new_phi_list[0].second;
                    for(int i=1;i<new_phi_list.size();i++){
                        if(phi_intr->NotEqual(new_phi_list[i].second,ave_operand)){
                            still_phi=true;
                            break;
                        }
                    }
                }
                if(still_phi){
                    phi_intr->SetPhiList(new_phi_list);
                    bb->Instruction_list.push_back(intr);
                //[2] 剩余phi_list>=2，且均相同； 或剩余phi_list=1
                }else{
                    Operand phi_source_op=new_phi_list[0].second;
                    int def_regno=phi_intr->GetDefRegno();
                    auto it=C->use_map.find(def_regno);
                    if(it!=C->use_map.end()){
                        //对于所有用到此phi_def_reg的指令，将操作数替换，并删除此phi指令
                        std::vector<Instruction> intrs=it->second;
                        for(auto &intr:intrs){
                            auto operands=intr->GetNonResultOperands();
                            std::vector<Operand> new_operands{};
                            for(auto &operand:operands){
                                if(operand->GetOperandType()==BasicOperand::REG){
                                    int use_regno=((RegOperand*)operand)->GetRegNo();
                                    if(use_regno==def_regno){
                                        //匹配上了，替换值
                                        new_operands.push_back(phi_source_op);
                                    }else{
                                        new_operands.push_back(operand);
                                    }
                                }else{
                                    new_operands.push_back(operand);
                                }
                            }
                            intr->SetNonResultOperands(new_operands);
                        }
                    }
                }
            }else{
                bb->Instruction_list.push_back(intr);
            }
        }
    }

}
//for sccp
void SimplifyCFGPass::RebuildCFGforSCCP(){
    // 重建CFG（遍历cfg，重建G与invG)
    DomInfo.clear();
    llvmIR->CFGInit();
    for (auto &[defI,cfg] : llvmIR->llvm_cfg) {
        //重建支配树 （正向）
        delete DomInfo[cfg];
        DomInfo[cfg] = new DominatorTree(cfg);
        DomInfo[cfg]->BuildDominatorTree(false);
        cfg->DomTree = DomInfo[cfg]; 
        // 消除不可达块
        EliminateUnreachedBlocksInsts(cfg);
        // 处理随不可达块受影响的phi指令
        EliminateNotExistPhi(cfg);
    }
}

void SimplifyCFGPass::RebuildCFG(){
    // 重建CFG（遍历cfg，重建G与invG)
    DomInfo.clear();
	llvmIR->SyncMaxInfo();
    llvmIR->CFGInit();

	// for(auto &[defI,cfg] : llvmIR->llvm_cfg){
	// 	std::cout << "function_def: " << defI->GetFunctionName() << std::endl;
	// 	std::cout << "max_reg: " << cfg->max_reg << std::endl;
	// 	std::cout << "max_label: " << cfg->max_label << std::endl;
	// }

    for (auto &[defI,cfg] : llvmIR->llvm_cfg) {
        //重建支配树 （正向）
        delete DomInfo[cfg];
        DomInfo[cfg] = new DominatorTree(cfg);
        DomInfo[cfg]->BuildDominatorTree(false);
        cfg->DomTree = DomInfo[cfg]; 
    }
}

// 检查是否可以安全消解当前块（不导致 phi 指令出现重复来源）
bool SimplifyCFGPass::IsSafeToEliminate(CFG *C, LLVMBlock block, LLVMBlock pred_block, LLVMBlock nextbb) {
    for (auto &next_intr : nextbb->Instruction_list) {
        if (next_intr->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
            PhiInstruction* phi_intr = (PhiInstruction*)next_intr;
            auto phi_list = phi_intr->GetPhiList();
            
            // 如果后继块的 phi 指令中已经存在来自 pred_block 的标签, 并且马上要将 block 替换为 pred_block 的标签, 则不安全
            for (int i = 0; i < phi_list.size(); i++) {
                if (((LabelOperand*)phi_list[i].first)->GetLabelNo() == block->block_id) {
                    for (auto &phi_pair : phi_list) {
                        if (((LabelOperand*)phi_pair.first)->GetLabelNo() == pred_block->block_id) {
                            return false; 
                        }
                    }
                }
            }
        }
    }
    return true; 
}

// 删除只有一条无条件跳转指令的基本块
void SimplifyCFGPass::EliminateOneBrUncondBlocks(CFG *C) {
    for (auto it = next(C->block_map->begin()); it != C->block_map->end(); ) {
        int id = it->first;
        LLVMBlock block = it->second;

        // 检查是否仅包含一条无条件跳转指令
        if (block->Instruction_list.size() == 1) {
            auto intr = *block->Instruction_list.begin();
            if (intr->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_UNCOND) {
                BrUncondInstruction* br_intr = (BrUncondInstruction*)intr;
                int dest_label = ((LabelOperand*)br_intr->GetDestLabel())->GetLabelNo();

                // [1] 获取前驱块和后继块
                std::set<LLVMBlock> preds = C->GetPredecessor(id);
                std::set<LLVMBlock> succs = C->GetSuccessor(id);

                // 仅处理单前驱单后继的情况
                if (preds.size() == 1 && succs.size() == 1) {
                    LLVMBlock pred_block = *preds.begin();
                    LLVMBlock nextbb = (*(C->block_map))[dest_label];

                    // [2] 语义安全检查：确保不会导致 phi 指令重复来源
                    if (!IsSafeToEliminate(C, block, pred_block, nextbb)) {
                        ++it;
                        continue; 
                    }

                    // [3] 更新前驱的跳转目标到 dest_label
                    std::deque<Instruction> old_Intrs = pred_block->Instruction_list;
                    pred_block->Instruction_list.clear();

                    for (auto& pre_intr : old_Intrs) {
                        if (pre_intr->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_COND) {
                            BrCondInstruction* pre_br_intr = (BrCondInstruction*)pre_intr;
                            int true_label = ((LabelOperand*)pre_br_intr->GetTrueLabel())->GetLabelNo();
                            int false_label = ((LabelOperand*)pre_br_intr->GetFalseLabel())->GetLabelNo();

                            if (true_label == id) {
                                pre_br_intr->ChangeTrueLabel(GetNewLabelOperand(dest_label));
                            }
                            if (false_label == id) {
                                pre_br_intr->ChangeFalseLabel(GetNewLabelOperand(dest_label));
                            }

                            // 如果 true_label == false_label，转换为无条件跳转
                            if (true_label == false_label) {
                                Instruction new_br_uncond = new BrUncondInstruction(GetNewLabelOperand(dest_label));
                                pred_block->Instruction_list.push_back(new_br_uncond);
                            } else {
                                pred_block->Instruction_list.push_back(pre_intr);
                            }
                        } else if (pre_intr->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_UNCOND) {
                            BrUncondInstruction* pre_br_intr = (BrUncondInstruction*)pre_intr;
                            if (((LabelOperand*)pre_br_intr->GetDestLabel())->GetLabelNo() == id) {
                                pre_br_intr->ChangeDestLabel(GetNewLabelOperand(dest_label));
                            }
                            pred_block->Instruction_list.push_back(pre_intr);
                        } else {
                            pred_block->Instruction_list.push_back(pre_intr);
                        }
                    }

                    // [4] 更新后继块的 phi 指令（将 id 替换为 pred_block->block_id）
                    for (auto &next_intr : nextbb->Instruction_list) {
                        if (next_intr->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
                            PhiInstruction* phi_intr = (PhiInstruction*)next_intr;
                            auto phi_list = phi_intr->GetPhiList();
                            for (int i = 0; i < phi_list.size(); i++) {
                                if (((LabelOperand*)phi_list[i].first)->GetLabelNo() == id) {
                                    Operand val = phi_list[i].second;
                                    phi_intr->ChangePhiPair(i, std::make_pair(GetNewLabelOperand(pred_block->block_id), val));
                                    break;
                                }
                            }
                        }
                    }

                    // [5] 更新 CFG 结构并删除当前块
                    C->G[pred_block->block_id].insert(nextbb);
                    C->G[pred_block->block_id].erase(block);
                    C->invG[dest_label].insert(pred_block);
                    C->invG[dest_label].erase(block);
                    C->G.erase(id);
                    C->invG.erase(id);

                    it = C->block_map->erase(it);
                    continue;
                }
            }
        }
        ++it;
    }
    // [6] 清理所有phi指令中引用已被删除块的元素
    std::set<int> valid_labels;
    for (const auto& pair : *C->block_map) {
        valid_labels.insert(pair.first);
    }
    for (auto& pair : *C->block_map) {
        LLVMBlock block = pair.second;
        for (auto& instr : block->Instruction_list) {
            if (instr->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
                PhiInstruction* phi = (PhiInstruction*)instr;
                auto phi_list = phi->GetPhiList();
                std::vector<std::pair<Operand, Operand>> new_phi_list;
                for (const auto& phi_pair : phi_list) {
                    int label = ((LabelOperand*)phi_pair.first)->GetLabelNo();
                    if (valid_labels.find(label) != valid_labels.end()) {
                        new_phi_list.push_back(phi_pair);
                    }
                }
                phi->SetPhiList(new_phi_list);
            }
        }
    }
}

// 3. 删除只有一个前驱的phi指令：主要用于SCCP的善后，产生的单前驱phi实际已经无用，我们删除这些phi和用到phi_result的后续指令
void SimplifyCFGPass::EliminateOnePredPhi(CFG* C,LLVMBlock nowblock,std::unordered_set<int> regno_tobedeleted){
    //std::cout<<" start a block!"<<std::endl;
    if(nowblock->dfs_id>0)return;
    //std::cout<<" we do: "<<std::endl;
    nowblock->dfs_id++;
        //[1]遍历此块的所有指令，处理冗余phi指令
        std::deque<Instruction> old_Intrs = nowblock->Instruction_list;
        nowblock->Instruction_list.clear();
        for(auto& intr: old_Intrs){
            if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::PHI){
                PhiInstruction* phi_intr = (PhiInstruction*)intr;
                auto phi_list = phi_intr->GetPhiList();
                //[1.1]若此phi指令的源值只有一个，则删除此phi指令
                if(phi_list.size()==1){
                    //删除此phi指令，并记录下其result regno，以便后继也清除
                    int regno = ((RegOperand*)phi_intr->GetResult())->GetRegNo();
                    regno_tobedeleted.insert(regno);
                    //std::cout<<"in block : "<<nowblock->block_id<<" delete phi of "<<regno<<std::endl;
                    continue;
                }else {
                
                    // // [1.2]若此phi指令的源值有多个，但都相同，也删除此phi指令
                    // bool tobedeleted=true;
                    // int sourceregno = ((RegOperand*)phi_list[0].second)->GetRegNo();
                    // for(auto & phi_pair: phi_list){
                    //     if(((RegOperand*)phi_pair.second)->GetRegNo()!=sourceregno){
                    //         //std::cout<<"in block : "<<nowblock->block_id<<" delete phi of "<<sourceregno<<std::endl;
                    //         tobedeleted=false;
                    //         break;
                    //     }
                    // }
                    // if(tobedeleted){
                    //     //删除此phi指令，并记录下其result regno，以便后继也清除
                    //     int regno = ((RegOperand*)phi_intr->GetResult())->GetRegNo();
                    //     regno_tobedeleted.insert(regno);
                    //     //std::cout<<"in block : "<<nowblock->block_id<<" delete phi of "<<regno<<std::endl;
                    //     continue;
                    // }
                    //[1.3]若此phi指令的源值被清除，assert：该phi指令冗余,也删除
                    bool tobedeleted=false;
                    for(int i=0;i<phi_list.size();i++){
                         if(regno_tobedeleted.count(((RegOperand*)phi_list[i].second)->GetRegNo())){
                             int regno=((RegOperand*)phi_intr->GetResult())->GetRegNo();
                             regno_tobedeleted.insert(regno);
                             //std::cout<<"in block : "<<nowblock->block_id<<" delete phi of "<<regno<<std::endl;
                             tobedeleted=true;
                             break;
                         }
                    }
                    if(!tobedeleted){
                        nowblock->Instruction_list.push_back(intr);
                    }
                    continue;
                }
            }
            nowblock->Instruction_list.push_back(intr);
        }
        //[2]递归遍历此块的所有后继
        auto succ_blocks = C->GetSuccessor(nowblock);
        for(auto &succ_block:succ_blocks){
            EliminateOnePredPhi(C,succ_block,regno_tobedeleted);
        }
        return ;
}

//对于优化后仅剩单个前驱的Phi指令，我们将其改为普通的赋值，删除phi
void SimplifyCFGPass::TransformOnePredPhi(CFG* C){
    for(auto &[id,block]:(*C->block_map)){
        for(auto it=block->Instruction_list.begin();it!=block->Instruction_list.end();){
            Instruction inst=*it;
            if(inst->GetOpcode()==BasicInstruction::LLVMIROpcode::PHI){
                PhiInstruction* phi=(PhiInstruction*)inst;
                auto phi_list=phi->GetPhiList();
                if(phi_list.size()==1){
                    Operand value=phi_list[0].second;
                    int phi_res_regno=((RegOperand*)phi->GetResult())->GetRegNo();
                    std::vector<Instruction> use_insts = (C->use_map)[phi_res_regno];

                    //将后续use此phi指令result的指令的use_operand全部替换为phi的source operand
                    for(auto &use_inst:use_insts){
                        auto operands=use_inst->GetNonResultOperands();
                        std::vector<Operand> new_operands; 
                        new_operands.reserve(operands.size());
                        for(auto &operand:operands){
                            if(operand->GetOperandType()==BasicOperand::REG){
                                if(((RegOperand*)operand)->GetRegNo()==phi_res_regno){
                                    new_operands.emplace_back(value->Clone());
                                    continue;
                                }
                            }
                            new_operands.emplace_back(operand);
                        }
                        use_inst->SetNonResultOperands(new_operands);
                    }
                    it=block->Instruction_list.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }
}


int getMergeType(LLVMBlock block, CFG* cfg){
    int pred_count = cfg->GetPredecessor(block).size();
    int succ_count = cfg->GetSuccessor(block).size();
    if (pred_count == 1 && succ_count == 1) {
        return 1; // 单前驱单后继---> merge_body
    } else if (pred_count == 1 && succ_count > 1) {
        return 2; // 单前驱多后继---> merge_tail
    } else if (pred_count > 1 && succ_count == 1) {
        return 3; // 多前驱单后继---> merge_head 
    } else if (pred_count > 1 && succ_count > 1){
        return 4; // 多前驱多后继
    } else if (pred_count ==1 && succ_count == 0) {
        return 5; // 单前驱无后继---> merge_tail
    } else if (pred_count >1 && succ_count == 0) {
        return 6; // 多前驱无后继
    } else {
        return 0; // 无前驱，entry
    }
}

void DFS_MergeBlocks(LLVMBlock block, CFG* cfg, std::unordered_map<int, int>& block_id_map){
    if(block->dfs_id > 0) return; // 已访问过
    block->dfs_id=1;
    
    int merge_type = getMergeType(block, cfg);
    //std::cout<<"DFS_MergeBlocks: "<<block->block_id<<" merge_type: "<<merge_type<<std::endl;
    switch (merge_type){
        case 5:
        case 6: //无后继，终止遍历
            return ;
        case 0: // 无前驱，entry，不合并，直接考察后继
        {
            for (auto &succ_block : cfg->GetSuccessor(block)) {
                DFS_MergeBlocks(succ_block, cfg, block_id_map);
            }
            return ;
        }
        case 2:
        case 4://多个后继，不能和后继块合并，依次考察每个后继
        {
            for (auto &succ_block : cfg->GetSuccessor(block)) {
                DFS_MergeBlocks(succ_block, cfg, block_id_map);
            }
            return ;
        }
        case 1:
        case 3://单个后继，可以和后继块合并
        {   std::vector<int> merge_blockids;
            LLVMBlock nextbb = *(cfg->GetSuccessor(block).begin());
            while(getMergeType(nextbb,cfg)==1){
                nextbb->dfs_id = 1; // 标记为已访问
                merge_blockids.push_back(nextbb->block_id);
                nextbb = *(cfg->GetSuccessor(nextbb).begin());
            }
            int last_mergetype=getMergeType(nextbb,cfg);
            if(last_mergetype==2 || last_mergetype == 5){//单前驱，可以作为merge_tail
                //合并list的最后一块
                nextbb->dfs_id = 1; // 标记为已访问
                merge_blockids.push_back(nextbb->block_id);

                //将merge_blockids中的所有块合并到block中
                for (int merge_block_id : merge_blockids) {
                    LLVMBlock merge_block = cfg->GetBlockWithId(merge_block_id);
                    if (merge_block == nullptr) continue; // 跳过无效的基本块
                    // 将merge_block的指令移动到block中
                    block->Instruction_list.pop_back();
                    block->Instruction_list.insert(block->Instruction_list.end(),
                                                   merge_block->Instruction_list.begin(),
                                                   merge_block->Instruction_list.end());
                    
                    //更新G和invG
                    for (auto &succ_block : cfg->GetSuccessor(merge_block_id)) {
                        cfg->G[block->block_id].insert(succ_block);
                        cfg->G[block->block_id].erase(merge_block);
                        cfg->invG[succ_block->block_id].insert(block);
                        cfg->invG[succ_block->block_id].erase(merge_block);
                    }
                    cfg->G.erase(merge_block_id);
                    cfg->invG.erase(merge_block_id);

                    // 从CFG中删除merge_block
                    cfg->block_map->erase(merge_block_id); 
                    block_id_map[merge_block_id] = block->block_id; // 记录映射关系
                }


                // std::cout<<"Merge blocks: ";
                // for (int merge_block_id : merge_blockids) {
                //     std::cout << merge_block_id << " ";
                // }
                // std::cout<< std::endl;

                int last_block_id = merge_blockids.back();
                // 继续访问后继
                for (auto &succ_block : cfg->GetSuccessor(last_block_id)) {
                    DFS_MergeBlocks(succ_block, cfg, block_id_map);
                }

            }else if(last_mergetype==3 || last_mergetype==4 || last_mergetype==6){ //多个前驱，不能与前驱块合并，仅处理其前的blocks，并考察各自

                //将merge_blockids中的所有块合并到block中
                for (int merge_block_id : merge_blockids) {
                    LLVMBlock merge_block = cfg->GetBlockWithId(merge_block_id);
                    if (merge_block == nullptr) continue; 
                    // 将merge_block的指令移动到block中
                    assert(block->Instruction_list.back()->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_UNCOND);
                    block->Instruction_list.pop_back();
                    block->Instruction_list.insert(block->Instruction_list.end(),
                                                   merge_block->Instruction_list.begin(),
                                                   merge_block->Instruction_list.end());
                    //更新G和invG
                    for (auto &succ_block : cfg->GetSuccessor(merge_block_id)) {
                        cfg->G[block->block_id].insert(succ_block);
                        cfg->G[block->block_id].erase(merge_block);
                        cfg->invG[succ_block->block_id].insert(block);
                        cfg->invG[succ_block->block_id].erase(merge_block);
                    }
                    cfg->G.erase(merge_block_id);
                    cfg->invG.erase(merge_block_id);
                    
                    // 从CFG中删除merge_block
                    cfg->block_map->erase(merge_block_id); 
                    block_id_map[merge_block_id] = block->block_id; // 记录映射关系
                }

                // std::cout<<"Merge blocks: ";
                // for (int merge_block_id : merge_blockids) {
                //     std::cout << merge_block_id << " ";
                // }
                // std::cout<< std::endl;

                //各自考察
                DFS_MergeBlocks(nextbb, cfg, block_id_map);
            }else {
                assert(0);
            }
            return ;
        }
    }

}

void SimplifyCFGPass::MergeBlocks() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        //std::cout << "--------------In function: " << defI->GetFunctionName()<<"------------------" << std::endl;
        // 重置dfs编号
        for (auto [id, block] : *(cfg->block_map)) {
            block->dfs_id = 0;
        }
        std::unordered_map<int, int> block_id_map;
        // 从入口块开始，合并通过br_uncond相连的多个连续基本块
        LLVMBlock entry_block = (*(cfg->block_map))[0];
        DFS_MergeBlocks(entry_block, cfg, block_id_map);
        // 更新phi指令中的label
        for (auto &[id, block] : *(cfg->block_map)) {
            for (auto &intr : block->Instruction_list) {
                if (intr->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
                    PhiInstruction *phi_intr = (PhiInstruction *)intr;
                    auto phi_list = phi_intr->GetPhiList();
                    std::vector<std::pair<Operand, Operand>> new_phi_list;
                    for (const auto &pair : phi_list) {
                        int label_no = ((LabelOperand *)pair.first)->GetLabelNo();
                        if (block_id_map.count(label_no)) {
                            int new_label_no = block_id_map[label_no];
                            new_phi_list.emplace_back(GetNewLabelOperand(new_label_no), pair.second);
                        } else {
                            new_phi_list.push_back(pair);
                        }
                    }
                    phi_intr->SetPhiList(new_phi_list);
                }
            }
        }
    }
}

// 1. 最终我们可以将基本块 0 和其唯一后继合并（如果唯一才合并， 0 block 的后继需要保证只有一个前驱）
// 2. 遍历控制流图，对于br_uncond直接跳转，对于直接跳转并重新编号；对于 br_cond 先跳转 true 分支，然后跳转 false 分支，依次重新编号
// 3. 统计新的 max_label_no, 更新 maxinfo
void SimplifyCFGPass::BasicBlockLayoutOptimize() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        // 1. 尝试将基本块0与其唯一后继合并
        LLVMBlock entry_block = (*(cfg->block_map))[0];
        if (entry_block != nullptr) {
            auto successors = cfg->GetSuccessor(0);
            if (successors.size() == 1) {
                LLVMBlock succ_block = *(successors.begin());
                auto preds_of_succ = cfg->GetPredecessor(succ_block);
                // 只有当后继块只有一个前驱（即基本块0）时才合并
                if (preds_of_succ.size() == 1) {
                    // 合并基本块0和其唯一后继
                    // 将后继块的指令移动到基本块0中
                    auto last_inst = entry_block->Instruction_list.back();
                    if (last_inst->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_UNCOND) {
                        // 删除基本块0的最后一条无条件跳转指令
                        entry_block->Instruction_list.pop_back();
                    }
                    
                    // 将后继块的所有指令添加到基本块0
                    entry_block->Instruction_list.insert(
                        entry_block->Instruction_list.end(),
                        succ_block->Instruction_list.begin(),
                        succ_block->Instruction_list.end()
                    );
                    
                    // 更新CFG结构
                    auto succs_of_succ = cfg->GetSuccessor(succ_block);
                    for (auto &succ_of_succ : succs_of_succ) {
                        cfg->G[0].insert(succ_of_succ);
                        cfg->invG[succ_of_succ->block_id].insert(entry_block);
                        cfg->invG[succ_of_succ->block_id].erase(succ_block);
                    }
                    
                    // 更新后继块的phi指令
                    for (auto &succ_of_succ : succs_of_succ) {
                        for (auto &intr : succ_of_succ->Instruction_list) {
                            if (intr->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
                                PhiInstruction *phi_intr = (PhiInstruction *)intr;
                                auto phi_list = phi_intr->GetPhiList();
                                for (int i = 0; i < phi_list.size(); i++) {
                                    if (((LabelOperand *)phi_list[i].first)->GetLabelNo() == succ_block->block_id) {
                                        Operand val = phi_list[i].second;
                                        phi_intr->ChangePhiPair(i, std::make_pair(GetNewLabelOperand(0), val));
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    // 删除后继块
                    cfg->G.erase(succ_block->block_id);
                    cfg->invG.erase(succ_block->block_id);
                    cfg->block_map->erase(succ_block->block_id);
                }
            }
        }
        
        // 2. 重新编号所有基本块，从0开始依次+1
        std::vector<std::pair<int, LLVMBlock>> blocks_to_renumber;
        for (auto &[old_id, block] : *(cfg->block_map)) {
            blocks_to_renumber.push_back({old_id, block});
        }
        
        // 按原ID排序，确保重新编号的一致性
        std::sort(blocks_to_renumber.begin(), blocks_to_renumber.end());
        
        std::unordered_map<int, int> id_mapping;
        std::map<int, LLVMBlock> new_block_map;
        
        // 创建新的ID映射
        for (int i = 0; i < blocks_to_renumber.size(); i++) {
            int old_id = blocks_to_renumber[i].first;
            LLVMBlock block = blocks_to_renumber[i].second;
            int new_id = i;
            
            id_mapping[old_id] = new_id;
            block->block_id = new_id;
            new_block_map[new_id] = block;
        }
        
        // 更新CFG的block_map
        *(cfg->block_map) = new_block_map;
        
        // 更新CFG的G和invG
        std::unordered_map<int, std::set<LLVMBlock>> new_G;
        std::unordered_map<int, std::set<LLVMBlock>> new_invG;
        
        for (auto &[old_id, successors] : cfg->G) {
            if (id_mapping.count(old_id)) {
                int new_id = id_mapping[old_id];
                new_G[new_id] = successors;
            }
        }
        
        for (auto &[old_id, predecessors] : cfg->invG) {
            if (id_mapping.count(old_id)) {
                int new_id = id_mapping[old_id];
                new_invG[new_id] = predecessors;
            }
        }
        
        cfg->G = new_G;
        cfg->invG = new_invG;
        
        // 更新所有跳转指令中的标签
        for (auto &[new_id, block] : *(cfg->block_map)) {
            for (auto &intr : block->Instruction_list) {
                intr->SetBlockID(new_id);
                
                if (intr->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_UNCOND) {
                    BrUncondInstruction *br_intr = (BrUncondInstruction *)intr;
                    auto dest_label = br_intr->GetDestLabel();
                    if (dest_label->GetOperandType() == BasicOperand::LABEL) {
                        int old_label_no = ((LabelOperand *)dest_label)->GetLabelNo();
                        if (id_mapping.count(old_label_no)) {
                            int new_label_no = id_mapping[old_label_no];
                            br_intr->ChangeDestLabel(GetNewLabelOperand(new_label_no));
                        }
                    }
                } else if (intr->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_COND) {
                    BrCondInstruction *br_intr = (BrCondInstruction *)intr;
                    auto true_label = br_intr->GetTrueLabel();
                    auto false_label = br_intr->GetFalseLabel();
                    
                    if (true_label->GetOperandType() == BasicOperand::LABEL) {
                        int old_label_no = ((LabelOperand *)true_label)->GetLabelNo();
                        if (id_mapping.count(old_label_no)) {
                            int new_label_no = id_mapping[old_label_no];
                            br_intr->ChangeTrueLabel(GetNewLabelOperand(new_label_no));
                        }
                    }
                    
                    if (false_label->GetOperandType() == BasicOperand::LABEL) {
                        int old_label_no = ((LabelOperand *)false_label)->GetLabelNo();
                        if (id_mapping.count(old_label_no)) {
                            int new_label_no = id_mapping[old_label_no];
                            br_intr->ChangeFalseLabel(GetNewLabelOperand(new_label_no));
                        }
                    }
                } else if (intr->GetOpcode() == BasicInstruction::LLVMIROpcode::PHI) {
                    PhiInstruction *phi_intr = (PhiInstruction *)intr;
                    auto phi_list = phi_intr->GetPhiList();
                    std::vector<std::pair<Operand, Operand>> new_phi_list;
                    
                    for (const auto &pair : phi_list) {
                        if (pair.first->GetOperandType() == BasicOperand::LABEL) {
                            int old_label_no = ((LabelOperand *)pair.first)->GetLabelNo();
                            if (id_mapping.count(old_label_no)) {
                                int new_label_no = id_mapping[old_label_no];
                                new_phi_list.emplace_back(GetNewLabelOperand(new_label_no), pair.second);
                            } else {
                                new_phi_list.push_back(pair);
                            }
                        } else {
                            new_phi_list.push_back(pair);
                        }
                    }
                    
                    phi_intr->SetPhiList(new_phi_list);
                }
            }
        }
        
        // 3. 更新max_label
        cfg->max_label = blocks_to_renumber.size() - 1;
    }
}