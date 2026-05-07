#include "LoopInfo.hh"

void LoopInfo::execute()
{
    LoopSearch loopSearch(m_);
    loopSearch.execute();

    auto &loopSet = loopSearch.get_loop_set();
    for(auto loop : loopSet)
    {
        auto newLoop = new Loop(*loop, m_);
        newLoop->set_loop_base(loopSearch.get_loop_base(loop));
        newLoop->find_preheader_and_end();
        newLoop->find_break();
        allLoops.push_back(newLoop);
        loopSearch.printSCC(loop);
        newLoop->find_basic_iduction_var();
        // newLoop->find_dependent_iduction_var();
    }

    

    for(auto loop : allLoops)
    {
        // auto terminal = loop->get_terminal_of_base();
        // std::cout << terminal->print() << std::endl;
        // std::cout << (terminal->is_cmp() || terminal->is_cmpbr() || terminal->is_br()) << std::endl;
        std::cout<<loop->get_ind_vars().size()<<std::endl;
        loop->debug_print_basic_iv();
    }

    // for(auto func:m_->get_functions())
    // {
    //     for(auto bb:func->get_basic_blocks())
    //     {
    //         for(auto inst : bb->get_instructions())
    //         {
    //             std::cout << inst->print() << std::endl;
    //             std::cout << dynamic_cast<Instruction*>(inst->get_operand(0)) << std::endl;
    //             std::cout << dynamic_cast<Instruction*>(inst->get_operand(1)) << std::endl;
    //         }
    //     }
    // }
}

void Loop::find_basic_iduction_var()
{
    for(auto inst : base->get_instructions())
    {
        if(!is_two_prev_phi(inst))
            continue;
        if(!check_phis_prev(inst))
            continue;
        
        auto newIndVal = new InductionVar(inst, InductionVar::IndVarType::basic);
        if(!check_bound_inst_and_set_ind_val(inst, newIndVal))
            continue;
        
        indVars.insert(newIndVal);
        inst2Iv.insert({inst, newIndVal});
    }
}


// 检测phi指令的前驱是否满足：
// 1: 其中一个前驱是一条更新指令
// 2: 另一个前驱不在循环中
bool Loop::check_phis_prev(Instruction *inst)
{
    Value *prevBB1 = inst->get_operand(1);
    Value *prevBB2 = inst->get_operand(3);
    auto test1 = is_update_inst(inst, dynamic_cast<Instruction *>(inst->get_operand(0)));
    auto test2 = is_update_inst(inst, dynamic_cast<Instruction *>(inst->get_operand(2)));
    bool case1 = test1 && !contains(prevBB2);
    bool case2 = !contains(prevBB1) && test2;

    return (case1 || case2);
}

bool Loop::is_update_inst(Instruction *iv, Instruction *inst)
{
    if(!check_update_inst_type(inst))
        return false;
    
    for(auto op : inst->get_operands())
    {
        if(iv == op)
            return true;
    }

    return false;
}

// 获取归纳变量的更新指令
// 这里假定该指令通过了is_two_prev_phi()、check_phis_prev()检测
Instruction *Loop::get_update_inst(Instruction *inst)
{
    Value *prev1 = inst->get_operand(0);
    Value *prev2 = inst->get_operand(2);
    if(contains(prev1))
        return dynamic_cast<Instruction*>(prev1);
    else
        return dynamic_cast<Instruction*>(prev2);
}

// Instruction *Loop::get_step_var(Instruction *inst)
// {
//     Value *prev1 = inst->get_operand(0);
//     Value *prev2 = inst->get_operand(2);
//     if(contains(prev1))
//         return dynamic_cast<Instruction*>(prev1);
//     else
//         return dynamic_cast<Instruction*>(prev2);
// }

// 检测更新指令是否是加法或减法
bool Loop::check_update_inst_type(Instruction *inst)
{
    return (inst != nullptr && (inst->is_add() || inst->is_sub()));
}


void Loop::find_latch()
{
    auto lat = endBlock;
    latchBlocks.insert(lat);
    while(lat->get_pre_basic_blocks().size() == 1)
    {
        auto latPre = *(lat->get_pre_basic_blocks().begin());

        break;
    }

}


void Loop::find_preheader_and_end()
{
    auto base = get_loop_base();
    for(auto preBB : base->get_pre_basic_blocks())
    {
        if(!contains(preBB))
            preHeader = preBB;
        else
            endBlock = preBB;
    }
}


bool Loop::is_loop_invariant(Instruction *inst)
{
    if(inst == nullptr)
        return true;
    if(!contains(inst))
        return true;
    if(!ops_is_invariant(inst))
        return false;

    return true;
}


bool Loop::ops_is_invariant(Instruction *inst)
{
    if(auto phi = dynamic_cast<PhiInst *>(inst))
    {
        for (int i=0; i < (int)phi->get_num_operands()/2; i++)
        {
            auto value = phi->get_operand(i*2);
            if(contains(value))
                return false;
        }
    }
    else if(!inst->is_int_binary())
    {
        return false;
    }
    else
    {
        for(auto op : inst->get_operands())
        {
            if(!is_loop_invariant(op))
                return false;
        }
    }
    return true;
}


Instruction *Loop::get_terminal_of_base()
{
    return base->get_instructions().back();
}


Instruction *Loop::get_terminal_of_bb(BasicBlock *bb)
{
    auto& insts = bb->get_instructions();
    auto inst = insts.back();
    return inst;
}


bool Loop::bound_inst_is_br(Instruction *inst)
{
    return (inst->is_br() || inst->is_cmpbr());
}


bool Loop::check_bound_inst_and_set_ind_val(Instruction *inst, InductionVar *newIndVal)
{
    auto boundInst = base->get_terminator();
    if(!bound_inst_is_br(boundInst))
        return false;


    Value *indVal;
    Value *boundVal;
    if(boundInst->is_cmpbr())
    {
        if(inst == boundInst->get_operand(0))
        {
            indVal = boundInst->get_operand(0);
            boundVal = boundInst->get_operand(1);
        }
        else if(inst == boundInst->get_operand(1))
        {
            indVal = boundInst->get_operand(1);
            boundVal = boundInst->get_operand(0);
        }
        else
            return false;
    }
    else if(boundInst->is_br())
    {
        auto cmp = dynamic_cast<CmpInst *>(boundInst->get_operand(0));
        if(!cmp)
            return false;
        if(inst == cmp->get_operand(0))
        {
            indVal = cmp->get_operand(0);
            boundVal = cmp->get_operand(1);
        }
        else if(inst == cmp->get_operand(1))
        {
            indVal = cmp->get_operand(1);
            boundVal = cmp->get_operand(0);
        }
        else
            return false;
    }

    //不是最简单的形式 while(i<x)，不支持
    if(contains(boundVal))
        return false;
    
    newIndVal->inst = inst;

    set_final_val(boundVal, newIndVal);

    set_direction(newIndVal);

    set_step_val(newIndVal);

    set_init_val(newIndVal);

    set_update_op(newIndVal);

    return true;
}

// bool Loop::check_bound_inst(Instruction *inst)
// {
//     auto boundInst = base->get_terminator();
//     if(!bound_inst_is_br(boundInst))
//         return false;


//     Value *indVal;
//     Value *boundVal;
//     if(boundInst->is_cmpbr())
//     {
//         if(inst == boundInst->get_operand(0))
//         {
//             indVal = boundInst->get_operand(0);
//             boundVal = boundInst->get_operand(1);
//         }
//         else if(inst == boundInst->get_operand(1))
//         {
//             indVal = boundInst->get_operand(1);
//             boundVal = boundInst->get_operand(0);
//         }
//         else
//             return false;
//     }
//     else if(boundInst->is_br())
//     {
//         auto cmp = dynamic_cast<CmpInst *>(boundInst->get_operand(0));
//         if(!cmp->is_cmp())
//             return false;
//         if(inst == cmp->get_operand(1))
//         {
//             indVal = cmp->get_operand(1);
//             boundVal = cmp->get_operand(2);
//         }
//         else if(inst == cmp->get_operand(2))
//         {
//             indVal = cmp->get_operand(2);
//             boundVal = cmp->get_operand(1);
//         }
//         else
//             return false;
//     }

//     //不是最简单的形式 while(i<x)，不支持
//     if(contains(boundVal))
//         return false;
    
//     return true;
// }

void Loop::set_final_val(Value *boundVal, InductionVar *newIndVal)
{
    if(has_break())
        newIndVal->finalVal = nullptr;
    else
        newIndVal->finalVal = boundVal;
}


void Loop::set_direction(InductionVar *newIndVal)
{
    CmpOp direction;
    auto boundInst = base->get_terminator();

    if(boundInst->is_cmpbr())
    {
        auto cmpbr = dynamic_cast<CmpBrInst *>(boundInst);
        cmpbr->get_cmp_op();
    }
    else if(boundInst->is_br())
    {
        auto cmp = dynamic_cast<CmpInst *>(boundInst->get_operand(0));
        direction = cmp->get_cmp_op();
    }
    if(direction == CmpOp::LT || direction == CmpOp::LE)
        newIndVal->direction = InductionVar::Direction::increase;
    else if(direction == CmpOp::GT || direction == CmpOp::GE)
        newIndVal->direction = InductionVar::Direction::decrease;
    else
        newIndVal->direction = InductionVar::Direction::unknown;
}


void Loop::set_step_val(InductionVar *newIndVal)
{
    auto phi = newIndVal->inst;
    auto updateInst = get_update_inst(phi);

    if(updateInst->get_operand(0) == phi)
        newIndVal->stepVal = updateInst->get_operand(1);
    else
        newIndVal->stepVal = updateInst->get_operand(0);
}


void Loop::set_init_val(InductionVar *newIndVal)
{
    auto phi = newIndVal->inst;

    if(!contains(phi->get_operand(1)))
        newIndVal->initVal = phi->get_operand(0);
    else
        newIndVal->initVal = phi->get_operand(2);
}


void Loop::set_update_op(InductionVar *newIndVal)
{
    auto phi = newIndVal->inst;
    auto updateInst = get_update_inst(phi);

    if(updateInst->is_add())
        newIndVal->updateOp = InductionVar::UpdateOp::add;
    else
        newIndVal->updateOp = InductionVar::UpdateOp::sub;
}

void Loop::find_break()
{
    for(auto block : bbSet)
    {
        if(block == base)
            continue;
        
        // if(is_break(get_terminal_of_bb(block)))
        // {
        //     hasBreak = true;
        //     return;
        // }
        for(auto inst : block->get_instructions())
        {
            if(is_break(inst))
            {
                hasBreak = true;
                std::cout<<"有break"<<std::endl;
                return;
            }
        }
    }
    hasBreak = false;
}

// 这里假定inst不在base中
bool Loop::is_break(Instruction *inst)
{
    if(inst->is_br() && !contains(inst->get_operand(0)))
        return true;
    else if(inst->is_br() && dynamic_cast<BranchInst *>(inst)->is_cond_br() && (!contains(inst->get_operand(1)) || !contains(inst->get_operand(2))))
        return true;
    else if(inst->is_cmpbr() && (!contains(inst->get_operand(2)) || !contains(inst->get_operand(3))))
        return true;
    else
        return false;
}


void Loop::debug_print_basic_iv()
{
    for(auto indVar : indVars)
    {
        if(indVar->type == InductionVar::IndVarType::basic)
        {
            std::cout << "基础归纳变量：";
            std::cout << indVar->inst->print() << std::endl;
            std::cout << "终止值：";
            std::cout << indVar->print_final_val() << std::endl;
            std::cout << "步长：";
            std::cout << indVar->stepVal->print() << std::endl;
            std::cout << "方向：";
            std::cout << indVar->print_direction() << std::endl;
        }
        else
        {
            std::cout << "依赖归纳变量：";
            std::cout << indVar->inst->print() << std::endl;
            std::cout << "基：";
            std::cout << indVar->basis->inst->print() << std::endl;
            std::cout << "系数：";
            std::cout << indVar->coef->print() << std::endl;
            std::cout << "常数：";
            std::cout << indVar->cons->print() << std::endl;
        }
    }
}





void Loop::find_dependent_iduction_var()
{
    bool addNew = true;
    while(addNew)
    {
        addNew = false;
        for(auto bb : bbSet)
        {
            for(auto inst : bb->get_instructions())
            {
                if(iv_exists(inst))
                    continue;
                if(inst->is_phi())
                    continue;
                if(inst->is_add()||inst->is_sub()||inst->is_mul())
                {
                    InductionVar *indVar;
                    Value *paraVal;
                    int ivPos;
                    if(indVar = iv_exists(inst->get_operand(0)))
                    {
                        paraVal = inst->get_operand(1);
                        ivPos = 0;
                    }
                    else if(indVar = iv_exists(inst->get_operand(1)))
                    {
                        paraVal = inst->get_operand(0);
                        ivPos = 1;
                    }
                    else
                        continue;
                    
                    if(!is_loop_invariant(paraVal))
                        continue;
                    
                    auto newIv = new InductionVar(inst, Loop::InductionVar::IndVarType::dependent);
                    if(indVar->type == Loop::InductionVar::IndVarType::basic)
                    {
                        newIv->basis = indVar;
                        switch(inst->get_instr_type())
                        {
                            case Instruction::add:
                                newIv->coef = ConstantInt::get(1, m_);
                                newIv->cons = paraVal;
                                break;
                            case Instruction::sub:
                                if(ivPos == 0)
                                {
                                    newIv->coef = ConstantInt::get(1, m_);
                                    newIv->cons = Utils::insert_sub_before_br(ConstantInt::get(0, m_), paraVal, m_, preHeader);
                                }
                                else
                                {
                                    newIv->coef = ConstantInt::get(-1, m_);
                                    newIv->cons = paraVal;
                                }
                                break;
                            case Instruction::mul:
                                newIv->coef = paraVal;
                                newIv->cons = ConstantInt::get(0, m_);
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        newIv->basis = indVar->basis;
                        switch(inst->get_instr_type())
                        {
                            case Instruction::add:
                                newIv->coef = indVar->coef;
                                newIv->cons = Utils::insert_add_before_br(indVar->cons, paraVal, m_, preHeader);
                                break;
                            case Instruction::sub:
                                if(ivPos == 0)
                                {
                                    newIv->coef = indVar->coef;
                                    newIv->cons = Utils::insert_sub_before_br(indVar->cons, paraVal, m_, preHeader);
                                }
                                else
                                {
                                    newIv->coef = Utils::insert_sub_before_br(ConstantInt::get(0, m_), indVar->coef, m_, preHeader);
                                    newIv->cons = Utils::insert_sub_before_br(paraVal, indVar->cons, m_, preHeader);
                                }
                                break;
                            case Instruction::mul:
                                newIv->coef = Utils::insert_mul_before_br(indVar->coef, paraVal, m_, preHeader);
                                newIv->cons = Utils::insert_mul_before_br(indVar->cons, paraVal, m_, preHeader);
                                break;
                            default:
                                break;
                        }
                    }

                    inst2Iv.insert({inst, newIv});
                    indVars.insert(newIv);
                    addNew = true;
                }
            }
        }
    }
}





Loop::InductionVar* Loop::get_basic_iduction_var()
{
    for(auto indVar : indVars)
    {
        if(indVar->type == InductionVar::IndVarType::basic)
            return indVar;
    }
    return nullptr;
}

Loop::InductionVar* Loop::iv_exists(Instruction *inst)
{
    if(auto iter = inst2Iv.find(inst); iter != inst2Iv.end())
        return iter->second;
    else
        return nullptr;
}


Loop::InductionVar* Loop::iv_exists(Value *val)
{
    auto inst = dynamic_cast<Instruction *>(val);
    if(!inst)
        return nullptr;
    else 
        return iv_exists(inst);
}