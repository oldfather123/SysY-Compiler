#include "VarLoopExpansion.hh"

void VarLoopExpansion::execute(){
    CFG_analyser = std::make_unique<CFGAnalyse>(module_);
    CFG_analyser->execute();
    std::cout << module_->print() << std::endl;
    find_try();
}

void VarLoopExpansion::find_try(){
    auto loops = CFG_analyser->get_loops();
    for(auto loop : *loops){
        if(loop->size() > 2)
            continue;
        unrool_var_loop(loop);
    }
}

void VarLoopExpansion::unrool_var_loop(std::vector<BasicBlock*>* loop){
    auto entry_bb = *((*loop).rbegin());
    auto body_bb = *((*loop).begin());
    if(entry_bb->get_instructions().size()==0){ return;}
    if(entry_bb->get_pre_basic_blocks().size()>2){ return;}
    if(body_bb->get_pre_basic_blocks().size()>1){ return;}
    if(body_bb->get_succ_basic_blocks().size()>1) {return;}

    auto terminator = entry_bb->get_terminator();
    if(terminator->is_br() && terminator->get_num_operands()==3){
        auto cond = dynamic_cast<Instruction *>(terminator->get_operand(0));
        if(cond==nullptr){ return;}
        if(cond->is_cmp()==false){ return;}

        auto cond_cmp = dynamic_cast<CmpInst *>(cond);
        auto cmp_op = cond_cmp->get_cmp_op();

        auto lval = cond_cmp->get_operand(0);
        auto rval = cond_cmp->get_operand(1);
        auto lval_instr = dynamic_cast<Instruction *>(lval);
        auto rval_instr = dynamic_cast<Instruction *>(rval);
        auto lval_const = dynamic_cast<Constant *>(lval);
        auto rval_const = dynamic_cast<Constant *>(rval);
        if(lval_const||rval_const){ return;}
        //边界条件指令
        Instruction *limit_instr;
        //与边界比较的值
        Instruction *modify_instr;
        //初始变量
        Value *init_instr;
        bool limit_pos_in_left = false;
        int modify_val = 0;
        if(dynamic_cast<PhiInst*>(lval_instr)){
            if(dynamic_cast<PhiInst*>(rval_instr)){ return;}
            if(lval_instr->get_num_operands()>4){ return;}
            limit_instr = rval_instr;
            modify_instr = lval_instr;
            limit_pos_in_left = false;
        }
        else if(dynamic_cast<PhiInst*>(rval_instr)){
            if(rval_instr->get_num_operands()>4){ return;}
            limit_instr = lval_instr;
            modify_instr = rval_instr;
            limit_pos_in_left = true;
        }
        else{ return;}

        auto val1 = modify_instr->get_operand(0);
        auto val1_from = dynamic_cast<BasicBlock*>(modify_instr->get_operand(1));
        auto val2 = modify_instr->get_operand(2);
        int init_pos = 0;
        Instruction *loop_single_instr;

        if(CFG_analyser->find_bb_loop(val1_from)==loop){
            loop_single_instr = dynamic_cast<Instruction *>(val1);
            init_instr = val2;
            init_pos = 1;
        }
        else{
            loop_single_instr = dynamic_cast<Instruction *>(val2);
            init_instr = (val1);
            init_pos = 0;
        }

        if(loop_single_instr==nullptr){ return;}
        if(loop_single_instr->is_add()){
            int const_pos = const_val_check(loop_single_instr);
            if(const_pos==0){ return;}
            auto const_val = dynamic_cast<ConstantInt *>(loop_single_instr->get_operand(const_pos-1));
            modify_val = const_val->get_value();
            auto ret_instr = dynamic_cast<Instruction *>(loop_single_instr->get_operand(2-const_pos));
            if(ret_instr!=modify_instr){ return;}
            if(modify_val!=1){
                if(!(modify_val==-1&&init_pos==0)){
                    return;
                }
            }
        }
        else{ return;}

        //额外判断，
        //初始值一定来自前置块
        //每次的累加/减得到的值在循环体中没有被其他地方使用
        bool use_once = true;
        for(auto instr: entry_bb->get_instructions()){
            if(instr==limit_instr){ return;}
        }
        for(auto instr: body_bb->get_instructions()){
            if(instr==limit_instr){ return;}
        }
        for(auto use: loop_single_instr->get_use_list()){
            auto use_instr = dynamic_cast<Instruction *>(use.val_);
            if(use_instr->get_parent()==body_bb){
                use_once = false;
            }
        }

        //unrool main body
        //处理循环前继块
        BasicBlock *pre_bb;
        for(auto bb: entry_bb->get_pre_basic_blocks()){
            if(bb!=body_bb){
                pre_bb = bb;
            }
        }
        auto &pre_instrs = pre_bb->get_instructions();
        auto pre_terminator = pre_bb->get_terminator();
        pre_instrs.pop_back();
        Instruction *new_init_instr;
        if(modify_val == -1){
            new_init_instr = BinaryInst::create_add(init_instr, ConstantInt::get(-6, module_), pre_bb, module_);
        }
        else{
            new_init_instr = BinaryInst::create_add(init_instr, ConstantInt::get(6, module_), pre_bb, module_);
        }
        pre_instrs.push_back(pre_terminator);
        modify_instr->set_operand(init_pos * 2, new_init_instr);

        ////first unrool
        std::map<Value *, Value *> phi2loop_instr;
        std::map<Value *, Value *> phi2new_instr;
        std::map<Value *, Value *> loop_instr2new_instr;
        std::map<Value *, Value *> old2new_loop_instr;
        BasicBlock* exit_bb;
        auto new_entry_bb = BasicBlock::create(module_, "", modify_instr->get_parent()->get_parent());
        auto new_body_bb = BasicBlock::create(module_, "", modify_instr->get_parent()->get_parent());
        auto mid_bb = BasicBlock::create(module_, "", modify_instr->get_parent()->get_parent());

        
        for(auto instr: entry_bb->get_instructions()){
            if(instr->is_phi()){
                auto val1 = instr->get_operand(0);
                auto val1_from = dynamic_cast<BasicBlock*>(instr->get_operand(1));
                auto val2 = instr->get_operand(2);
                if(CFG_analyser->find_bb_loop(val1_from)==loop){
                    phi2loop_instr[instr] = val1;
                }
                else{
                    phi2loop_instr[instr] = val2;
                }
                phi2new_instr[instr] = nullptr;
            }
            else{
                break;
            }
        }
        // modify new_body_bb
        for(auto instr: entry_bb->get_instructions()){
            auto new_instr = instr->copy_inst(new_entry_bb);
            old2new_loop_instr[instr] = new_instr;
            if(new_instr->is_phi()){
                new_entry_bb->add_instruction(new_instr);
            }
            
        }
        
        if(modify_val == -1){
            auto new_body_entry_instr = BinaryInst::create_add(old2new_loop_instr[modify_instr], ConstantInt::get(6, module_), new_body_bb, module_);
            phi2new_instr[old2new_loop_instr[modify_instr]]  = new_body_entry_instr;
        }
        else{ 
            auto new_body_entry_instr =BinaryInst::create_add(old2new_loop_instr[modify_instr], ConstantInt::get(-6, module_), new_body_bb, module_);
            phi2new_instr[old2new_loop_instr[modify_instr]] = new_body_entry_instr;
        }
        for (int i = 0; i <6; i++){
            for(auto instr: body_bb->get_instructions()){
                if(instr->is_br()){ continue;}
                auto new_instr = instr->copy_inst(new_body_bb);
                for (int idx = 0; idx < new_instr->get_num_operands(); idx++){
                    auto operand = new_instr->get_operand(idx);
                    if(i==0){
                        if(old2new_loop_instr[operand]!=nullptr){
                            operand->remove_use(new_instr);
                            new_instr->set_operand(idx, old2new_loop_instr[operand]);
                            operand = old2new_loop_instr[operand];
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
            for(auto pair: phi2loop_instr){
                phi2new_instr[pair.first] = loop_instr2new_instr[pair.second];
            }
        }
        if(modify_val == -1){
            auto new_body_entry_instr = BinaryInst::create_add(phi2new_instr[modify_instr], ConstantInt::get(-6, module_), new_body_bb, module_);
            phi2new_instr[modify_instr] = new_body_entry_instr;
        }
        else{ 
            auto new_body_entry_instr = BinaryInst::create_add(phi2new_instr[modify_instr], ConstantInt::get(6, module_), new_body_bb, module_);
            phi2new_instr[modify_instr] = new_body_entry_instr;
        }
        pre_terminator->set_operand(0, new_entry_bb);
        BranchInst::create_br(new_entry_bb, new_body_bb);
        
        //unrool new_entry_bb
        for(auto instr: entry_bb->get_instructions()){
            auto new_instr = dynamic_cast<Instruction*>(old2new_loop_instr[instr]);
            if(instr->is_phi()){
                auto val1 = new_instr->get_operand(0);
                auto val1_from = dynamic_cast<BasicBlock*>(new_instr->get_operand(1));
                auto val2 = new_instr->get_operand(2);
                auto val2_from = dynamic_cast<BasicBlock*>(new_instr->get_operand(3));
                if(CFG_analyser->find_bb_loop(val1_from)==loop){
                    val1->remove_use(new_instr);
                    new_instr->set_operand(0, phi2new_instr[instr]);
                    new_instr->set_operand(1, new_body_bb);
                }
                else{
                    val2->remove_use(new_instr);
                    new_instr->set_operand(2, phi2new_instr[instr]);
                    new_instr->set_operand(3, new_body_bb);
                }
            }
            else if(new_instr->is_cmp()){
                auto val1 = new_instr->get_operand(0);
                auto val2 = new_instr->get_operand(1);
                if(old2new_loop_instr[val1]!=nullptr){
                    val1->remove_use(new_instr);
                    new_instr->set_operand(0, old2new_loop_instr[val1]);
                }
                if(old2new_loop_instr[val2]!=nullptr){
                    val2->remove_use(new_instr);
                    new_instr->set_operand(1, old2new_loop_instr[val2]);
                }
            }
            else if(new_instr->is_br()){
                auto cmp_val = new_instr->get_operand(0);
                auto goal_bb1 = dynamic_cast<BasicBlock*>(new_instr->get_operand(1));
                auto goal_bb2 = dynamic_cast<BasicBlock*>(new_instr->get_operand(2));
                 if(goal_bb1==body_bb){
                    exit_bb = goal_bb2;
                    goal_bb2->remove_use(new_instr);
                    new_instr->set_operand(2, mid_bb);
                    new_instr->set_operand(1, new_body_bb);
                }
                else{
                    exit_bb = goal_bb1;
                    goal_bb1->remove_use(new_instr);
                    new_instr->set_operand(1, mid_bb);
                    new_instr->set_operand(2, new_body_bb);
                }
                if(old2new_loop_instr[cmp_val]!=nullptr){
                    cmp_val->remove_use(new_instr);
                    new_instr->set_operand(0, old2new_loop_instr[cmp_val]);
                }
            }
        }
        pre_bb->remove_succ_basic_block(entry_bb);
        pre_bb->add_succ_basic_block(new_entry_bb);
        new_entry_bb->add_succ_basic_block(mid_bb);
        new_entry_bb->add_succ_basic_block(new_body_bb);
        new_entry_bb->add_pre_basic_block(pre_bb);
        new_body_bb->add_pre_basic_block(new_entry_bb);

        ////unrool rest loop
        // unrool rest bodyauto mid_bb = BasicBlock::create(module_, "", modify_instr->get_parent()->get_parent());
        Instruction *new_mid_instr;
        auto rest_entry_bb = BasicBlock::create(module_, "", modify_instr->get_parent()->get_parent());
        auto rest_body_bb = BasicBlock::create(module_, "", modify_instr->get_parent()->get_parent());
        std::map<Value*, Value*> old2rest_loop_instr;

        if(modify_val==-1){
            new_mid_instr = BinaryInst::create_add(old2new_loop_instr[modify_instr], ConstantInt::get(6, module_), mid_bb, module_);
        }
        else{
            new_mid_instr = BinaryInst::create_add(old2new_loop_instr[modify_instr], ConstantInt::get(-6, module_), mid_bb, module_);
        }

        for(auto instr: body_bb->get_instructions()){
            auto new_instr = instr->copy_inst(rest_body_bb);
            old2rest_loop_instr[instr] = new_instr;
        }
        for(auto instr: entry_bb->get_instructions()){
            auto new_instr = instr->copy_inst(rest_entry_bb);
            old2rest_loop_instr[instr] = new_instr;
            if(instr->is_phi()){
                rest_entry_bb->add_instruction(new_instr);
                auto val1 = new_instr->get_operand(0);
                auto val1_from = dynamic_cast<BasicBlock*>(new_instr->get_operand(1));
                auto val2 = new_instr->get_operand(2);
                auto val2_from = dynamic_cast<BasicBlock*>(new_instr->get_operand(3));
                if(val1_from==body_bb){
                    val1->remove_use(new_instr);
                    val1_from->remove_use(new_instr);
                    val2->remove_use(new_instr);
                    val2_from->remove_use(new_instr);
                    if(val1 == loop_single_instr){
                        new_instr->set_operand(2, new_mid_instr);
                    }
                    else{
                        new_instr->set_operand(2, old2new_loop_instr[instr]);
                    }
                    if(old2rest_loop_instr[val1]!=nullptr){
                        new_instr->set_operand(0, old2rest_loop_instr[val1]);
                    }                  
                    new_instr->set_operand(1, rest_body_bb);
                    new_instr->set_operand(3, mid_bb);
                }
                else{
                    val1->remove_use(new_instr);
                    val1_from->remove_use(new_instr);
                    val2->remove_use(new_instr);
                    val2_from->remove_use(new_instr);
                    if(val2 == loop_single_instr){
                        new_instr->set_operand(0, new_mid_instr);
                    }
                    else{
                        new_instr->set_operand(0, old2new_loop_instr[instr]);
                    }
                    new_instr->set_operand(3, rest_body_bb);
                    if(old2rest_loop_instr[val2]!=nullptr){
                        new_instr->set_operand(2, old2rest_loop_instr[val2]);
                    }
                    new_instr->set_operand(1, mid_bb);
                }
            }
            else if(new_instr->is_cmp()){
                auto val1 = new_instr->get_operand(0);
                auto val2 = new_instr->get_operand(1);
                auto val1_instr = dynamic_cast<Instruction *>(val1);
                if(val1_instr->is_phi()){
                    val1->remove_use(new_instr);
                    new_instr->set_operand(0, old2rest_loop_instr[val1]);
                }
                else{
                    val2->remove_use(new_instr);
                    new_instr->set_operand(1, old2rest_loop_instr[val2]);
                }
            }
            else if(new_instr->is_br()){
                auto cmp_val = new_instr->get_operand(0);
                auto val1 = new_instr->get_operand(1);
                auto val1_bb = dynamic_cast<BasicBlock *>(val1);
                auto val2 = new_instr->get_operand(2);
                cmp_val->remove_use(new_instr);
                val1->remove_use(new_instr);
                val2->remove_use(new_instr);
                new_instr->set_operand(0, old2rest_loop_instr[cmp_val]);
                if(val1_bb==body_bb){
                    new_instr->set_operand(1, rest_body_bb);
                    new_instr->set_operand(2, exit_bb);
                }
                else{
                    new_instr->set_operand(1, exit_bb);
                    new_instr->set_operand(2, rest_body_bb);
                }
            }
        }
        for(auto instr: rest_body_bb->get_instructions()){
            for (int idx = 0; idx < instr->get_num_operands(); idx++){
                auto operand = instr->get_operand(idx);
                if(old2rest_loop_instr[operand]!=nullptr){
                    operand->remove_use(instr);
                    instr->set_operand(idx, old2rest_loop_instr[operand]);
                }
            }
        }
        auto new_body_terminator = rest_body_bb->get_terminator();
        new_body_terminator->set_operand(0, rest_entry_bb);
        //创建连接
        BranchInst::create_br(rest_entry_bb, mid_bb);
        mid_bb->add_pre_basic_block(new_entry_bb);
        rest_entry_bb->add_pre_basic_block(rest_body_bb);
        rest_entry_bb->add_succ_basic_block(rest_body_bb);
        rest_entry_bb->add_succ_basic_block(exit_bb);
        rest_body_bb->add_pre_basic_block(rest_entry_bb);
        rest_body_bb->add_succ_basic_block(rest_entry_bb);
        exit_bb->remove_pre_basic_block(entry_bb);
        exit_bb->add_pre_basic_block(rest_entry_bb);

        for(auto instr: entry_bb->get_instructions()){
            if(instr->is_phi()){
                for(auto use : instr->get_use_list()){
                    auto use_instr = dynamic_cast<Instruction *>(use.val_);
                    if(use_instr==nullptr)
                        continue;
                    if(CFG_analyser->find_bb_loop(use_instr->get_parent())!=loop){
                        use_instr->set_operand(use.arg_no_, old2rest_loop_instr[instr]);
                }
            }
            }
        }

        std::vector<Instruction*> delete_list;
        for(auto instr: body_bb->get_instructions()){
            instr->remove_use_of_ops();
            delete_list.push_back(instr);
        }
        for(auto instr: delete_list){
            body_bb->delete_instr(instr);
        }
        delete_list.clear();
        for(auto instr: entry_bb->get_instructions()){
            instr->remove_use_of_ops();
            delete_list.push_back(instr);
        }
        for(auto instr: delete_list){
            entry_bb->delete_instr(instr);
        }
        modify_instr->get_parent()->get_parent()->remove_basic_block(body_bb);
        modify_instr->get_parent()->get_parent()->remove_basic_block(entry_bb);

    }
    else{
        return ;
    }
}

int VarLoopExpansion::const_val_check(Instruction* instr){
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