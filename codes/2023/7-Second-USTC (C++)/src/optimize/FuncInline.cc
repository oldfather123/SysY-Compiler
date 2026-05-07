#include <math.h>
#include "FuncInline.hh"

void FuncInline::execute(){
    inline_call_find();
    func_inline();
}

void FuncInline::inline_call_find(){
    std::set<Function *> recursive_record;
    std::vector<Function*> delete_list;
    for(auto func: module_->get_functions()){
        if(func == module_->get_main_function())
            continue;
        if(func->get_num_basic_blocks()==0) continue;
        //compute num of instr
        int LOC = 0;
        for(auto bb: func->get_basic_blocks()){
            LOC += bb->get_num_of_instrs();
        }
        //check recursive
        for(auto use: func->get_use_list()){
            auto instr = dynamic_cast<Instruction *>(use.val_);
            if(instr->get_parent()->get_parent()==func){
                recursive_record.insert(func);
                break;
            }
        }
        if(recursive_record.find(func)!=recursive_record.end()) continue;

        //reocrd func call
        for(auto use: func->get_use_list()){
            auto instr = dynamic_cast<Instruction *>(use.val_);
            auto depth = instr->get_parent()->get_loop_depth();
            //if(depth+2<exp(0.04*(double)LOC)) continue;
            //if(LOC>=MAX_INSTRUCTION_NUM) continue;
            calling_pair.push_back({instr->get_parent()->get_parent(), {instr, func}});
        }
        delete_list.push_back(func);
    }

    for(auto func: delete_list){
        module_->get_functions().remove(func);
    }
}

void FuncInline::func_inline(){
    for(auto pair: calling_pair){
        auto cur_func = pair.first;
        auto instr_call = pair.second.first;
        auto call_func = pair.second.second;
        auto cur_bb = instr_call->get_parent();
        auto split_bb = BasicBlock::create(module_, "", cur_func);

        //if(call_func->get_name()=="f") continue;
        //if(call_func->get_name()=="my_sqrt") continue;
        //if(call_func->get_name()=="my_fabs") continue;

        bool have_func = false;
        //move alloca_instr in entry_bb to begin
        std::vector<Instruction *> add_begin_instruction;
        std::vector<Instruction *> delete_list;
        for(auto instr: cur_func->get_entry_block()->get_instructions()){
            if(instr->is_alloca()){
                    add_begin_instruction.push_back(instr);
                }
        }

        
        for(auto instr: add_begin_instruction){
            cur_func->get_entry_block()->get_instructions().remove(instr);
        }
        for(auto instr: add_begin_instruction){
            cur_func->get_entry_block()->add_instr_begin(instr);
        }

        // split the bb having instr_call 
        bool need_move = false;
        for (auto instr_it = cur_bb->get_instructions().begin(); instr_it != cur_bb->get_instructions().end();instr_it++){
            if(need_move){
                auto instr = *instr_it;
                split_bb->add_instruction(instr);
                instr->set_parent(split_bb);
                instr_it--;
                cur_bb->get_instructions().remove(instr);
            }
            if((*instr_it)==instr_call){
                need_move = true;
            }
        }
        
        //modify succ_bb
        for(auto succ_bb: cur_bb->get_succ_basic_blocks()){
            succ_bb->remove_pre_basic_block(cur_bb);
            succ_bb->add_pre_basic_block(split_bb);
            split_bb->add_succ_basic_block(succ_bb);
            for(auto succ_instr: succ_bb->get_instructions()){
                if(succ_instr->is_phi()){
                    for (auto i = 0; i < succ_instr->get_num_operands();i+=2){
                        auto from_bb = dynamic_cast<BasicBlock *>(succ_instr->get_operand(i + 1));
                        if(from_bb==cur_bb){
                            from_bb->remove_use(succ_instr);
                            succ_instr->set_operand(i + 1, split_bb);
                        }
                    }
                }
            }
        }
        cur_bb->get_succ_basic_blocks().clear();



        auto func_entry = cur_func->get_entry_block();
        auto entry_instrs = &func_entry->get_instructions();
        auto entry_terminator = func_entry->get_terminator();
        if(entry_terminator !=nullptr){
            entry_instrs->pop_back();
        }

        Instruction *new_instr;
        BasicBlock *new_bb;
        std::map<Value *, Value *> old2new_instr;
        std::map<Value *, BasicBlock *> old2new_bb;
        std::vector<BasicBlock *> new_bbs;
        std::vector<std::pair<Value*, BasicBlock*>>split_phi_pair;
        int arg_idx = 1;
        for (auto arg: call_func->get_args()){
            old2new_instr[arg] = instr_call->get_operand(arg_idx++);
        }
        //copy from func_bb to new_bb
        for(auto old_bb: call_func->get_basic_blocks()){
            new_bb = BasicBlock::create(module_, "", cur_func);
            old2new_bb[old_bb] = new_bb;
            new_bbs.push_back(new_bb);
            for(auto old_instr: old_bb->get_instructions()){
                if(old_instr->is_alloca()){
                    new_instr = old_instr->copy_inst(func_entry);
                }
                else{
                    new_instr = old_instr->copy_inst(new_bb);
                }
                if(old_instr->is_phi()){
                    new_bb->add_instruction(new_instr);
                }
                old2new_instr[old_instr] = new_instr;
            }
        }
        //modify instr in new_bb with new operand
        for(auto bb: new_bbs){
            for(auto instr:bb->get_instructions()){
                if(instr->is_phi()){
                    for (int i = 0; i < instr->get_num_operands(); i+=2){
                        if(old2new_instr[instr->get_operand(i)]!=nullptr){
                            instr->get_operand(i)->remove_use(instr);
                            instr->set_operand(i, old2new_instr[instr->get_operand(i)]);
                        }
                        instr->get_operand(i + 1)->remove_use(instr);
                        instr->set_operand(i + 1, old2new_bb[instr->get_operand(i + 1)]);
                    }
                }
                else if(instr->is_br()){
                    if(instr->get_num_operands()==3){
                        if(old2new_instr[instr->get_operand(0)]!=nullptr){
                            instr->get_operand(0)->remove_use(instr);
                            instr->set_operand(0, old2new_instr[instr->get_operand(0)]);
                        }
                        instr->get_operand(1)->remove_use(instr);
                        auto true_bb = old2new_bb[instr->get_operand(1)];
                        instr->set_operand(1,true_bb);
                        true_bb->add_pre_basic_block(bb);
                        bb->add_succ_basic_block(true_bb);
                        auto false_bb = old2new_bb[instr->get_operand(2)];
                        instr->get_operand(2)->remove_use(instr);
                        instr->set_operand(2,false_bb);
                        false_bb->add_pre_basic_block(bb);
                        bb->add_succ_basic_block(false_bb);
                    }
                    else{
                        instr->get_operand(0)->remove_use(instr);
                        auto true_bb = old2new_bb[instr->get_operand(0)];
                        instr->set_operand(0,true_bb);
                        true_bb->add_pre_basic_block(bb);
                        bb->add_succ_basic_block(true_bb);
                    }
                }
                else if(instr->is_ret()){
                    if (instr->get_num_operands()>0){
                        split_phi_pair.push_back({instr->get_operand(0),bb});
                    }
                    bb->delete_instr(instr);
                    BranchInst::create_br(split_bb,bb);
                    break;
                }
                else{
                    for (int i = 0; i < instr->get_num_operands(); i++){
                        if(old2new_instr[instr->get_operand(i)]!=nullptr){
                            instr->get_operand(i)->remove_use(instr);
                            instr->set_operand(i,old2new_instr[instr->get_operand(i)]);
                        }
                    }
                }
            }
        }
        //create phi for mul ret_val && replace the use of ret_val with new_phi
        if(split_phi_pair.size()>1){
            auto ret_phi = PhiInst::create_phi(call_func->get_return_type(), split_bb);
            split_bb->add_instr_begin(ret_phi);
            for(auto phi_pair: split_phi_pair){
                Value *real_val;
                if(old2new_instr[phi_pair.first]==nullptr){
                    real_val = phi_pair.first;
                }
                else{
                    real_val = old2new_instr[phi_pair.first];
                }
                auto real_bb = phi_pair.second;
                ret_phi->add_phi_pair_operand(real_val, real_bb);
            }
            instr_call->replace_all_use_with(ret_phi);
        }
        else if(split_phi_pair.size()==1){
            auto ret_val = (*split_phi_pair.begin()).first;
            if(old2new_instr[ret_val]!=nullptr){
                ret_val = old2new_instr[ret_val];
            }
            instr_call->replace_all_use_with(ret_val);
        }
        cur_bb->delete_instr(instr_call);
        auto new_entry = old2new_bb[call_func->get_entry_block()];
        BranchInst::create_br(new_entry, cur_bb);
        if (entry_terminator != nullptr){
            entry_instrs->push_back(entry_terminator);
        }
    }
}



