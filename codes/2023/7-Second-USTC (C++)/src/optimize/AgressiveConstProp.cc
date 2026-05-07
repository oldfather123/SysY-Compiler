#include "AgressiveConstProp.hh"
#include "DominateTree.hh"
#include "IRBuilder.hh"
#include<algorithm>

void AgressiveConstProp::execute() {

    if(!test())
        return;
    std::cout<<"AgressiveConstProp 通过"<<std::endl;
    auto loop_exp = new LoopExpansion(m_);
    // auto loop_invariant = new LoopInvariant(m_);

    loop_exp->execute();
    // loop_invariant->execute();

    for(auto fun:module_->get_functions())
    {
        if(fun->get_basic_blocks().empty())
            continue;

        collect_gep(fun);
        array_const_prop(fun);
    }

}

bool AgressiveConstProp::test()
{
    for(auto fun:module_->get_functions())
    {
        if(fun->get_basic_blocks().empty())
            continue;

        for(auto bb:fun->get_basic_blocks())
        {
            for(auto inst : bb->get_instructions())
            {
                if(inst->is_loadoffset())
                {
                    auto offset = dynamic_cast<LoadOffsetInst*>(inst)->get_offset();
                    if(!dynamic_cast<ConstantInt*>(offset) && !dynamic_cast<ConstantFP*>(offset))
                        return false;
                }
                else if(inst->is_storeoffset())
                {
                    auto offset = dynamic_cast<StoreOffsetInst*>(inst)->get_offset();
                    auto rval = dynamic_cast<StoreOffsetInst*>(inst)->get_rval();
                    if(dynamic_cast<ConstantInt*>(rval) && dynamic_cast<ConstantInt*>(rval)->get_value() == 0)
                        continue;
                    if(!dynamic_cast<ConstantInt*>(offset) && !dynamic_cast<ConstantFP*>(offset))
                        return false;
                }
                else if(inst->is_call())
                {
                    auto callInst = dynamic_cast<CallInst*>(inst);
                    int i=0;
                    for(auto param : callInst->get_operands())
                    {
                        if(!i)
                            i++;
                        else if(!param->get_type()->is_integer_type())
                            return false;
                        else if(i>=2)
                            return false;
                    }
                }
            }
        }
    }

    return true;
}



void AgressiveConstProp::collect_gep(Function *fun)
{
    std::unordered_map<Value *, GetElementPtrInst *> gepInsts;
    std::vector<Instruction *> deleteInst;
    BasicBlock *entry = fun->get_entry_block();

    // 初始化gepInsts
    for(auto inst : entry->get_instructions())
    {
        if(inst->is_gep())
        {
            if(gepInsts.find(inst->get_operand(0)) == gepInsts.end())
                gepInsts.insert({inst->get_operand(0),dynamic_cast<GetElementPtrInst*>(inst)});
            else
            {
                inst->replace_all_use_with(gepInsts[inst->get_operand(0)]);
                deleteInst.push_back(inst);
            }
        }
    }

    for(auto bb : fun->get_basic_blocks())
    {
        if(bb == entry)
            continue;
        
        for(auto inst : bb->get_instructions())
        {
            if(!inst->is_gep())
                continue;
            
            if(gepInsts.find(inst->get_operand(0)) == gepInsts.end())
            {
                insert_gep_in_entry_end(inst, entry);
                gepInsts.insert({inst->get_operand(0),dynamic_cast<GetElementPtrInst*>(inst)});
            }
            else
            {
                inst->replace_all_use_with(gepInsts[inst->get_operand(0)]);
            }
            deleteInst.push_back(inst);
        }
    }
}


// 这里假定已经经过LIR
bool AgressiveConstProp::gep_equal(Value *value1,Value *value2)
{
    auto gep1 = dynamic_cast<GetElementPtrInst*>(value1);
    auto gep2 = dynamic_cast<GetElementPtrInst*>(value2);
    if(!gep1 || !gep2)
        return false;
    
    return (gep1->get_operand(0) == gep2->get_operand(0));
}


void AgressiveConstProp::insert_gep_in_entry_end(Instruction *instruction, BasicBlock *entry)
{
    auto iter = entry->get_instructions().end();
    iter--;

    entry->add_instruction(iter, instruction);
}


void AgressiveConstProp::array_const_prop(Function *fun)
{
    uselessStoreoffsets.clear();
    collect_useless_storeoffset(fun);
    for(auto bb : fun->get_basic_blocks())
    {
        if(bb->get_terminator()->is_ret())
        {
            for(auto pair : uselessStoreoffsets[bb])
            {
                for(auto inst : pair.second)
                {
                    inst->get_parent()->delete_instr(inst);
                }
            }
        }
    }
}

void AgressiveConstProp::collect_useless_storeoffset(Function *fun)
{
    if(fun->get_basic_blocks().empty())
        return;

    BasicBlock *entry = fun->get_entry_block();
    DominateTree dom_tree(module_);
    dom_tree.get_reverse_post_order(fun);
    auto reversePostOrder = dom_tree.reverse_post_order;//& 获取逆后续
    std::vector<Instruction *>deleteInst;
    
    bool changed = true;
    int i = 4;
    while(i)
    {
        changed = false;
        i--;
        for(auto bb : reversePostOrder)
        {
            auto input = combine_inputs(bb);
            // std::unordered_map<Addr, std::unordered_set<StoreOffsetInst *>, Addr::Hash, Addr::Equal> output;
            // for(auto pre : bb->get_pre_basic_blocks())
            // {
            //     auto input = uselessStoreoffsets[bb];
            //     for(auto pair : input)
            //     {
            //         if(output.find(pair.first) == output.end())
            //         {
            //             output.insert(pair);
            //         }
            //         else
            //         {
            //             output[pair.first].insert(pair.second.begin(), pair.second.end());
            //         }
            //     }
            // }
            // auto input = output;

            for(auto inst : bb->get_instructions())
            {
                if(inst->is_storeoffset())
                {
                    auto storeoffset = dynamic_cast<StoreOffsetInst*>(inst);
                    auto gepInst = dynamic_cast<GetElementPtrInst*>(storeoffset->get_lval());
                    if(input.find(Addr(gepInst, storeoffset->get_offset())) == input.end())
                    {
                        input.insert({Addr(gepInst, storeoffset->get_offset()), {storeoffset}});
                    }
                    else
                    {
                        for(auto inst : input[Addr(gepInst, storeoffset->get_offset())])
                        {
                            // input[Addr(gepInst, storeoffset->get_offset())].erase(inst);
                            deleteInst.push_back(inst);
                        }

                        auto xx = storeoffset->get_offset();
                        input[Addr(gepInst, storeoffset->get_offset())].clear();
                        input[Addr(gepInst, storeoffset->get_offset())].insert(storeoffset);
                    }
                }
                else if(inst->is_loadoffset())
                {
                    auto loadffset = dynamic_cast<LoadOffsetInst*>(inst);
                    auto gepInst = dynamic_cast<GetElementPtrInst*>(loadffset->get_lval());
                    if(input.find(Addr(gepInst, loadffset->get_offset())) != input.end())
                    {
                        if(input[Addr(gepInst, loadffset->get_offset())].size() == 1)
                        {
                            auto storeoffset = (*input[Addr(gepInst, loadffset->get_offset())].begin());
                            deleteInst.push_back(storeoffset);
                            deleteInst.push_back(loadffset);
                            loadffset->replace_all_use_with(storeoffset->get_rval());
                            input[Addr(gepInst, loadffset->get_offset())].clear();
                        }
                        else if(input[Addr(gepInst, loadffset->get_offset())].size() > 1)
                        {
                            input[Addr(gepInst, loadffset->get_offset())].clear();
                        }
                    }
                }
            }

            uselessStoreoffsets[bb] = input;

            for(auto inst : deleteInst)
            {
                inst->get_parent()->delete_instr(inst);
            }
        }
    }

    // for(auto bb : reversePostOrder)
    // {
    //     for(auto inst : bb->get_instructions())
    //     {
    //         if(inst->is_storeoffset())
    //         {
    //             auto storeoffset = dynamic_cast<StoreOffsetInst*>(inst);
    //             if(uselessStoreoffsets.find(storeoffset) == uselessStoreoffsets.end())
    //             {
    //                 uselessStoreoffsets.insert(storeoffset);
    //             }
    //         }
    //     }
    // }

}

std::unordered_map<Addr, std::unordered_set<StoreOffsetInst *>, Addr::Hash, Addr::Equal> AgressiveConstProp::combine_inputs(BasicBlock *bb)
{
    std::unordered_map<Addr, std::unordered_set<StoreOffsetInst *>, Addr::Hash, Addr::Equal> output = {};
    for(auto pre : bb->get_pre_basic_blocks())
    {
        if(uselessStoreoffsets.find(pre) == uselessStoreoffsets.end())
            continue;
        auto input = uselessStoreoffsets[pre];
        for(auto pair : input)
        {
            if(output.find(pair.first) == output.end())
            {
                output.insert(pair);
            }
            else
            {
                output[pair.first].insert(pair.second.begin(), pair.second.end());
            }
        }
    }

    return output;
}