#include "RedundantInstsEli.hh"

void RedundantInstsEli::execute(){
    remove_same_phi();
    remove_redundant_geps();
}

void RedundantInstsEli::remove_same_phi(){
    for(auto func: module_->get_functions()) {
        for(auto bb: func->get_basic_blocks()){
            cur_bb = bb;
            std::vector<Instruction *> delete_list;
            for(auto instr: cur_bb->get_instructions()){
                if(instr->is_phi()){
                    auto fir_val = instr->get_operand(0);
                    auto fir_iconst = dynamic_cast<ConstantInt *>(fir_val);
                    int i_val;
                    if(fir_iconst){ i_val = fir_iconst->get_value();}
                    auto fir_fconst = dynamic_cast<ConstantFP *>(fir_val);
                    float f_val;
                    if(fir_fconst){ f_val = fir_fconst->get_value();}

                    if(instr->get_num_operands()==2){
                        instr->replace_all_use_with(fir_val);
                        delete_list.push_back(instr);
                    }
                    else{
                        bool is_dif = false;
                        for (int i = 2; i < instr->get_num_operands();i+=2){
                            auto operand = instr->get_operand(i);
                            auto operand_iconst = dynamic_cast<ConstantInt *>(operand);
                            auto operand_fconst = dynamic_cast<ConstantFP *>(operand);
                            if(fir_iconst){
                                if(!(operand_iconst &&i_val == operand_iconst->get_value())){
                                    is_dif = true;
                                    break;
                                }
                            }
                            else if(fir_fconst){
                                if(!(operand_fconst &&f_val == operand_fconst->get_value())){
                                    is_dif = true;
                                    break;
                                }
                            }
                            else{
                                if(!(fir_val == operand)){
                                    is_dif = true;
                                    break;
                                }
                            }
                        }

                        if(!is_dif){
                            instr->replace_all_use_with(fir_val);
                            delete_list.push_back(instr);
                        }

                    }
                }   
                else{
                    for(auto delete_instr: delete_list){
                        cur_bb->delete_instr(delete_instr);
                    }
                    return;
                }
            }
        }
    }
}

void RedundantInstsEli::replace_phi(){
    std::vector<Instruction *> delete_list;
    for(auto instr: cur_bb->get_instructions()){
        if(instr->is_phi()){
            int rep_cnt = 0;
            Value* dif_val;
            for (int i = 0; i < instr->get_num_operands();i+=2){
                auto operand = instr->get_operand(i);
                if(operand!=instr){
                    dif_val = operand;
                    rep_cnt++;
                }
            }
            if(rep_cnt == 0){
                delete_list.push_back(instr);
            }
            else if(rep_cnt == 1){
                instr->replace_all_use_with(dif_val);
                delete_list.push_back(instr);
            }
        }
        else{
            for(auto delete_instr: delete_list){
                cur_bb->delete_instr(delete_instr);
            }
            return;
        }
    }
}

void RedundantInstsEli::remove_redundant_geps() {

    std::vector<Instruction*> to_remove_geps;
    bool change;
    do {    
        change = false;
        for(auto func: module_->get_functions()) {
            if(func->is_declaration())
                continue;
            for(auto bb: func->get_basic_blocks()) {
                cur_bb = bb;
                for(auto instr: cur_bb->get_instructions()) {
                    if(instr->is_gep()) {
                        auto base = instr->get_operand(0);
                        auto gep_base = dynamic_cast<GetElementPtrInst*>(base);
                        if(gep_base) {
                            instr->replace_all_use_with(gep_base);
                            to_remove_geps.push_back(instr);
                        }
                    }
                }
            }
        }
        if(! to_remove_geps.empty()) {
            change = true;
            for(auto gep_instr: to_remove_geps) {
                gep_instr->get_parent()->delete_instr(gep_instr);
            }
            to_remove_geps.clear();
        }
    } while(change);    
}
