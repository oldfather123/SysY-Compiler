#include "RecSeq.hh"

bool RecSeq::findRecFun()
{

    for(auto fun : m_->get_functions())
    {
        if(fun->is_declaration())
            continue;
        
        std::cout<<fun->get_name()<<std::endl;
        std::vector<Value *> tempParams;
        if(fun->get_args().size() > 2)
        {
            //参数过多
            return false;
        }
        for(auto param : fun->get_args())
        {
            // 数组参数
            if(param->get_type()->is_pointer_type())
                return false;

            tempParams.push_back(param);
        }

        bool added = false;
        int tempRecParamNum = 0;
        Value *tempRecParam;
        Value *tempSlaveParam;
        CmpInst *tempCondInst;
        BranchInst *tempBrInst;
        BasicBlock *tempRetBB;
        BasicBlock *tempBodyEntry;
        BasicBlock *tempBodyEntryPre;
        std::vector<Value *> tempUpdateInsts;
        for(auto bb : fun->get_basic_blocks())
        {
            for(auto inst : bb->get_instructions())
            {
                if(inst->is_store())
                {
                    // 修改了全局变量
                    auto lval = dynamic_cast<StoreInst*>(inst)->get_lval();
                    if(dynamic_cast<GlobalVariable*>(lval))
                        return false;
                }
                else if(inst->is_gep())
                {
                    // 包含数组
                    return false;
                }
                else if(inst->is_call())
                {
                    auto callInst = dynamic_cast<CallInst*>(inst);
                    auto callee = dynamic_cast<Function *>(callInst->get_operand(0));
                    if(callee != fun)
                    {
                        // 调用了其他函数
                        return false;
                    }
                }
                else if(inst->is_cmp())
                {
                    auto cmpInst = dynamic_cast<CmpInst*>(inst);
                    auto lval = cmpInst->get_operand(0);
                    auto rval = cmpInst->get_operand(1);
                    auto it_l = std::find(tempParams.begin(), tempParams.end(), lval);
                    auto it_r = std::find(tempParams.begin(), tempParams.end(), rval);
                    //其中一个是参数，另外一个是常数
                    if(it_l != tempParams.end() && dynamic_cast<ConstantInt*>(rval) && dynamic_cast<ConstantInt*>(rval)->get_value() == 0)
                    {
                        tempRecParam = lval;
                        tempRecParamNum++;
                    }
                    else if(it_r != tempParams.end() && dynamic_cast<ConstantInt*>(lval) && dynamic_cast<ConstantInt*>(lval)->get_value() == 0)
                    {
                        tempRecParam = rval;
                        tempRecParamNum++;
                    }
                    // 多于一个递推参数或没有递推参数
                    if(tempRecParamNum != 1)
                        return false;

                    tempCondInst = cmpInst;
                }
                else if(inst->is_br())
                {
                    auto brInst = dynamic_cast<BranchInst*>(inst);
                    auto cond = brInst->get_operand(0);
                    auto trueBB = dynamic_cast<BasicBlock*>(brInst->get_operand(1));
                    auto falseBB = dynamic_cast<BasicBlock*>(brInst->get_operand(2));
                    if(!tempCondInst || cond != tempCondInst)
                    {
                        continue;
                    }
                    tempBodyEntry = falseBB;
                    tempBodyEntryPre = bb;
                    tempBrInst = brInst;
                }
                else if(inst->is_add() || inst->is_sub())
                {
                    auto lval = inst->get_operand(0);
                    auto rval = inst->get_operand(1);
                    if(tempRecParam && lval == tempRecParam)
                    {
                        tempUpdateInsts.push_back(inst);
                    }
                    else if(tempRecParam && rval == tempRecParam)
                    {
                        tempUpdateInsts.push_back(inst);
                    }
                    
                }
            }
        }

        bool test = false;
        for(auto bb : fun->get_basic_blocks())
        {
            for(auto inst : bb->get_instructions())
            {
                if(inst->is_call())
                {
                    auto callInst = dynamic_cast<CallInst*>(inst);
                    auto callee = dynamic_cast<Function *>(callInst->get_operand(0));
                    for(int i = 1; i < callInst->get_num_operands();i++)
                    {
                        // 若递归调用自己时，传入了递推参数，则测试通过
                        auto it = std::find(tempUpdateInsts.begin(), tempUpdateInsts.end(), callInst->get_operand(i));
                        if(it != tempUpdateInsts.end())
                        {
                            test = true;
                        }
                    }
                    if(!added)
                    {
                        auto recFun = new RecurFun();
                        recurFuns.push_back(recFun);
                        recFun->recInsts.push_back(inst);
                        added = true;
                    }
                    else
                    {
                        auto recFun = recurFuns.back();
                        recFun->recInsts.push_back(inst);
                    }
                }
                else if(inst->is_ret())
                {
                    tempRetBB = bb;
                }
            }
        }
        if(!test)
            return false;

        // succ！
        auto recFun = recurFuns.back();
        recFun->bodyEntry = tempBodyEntry;
        recFun->bodyEntryPre = tempBodyEntryPre;
        recFun->condInst = tempCondInst;
        recFun->brInst = tempBrInst;
        recFun->recParam = tempRecParam;
        recFun->params = tempParams;
        recFun->func = fun;
        recFun->retBlock = tempRetBB;
        if(fun->get_args().size() == 2)
        {
            int i = 0;
            for(auto param : fun->get_args())
            {
                if(param != recFun->recParam)
                {    
                    recFun->slaveParam = param;
                    recFun->slaveParamIdx = i;
                }
                else
                {
                    recFun->recParamIdx = i;
                }
                i++;
            }
        }
        this->recFun = recFun;
        return true; 
    }
    return false;
   
}

void RecSeq::execute()
{
    bool find = findRecFun();
    if(!find)
        return;
    for(auto recFun : recurFuns)
    {
        // std::cout<<recFun->func->print()<<std::endl;
        std::cout<<recFun->recParamIdx<<std::endl;
        std::cout<<recFun->slaveParamIdx<<std::endl;
    }
    locate_rec_fun_call();
    // 从参数不为常数，则跳过
    auto it = recCallInst->get_operand(recFun->slaveParamIdx+1);
    auto xx = dynamic_cast<ConstantFP*>(it);
    if(recFun->slaveParam && !dynamic_cast<ConstantInt*>(recCallInst->get_operand(recFun->slaveParamIdx+1)) && !dynamic_cast<ConstantFP*>(recCallInst->get_operand(recFun->slaveParamIdx+1)))
        return;
    if(dynamic_cast<ConstantInt*>(recCallInst->get_operand(recFun->slaveParamIdx+1)))
    {
        i_slaveInitVal = dynamic_cast<ConstantInt*>(recCallInst->get_operand(recFun->slaveParamIdx+1))->get_value();
    }
    else
    {
        f_slaveInitVal = dynamic_cast<ConstantFP*>(recCallInst->get_operand(recFun->slaveParamIdx+1))->get_value();
    }
    slaveInitVal = recCallInst->get_operand(recFun->slaveParamIdx+1);

    // prepare();
    addRecord();
}

Instruction *RecSeq::locate_rec_fun_call()
{
    auto mainFun = m_->get_functions().back();
    std::cout<<mainFun->print()<<std::endl;

    auto f = recurFuns.front()->func;

    CallInst* recCallInst_;
    int cnt = 0;
    for(auto bb : mainFun->get_basic_blocks())
    {
        for(auto inst : bb->get_instructions())
        {
            if(inst->is_call())
            {
                auto callInst = dynamic_cast<CallInst*>(inst);
                auto callFun = dynamic_cast<Function *>(callInst->get_operand(0));
                if(callFun && callFun == f)
                {
                    recCallInst_ = callInst;
                    cnt++;
                }
            }
        }
    }
    if(cnt != 1)
    {
        return nullptr;
    }

    this->recCallInst = recCallInst_;
    return recCallInst;
}

void RecSeq::prepare()
{
    std::vector<Constant *> foo;
    auto retType = recFun->func->get_return_type();
    if(retType->is_integer_type())
    {
        for(int i = 0; i < 90; i++) 
        {
            foo.push_back(ConstantInt::get(-1,m_));
        }
    }
    else if(retType->is_float_type())
    {
        for(int i = 0; i < 90; i++) 
        {
            foo.push_back(ConstantFP::get(-1,m_));
        }
    }
    
    auto initializer = ConstantArray::get(ArrayType::get(retType, 90), foo);
    auto record = GlobalVariable::create("record", m_, retType, true, initializer);
}

void RecSeq::addRecord()
{
    std::vector<Constant *> foo;
    auto retType = recFun->func->get_return_type();
    if(retType->is_integer_type())
    {
        for(int i = 0; i < 90; i++) 
        {
            foo.push_back(ConstantInt::get(-1,m_));
        }
    }
    else if(retType->is_float_type())
    {
        for(int i = 0; i < 90; i++) 
        {
            foo.push_back(ConstantFP::get(-1,m_));
        }
    }
    
    auto initializer = ConstantArray::get(ArrayType::get(retType, 90), foo);
    auto record = GlobalVariable::create("record", m_, ArrayType::get(retType, 90), false, initializer);



    auto if1 = BasicBlock::create(module_, "", recFun->func);
    auto if2 = BasicBlock::create(module_, "", recFun->func);
    auto computeOffsetBB = BasicBlock::create(module_, "", recFun->func);
    auto elseBB = BasicBlock::create(module_, "", recFun->func);
    auto foundRecordBB = BasicBlock::create(module_, "", recFun->func);

    IRBuilder builder_if1(if1, module_);
    if1->add_pre_basic_block(recFun->bodyEntryPre);
    if1->add_succ_basic_block(if2);
    if1->add_succ_basic_block(computeOffsetBB);
    recFun->bodyEntryPre->remove_succ_basic_block(recFun->bodyEntry);
    recFun->bodyEntryPre->add_succ_basic_block(if1);
    // recFun->brInst->remove_operands(2, 2);
    recFun->brInst->remove_use(recFun->brInst->get_operand(2));
    recFun->brInst->set_operand(2, if1);
    auto fcmp = builder_if1.create_fcmp_eq(recFun->slaveParam, ConstantFP::get(f_slaveInitVal, m_));
    auto condBr = builder_if1.create_cond_br(fcmp, computeOffsetBB, if2);

    IRBuilder builder_if2(if2, module_);
    if2->add_pre_basic_block(if1);
    if2->add_succ_basic_block(elseBB);
    if2->add_succ_basic_block(computeOffsetBB);
    fcmp = builder_if2.create_fcmp_eq(recFun->slaveParam, ConstantFP::get(f_slaveInitVal*2, m_));
    condBr = builder_if2.create_cond_br(fcmp, computeOffsetBB, elseBB);

    IRBuilder builder_else(elseBB, module_);
    elseBB->add_pre_basic_block(if2);
    elseBB->add_succ_basic_block(computeOffsetBB);
    condBr = builder_else.create_br(computeOffsetBB);

    IRBuilder builder_offset(computeOffsetBB, module_);
    computeOffsetBB->add_pre_basic_block(if1);
    computeOffsetBB->add_pre_basic_block(if2);
    computeOffsetBB->add_pre_basic_block(elseBB);
    computeOffsetBB->add_succ_basic_block(foundRecordBB);
    computeOffsetBB->add_succ_basic_block(recFun->bodyEntry);
    recFun->bodyEntry->add_pre_basic_block(computeOffsetBB);
    recFun->bodyEntry->remove_pre_basic_block(recFun->bodyEntryPre);
    auto phi = PhiInst::create_phi(recFun->recParam->get_type(), computeOffsetBB);
    phi->add_phi_pair_operand(ConstantInt::get(30, m_), if1);
    phi->add_phi_pair_operand(ConstantInt::get(60, m_), if2);
    phi->add_phi_pair_operand(ConstantInt::get(0, m_), elseBB);
    // newphi->set_lval(var);
    computeOffsetBB->add_instr_begin(phi);
    auto addr = builder_offset.create_iadd(recFun->recParam, phi);
    auto gep = builder_offset.create_gep(record, {ConstantInt::get(0, m_), addr});
    auto load = builder_offset.create_load(gep);
    fcmp = builder_offset.create_fcmp_ne(load, ConstantFP::get(-1, m_));
    condBr = builder_offset.create_cond_br(fcmp, foundRecordBB, recFun->bodyEntry);

    IRBuilder builder_found(foundRecordBB, module_);
    foundRecordBB->add_pre_basic_block(computeOffsetBB);
    foundRecordBB->add_succ_basic_block(recFun->retBlock);
    condBr = builder_found.create_br(recFun->retBlock);

    IRBuilder builder_body(recFun->bodyEntry, module_);
    auto iter = recFun->bodyEntry->get_instructions().end();
    iter--;
    iter--;
    auto sub = *iter;
    auto store = builder_body.create_store(sub, gep);
    recFun->bodyEntry->get_instructions().pop_back();
    iter++;
    sub->get_parent()->add_instruction(iter, store);

    IRBuilder builder_ret(recFun->retBlock, module_);
    recFun->retBlock->add_pre_basic_block(foundRecordBB);
    auto retPhi = recFun->retBlock->get_instructions().front();
    // phi = PhiInst::create_phi(recFun->func->get_return_type(), recFun->retBlock);
    dynamic_cast<PhiInst*>(retPhi)->add_phi_pair_operand(load, foundRecordBB);
    // phi->add_phi_pair_operand(ConstantInt::get(60, m_), recFun->bodyEntry);
    // newphi->set_lval(var);
    // recFun->retBlock->add_instr_begin(phi);
}