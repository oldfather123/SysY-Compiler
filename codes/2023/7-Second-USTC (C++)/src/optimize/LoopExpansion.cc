#include "LoopExpansion.hh"

void LoopExpansion::execute(){
    CFG_analyser = std::make_unique<CFGAnalyse>(module_);
    CFG_analyser->execute();
    find_try();
}

void LoopExpansion::find_try(){
    auto loops = CFG_analyser->get_loops();
    for(auto loop : *loops){
        if(loop->size() > 2)
            continue;
        auto expand_time = loop_check(loop);
        if(expand_time==0 || (expand_time>301 || expand_time<17)){
            continue;
        }
        expand(expand_time, loop);
    }
}

int LoopExpansion::loop_check(std::vector<BasicBlock*>* loop){
    auto entry_bb = *((*loop).rbegin());
    auto body_bb = *((*loop).begin());
    if(entry_bb->get_pre_basic_blocks().size()>2){
        return 0;
    }
    if(body_bb->get_pre_basic_blocks().size()>1){
        return 0;
    }
    auto terminator = entry_bb->get_terminator();
    if(terminator->is_br() && terminator->get_num_operands()==3){
        auto cond = dynamic_cast<Instruction *>(terminator->get_operand(0));
        if (cond == nullptr)
            return 0;
        if (cond->is_cmp() == false)
            return 0;
        else{
            auto cond_cmp = dynamic_cast<CmpInst *>(cond);
            auto cmp_op = cond_cmp->get_cmp_op();
            int const_pos = const_val_check(cond_cmp);
            if(const_pos==0){
                return 0;
            }
            int limit_val = dynamic_cast<ConstantInt *>(cond_cmp->get_operand(const_pos-1))->get_value();
            int init_val;
            int modify_val = 0;
            auto key = dynamic_cast<Instruction *>(cond_cmp->get_operand(2-const_pos));
            if(const_pos == 1){
                switch (cmp_op){
                case GE :
                    cmp_op = LE;
                    break;
                case LE :
                    cmp_op = GE;
                    break;
                case GT :
                    cmp_op = LT;
                    break;
                case LT :
                    cmp_op = GT;
                    break;
                default:
                    break;
                }
            }
            auto cur_instr = key;
            if(cur_instr == nullptr)
                return 0;
            if(!cur_instr->is_phi()||cur_instr->get_num_operands()>4)
                return 0;
            auto val1 = cur_instr->get_operand(0);
            auto val1_from = dynamic_cast<BasicBlock*>(cur_instr->get_operand(1));
            auto val2 = cur_instr->get_operand(2);
            if(CFG_analyser->find_bb_loop(val1_from) == loop){
                cur_instr = dynamic_cast<Instruction *>(val1);
                auto const_init_val = dynamic_cast<ConstantInt *>(val2);
                if(const_init_val == nullptr)
                    return 0;
                init_val = const_init_val->get_value();
            }
            else{
                cur_instr = dynamic_cast<Instruction *>(val2);
                auto const_init_val = dynamic_cast<ConstantInt *>(val1);
                if(const_init_val == nullptr)
                    return 0;
                init_val = const_init_val->get_value();
            }
            while(cur_instr!=key){
                if(cur_instr == nullptr)
                    return 0;
                if(cur_instr->is_add()){
                    const_pos = const_val_check(cur_instr);
                    if(const_pos==0)
                        return 0;
                    auto const_val = dynamic_cast<ConstantInt *>(cur_instr->get_operand(const_pos-1));
                    modify_val += const_val->get_value();
                    cur_instr = dynamic_cast<Instruction *>(cur_instr->get_operand(2-const_pos));
                }
                else if(cur_instr->is_sub()){
                    const_pos = const_val_check(cur_instr);
                    if(const_pos!=2)
                        return 0;
                    auto const_val = dynamic_cast<ConstantInt *>(cur_instr->get_operand(1));
                    modify_val -= const_val->get_value();
                    cur_instr = dynamic_cast<Instruction *>(cur_instr->get_operand(0));
                }
                else{
                    return 0;
                }
            }
            int gap = init_val - limit_val;
            int time = 0;
            switch (cmp_op){
            case GE :
                if(gap<0)
                    return -1;
                else{
                    if(modify_val>=0)
                        return 0;
                    while (gap >= 0){
                        gap += modify_val;
                        time ++;
                    }
                    return time;
                }
                break;
            case LE :
                if(gap>0)
                    return -1;
                else{
                    if(modify_val<=0)
                        return 0;
                    while (gap <= 0){
                        gap += modify_val;
                        time ++;
                    }
                    return time;
                }
                break;
            case GT :
                if(gap<=0)
                    return -1;
                else{
                    if(modify_val>=0)
                        return 0;
                    while (gap > 0){
                        gap += modify_val;
                        time ++;
                    }
                    return time;
                }
                break;
            case LT :
                if(gap>=0)
                    return -1;
                else{
                    if(modify_val<=0)
                        return 0;
                    while (gap < 0){
                        gap += modify_val;
                        time ++;
                    }
                    return time;
                }
                break;
            case NE :
                if(gap==0)
                    return -1;
                else{
                    if (gap%modify_val != 0){
                        return 0;
                    }
                    return gap/(- modify_val);
                }
                break;
            default:
                return 0;
                break;
            }
        }
    }
    else{
        return 0;
    }
}

int LoopExpansion::const_val_check(Instruction* instr){
    auto const_op1 = dynamic_cast<ConstantInt *>(instr->get_operand(0));
    auto const_op2 = dynamic_cast<ConstantInt *>(instr->get_operand(1));
    if(const_op1 && (const_op2==nullptr))
        return 1;
    else if(const_op2 && (const_op1==nullptr))
        return 2;
    else{
        return 0;
    }
}


void LoopExpansion::expand(int time, std::vector<BasicBlock*>* loop){
    auto entry_bb = *((*loop).rbegin());
    auto body_bb = *((*loop).begin());

    auto entry_terminator = entry_bb->get_terminator();
    auto next_bb = entry_terminator->get_operand(2);
    auto body_terminator = body_bb->get_terminator();
    entry_bb->get_instructions().pop_back();
    body_bb->delete_instr(body_terminator);

    std::map<Value *, Value *> phi2loop_instr;
    std::map<Value *, Value *> phi2outer_instr;
    std::map<Value *, Value *> phi2new_instr;
    std::map<Value *, Value *> loop_instr2new_instr;

    for(auto instr : entry_bb->get_instructions()){
        if(instr->is_phi()){
            auto val1 = instr->get_operand(0);
            auto val1_from = dynamic_cast<BasicBlock*>(instr->get_operand(1));
            auto val2 = instr->get_operand(2);
            if(CFG_analyser->find_bb_loop(val1_from) == loop){
                phi2outer_instr[instr] = val2;
                phi2loop_instr[instr] = val1;
            }
            else{
                phi2outer_instr[instr] = val1;
                phi2loop_instr[instr] = val2;
            }
            phi2new_instr[instr] = nullptr;
        }
        else
            break;
    }

    for (int i = 0; i < time; i++){
        for(auto instr : body_bb->get_instructions()){
            auto new_instr = instr->copy_inst(entry_bb);
            for (int idx = 0; idx < new_instr->get_num_operands(); idx++){
                auto operand = new_instr->get_operand(idx);
                if(i==0){
                    if(phi2outer_instr[operand]!=nullptr){
                        operand->remove_use(new_instr);
                        new_instr->set_operand(idx, phi2outer_instr[operand]);
                    }
                }
                if(phi2new_instr[operand]!=nullptr){
                    operand->remove_use(new_instr);
                    new_instr->set_operand(idx, phi2new_instr[operand]);
                }
                else if(loop_instr2new_instr[operand]!=nullptr){
                    operand->remove_use(new_instr);
                    new_instr->set_operand(idx, loop_instr2new_instr[operand]);
                }
            }
            loop_instr2new_instr[instr] = new_instr;
        }
        for(auto pair : phi2loop_instr){
            phi2new_instr[pair.first] = loop_instr2new_instr[pair.second];
        }
    }

    for (auto iter_instr = entry_bb->get_instructions().begin();iter_instr!=entry_bb->get_instructions().end();){
        auto instr = *iter_instr;
        if(instr->is_phi()){
            auto loop_val = phi2loop_instr[instr];
            auto new_val = loop_instr2new_instr[loop_val];
            for(auto use : instr->get_use_list()){
                auto use_instr = dynamic_cast<Instruction *>(use.val_);
                if(use_instr==nullptr)
                    continue;
                if(CFG_analyser->find_bb_loop(use_instr->get_parent())!=loop){
                    use_instr->set_operand(use.arg_no_, new_val);
                }
            }
            iter_instr++;
            entry_bb->delete_instr(instr);
        }
        else if(instr->is_cmp()){
            iter_instr++;
            entry_bb->delete_instr(instr);
        }
        else{
            break;
        }
    }

    entry_terminator->remove_operands(0, 2);
    entry_terminator->add_operand(next_bb);
    entry_bb->get_instructions().push_back(entry_terminator);
    auto body_instrs = body_bb->get_instructions();
    for(auto instr: body_instrs){
        body_bb->delete_instr(instr);
    }
    body_bb->get_parent()->remove_basic_block(body_bb);
}













