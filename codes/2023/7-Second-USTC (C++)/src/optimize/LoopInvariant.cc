#include"LoopInvariant.hh"

void LoopInvariant::execute(){
    CFG_analyser = std::make_unique<CFGAnalyse>(m_);
    CFG_analyser->execute();
    auto loops = CFG_analyser->get_loops();
    for(auto loop : *loops){
        bool inner_first = false;
        for(auto bb : *loop){
            if(CFG_analyser->find_bb_loop(bb) != loop)
            {
                inner_first = true;
                break;
            }
        }
        if(inner_first) continue;

        auto cur_loop = loop;
        while(cur_loop != nullptr){
            invariants.clear();
            find_invariants(cur_loop);
            if(!invariants.empty()){
                move_invariants_out(cur_loop);
            }
            cur_loop = CFG_analyser->find_outer_loop(cur_loop);
        }
    }
    std::cout<<"Finished"<<std::endl;
}

void LoopInvariant::find_invariants(std::vector<BasicBlock *>* loop){
    std::set<Value *> defined_in_loop;
    std::map<Value *, std::set<Instruction *>> addr2st;
    std::map<Instruction *, Value *> st2addr;
    std::map<Instruction *, Value *> ld2addr;
    std::vector<Instruction *> invariant;
    bool have_call = false;
    for(auto bb : *loop){
        for(auto instr: bb->get_instructions()){
            defined_in_loop.insert(instr);
            if (instr->is_store()){
                auto ptr = instr->get_operand(1);
                addr2st[ptr].insert(instr);
                while (dynamic_cast<Instruction *>(ptr) &&
                            dynamic_cast<Instruction *>(ptr)->is_gep()) {
                    ptr = dynamic_cast<Instruction *>(ptr)->get_operand(0);
                }
                st2addr[instr] = ptr;
            }
            if(instr->is_load()){
                auto ptr = instr->get_operand(0);
                while (dynamic_cast<Instruction *>(ptr) &&
                            dynamic_cast<Instruction *>(ptr)->is_gep()) {
                    ptr = dynamic_cast<Instruction *>(ptr)->get_operand(0);
                }
                ld2addr[instr] = ptr;
            }
            if(instr->is_call())
                have_call = true;
        }     
    }
    bool changed = false;
    do{
        bool st_changed = false;
        changed = false;
        for(auto bb : *loop){
            invariant.clear();
            for(auto instr: bb->get_instructions()){
                bool is_invariant = true;
                if(instr->is_call()||instr->is_alloca()||instr->is_ret()||instr->is_br()||
                   instr->is_cmp()||instr->is_phi())
                    continue;
                if(defined_in_loop.find(instr) == defined_in_loop.end())
                    continue;
                for(auto operand : instr->get_operands()){
                    if(defined_in_loop.find(operand) != defined_in_loop.end()){
                        is_invariant = false;
                    }
                }
                if(instr->is_store()) continue;
                if((instr->is_load()||instr->is_store())&&have_call)
                    continue;
                // if(instr->is_store() ){
                //     auto ptr = st2addr[instr];
                //     if(addr2st[ptr].size()>1)
                //         continue;
                // }
                if(is_invariant && instr->is_load()){
                    auto ptr =  ld2addr[instr];
                    for(auto pair : st2addr){
                        if (pair.second == ptr) {
                            is_invariant = false;
                            break;
                        }
                    }
                }
                if(is_invariant){
                    defined_in_loop.erase(instr);
                    invariant.push_back(instr);
                    changed = true;
                }
            }
            if(!invariant.empty()){
                invariants.push_back({bb,invariant});
            }
        }
    } while (changed);
}

void LoopInvariant::move_invariants_out(std::vector<BasicBlock *>* loop){
    auto first_bb = CFG_analyser->find_loop_entry(loop);
    BasicBlock* new_bb = BasicBlock::create(m_, "", first_bb->get_parent());

    //锟斤拷锟斤拷循锟斤拷锟斤拷锟角帮拷锟�
    std::vector<BasicBlock *> pre_bbs;
    for(auto pre_bb : first_bb->get_pre_basic_blocks()){
        if(CFG_analyser->find_bb_loop(pre_bb) != loop){
            pre_bbs.push_back(pre_bb);
        }
    }

    //锟斤拷锟斤拷phi指锟斤拷
    for(auto instr : first_bb->get_instructions()){
        if(!instr->is_phi())
            break;
        auto inst_phi = dynamic_cast<PhiInst *>(instr);
        std::vector<std::pair<Value*,BasicBlock*>> ops_in_loop;
        std::vector<std::pair<Value*,BasicBlock*>> ops_out_loop;
        for (auto i = 0; i < instr->get_num_operands(); i=i+2){
            auto val = instr->get_operand(i);
            auto from_bb = dynamic_cast<BasicBlock*>(instr->get_operand(i+1));
            val->remove_use(instr);
            from_bb->remove_use(instr);
            if (CFG_analyser->find_bb_loop(from_bb) == loop){
                   ops_in_loop.push_back({val,from_bb});
            }
            else{
                ops_out_loop.push_back({val,from_bb});
            }
        }
        if (ops_out_loop.size()>1){
            auto new_phi = PhiInst::create_phi((*ops_out_loop.begin()).first->get_type(),new_bb);
            new_bb->add_instruction(new_phi);
            for (auto pair : ops_out_loop){
                new_phi->add_phi_pair_operand(pair.first,pair.second);
            }
            for (int i=0; i < (int)inst_phi->get_num_operands()/2; i++)
            {
                auto value = inst_phi->get_operand(i*2);
                auto bb = inst_phi->get_operand(i*2 + 1);
                value->remove_use(inst_phi);
                bb->remove_use(inst_phi);
            }
            inst_phi->remove_operands(0, instr->get_num_operands()-1);
            //
            for (auto pair : ops_in_loop){
                inst_phi->add_phi_pair_operand(pair.first,pair.second);
            }
            inst_phi->add_phi_pair_operand(new_phi, new_bb);
        }
        else{
            //not need a new phi
            for (int i=0; i < (int)inst_phi->get_num_operands()/2; i++)
            {
                auto value = inst_phi->get_operand(i*2);
                auto bb = inst_phi->get_operand(i*2 + 1);
                value->remove_use(inst_phi);
                bb->remove_use(inst_phi);
            }
            inst_phi->remove_operands(0, instr->get_num_operands()-1);
            for (auto pair : ops_out_loop){
                inst_phi->add_phi_pair_operand(pair.first,new_bb);
            }
            for (auto pair : ops_in_loop){
                inst_phi->add_phi_pair_operand(pair.first,pair.second);
            }
        }
    }


    //锟狡讹拷循锟斤拷锟斤拷锟斤拷式锟斤拷
    for(auto pair : invariants){
        for(auto instr : pair.second){
            pair.first->get_instructions().remove(instr);
            new_bb->add_instruction(instr);
            instr->set_parent(new_bb);
        }
    }
    //锟斤拷转指锟斤拷
    BranchInst::create_br(first_bb, new_bb);

    for(auto pre_bb : pre_bbs){
        auto terminator = pre_bb->get_terminator();
        if (terminator->get_num_operands() == 1){
            terminator->set_operand(0, new_bb);
        }
        else{
            if (dynamic_cast<BasicBlock*>(terminator->get_operand(1)) == first_bb){
                terminator->set_operand(1, new_bb);
            }
            else{
                terminator->set_operand(2, new_bb);
            }
        }
        first_bb->remove_use(terminator);
        first_bb->remove_pre_basic_block(pre_bb);
        pre_bb->remove_succ_basic_block(first_bb);
        new_bb->add_pre_basic_block(pre_bb);
        pre_bb->add_succ_basic_block(new_bb);
    }
}



