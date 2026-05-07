#include "DeadStoreEli.hh"
#include "ConstProp.hh"

void DeadStoreEli::execute()
{
    for(auto fun : m_->get_functions())
    {
        remove_dead_alloc_naive(fun);
    }

    auto cp = ConstProp(m_);
    cp.execute();

    for(auto fun : m_->get_functions())
    {
        for(auto bb : fun->get_basic_blocks())
        {
            remove_dead_storeoffset(bb);
        }
    }

    for(auto fun : m_->get_functions())
    {
        remove_dead_alloc_naive(fun);
    }
}

void DeadStoreEli::remove_dead_storeoffset(BasicBlock *bb)
{
    std::unordered_set<Instruction *> deleteInst;
    std::map<Value*, std::map<Value*, Instruction*>* > storeList;

    for(auto inst : bb->get_instructions())
    {
        if(inst->is_storeoffset())
        {
            auto rval = inst->get_operand(0);
            auto lval = inst->get_operand(1);
            auto offset = inst->get_operand(2);
            auto gep = dynamic_cast<GetElementPtrInst*>(lval);
            if(!gep)
            {
                continue;
            }
            if(dynamic_cast<GlobalVariable*>(gep->get_operand(0)))
            {
                //跳过全局数组、变量
                continue;
            }


            if(storeList.find(lval) == storeList.end())
            {
                //该store指令第一次出现，插入表中
                auto offsetList = new std::map<Value*, Instruction*>;
                offsetList->insert(std::make_pair(offset, inst));
                storeList.insert(std::make_pair(lval, offsetList));
            }
            else if(storeList[lval]->find(offset) == storeList[lval]->end())
            {
                auto offsetList = storeList[lval];
                (*offsetList).insert(std::make_pair(offset, inst));
            }
            else
            {
                //上一条store指令冗余，删去
                auto offsetList = storeList[lval];
                auto lastInst = (*offsetList)[offset];
                deleteInst.insert(lastInst);
                (*offsetList)[offset] = inst;
            }
        }
        else if(inst->is_loadoffset())
        {
            auto lval = inst->get_operand(0);
            auto offset = inst->get_operand(1);
            auto gep = dynamic_cast<GetElementPtrInst*>(lval);
            if(!gep)
            {
                continue;
            }
            if(dynamic_cast<GlobalVariable*>(gep->get_operand(0)))
            {
                //跳过全局数组、变量
                continue;
            }
            if(storeList.find(lval) != storeList.end() && storeList[lval]->find(offset) != storeList[lval]->end())
            {
                auto offsetList = storeList[lval];
                auto iter = offsetList->find(offset);
                (*offsetList).erase(iter);
            }
        }
        else if(inst->is_call())
        {
            auto opList = inst->get_operands();
            for(auto op : opList)
            {
                auto offsetList = storeList.find(op);
                if(offsetList != storeList.end())
                {
                    storeList.erase(offsetList);
                }
            }
        }
    }

    for(auto inst : deleteInst)
    {
        bb->delete_instr(inst);
    }
}

void DeadStoreEli::remove_dead_store(BasicBlock *bb)
{
    std::unordered_set<Instruction *> deleteInst;
    std::map<Value*, Instruction*> storeList;

    for(auto inst : bb->get_instructions())
    {
        if(inst->is_store())
        {
            auto rval = inst->get_operand(0);
            auto lval = inst->get_operand(1);
            auto iter = storeList.find(lval);
            if(iter == storeList.end())
            {
                //该store指令第一次出现，插入表中
                storeList.insert(std::make_pair(lval, inst));
            }
            else
            {
                //上一条store指令冗余，删去
                deleteInst.insert((*iter).second);
                storeList[lval] = inst;
            }
        }
        else if(inst->is_load())
        {
            auto lval = inst->get_operand(0);
            auto iter = storeList.find(lval);
            if(iter != storeList.end())
            {
                storeList.erase(iter);
            }
        }
    }

    for(auto inst : deleteInst)
    {
        bb->delete_instr(inst);
    }
}

void DeadStoreEli::remove_dead_alloc_naive(Function *fun)
{
    std::unordered_set<Value*> deadAddrs;
    std::list<std::__cxx11::list<Instruction *>::iterator > deadInsts;
    for(auto bb:fun->get_basic_blocks())
    {
        auto& insts = bb->get_instructions();
        for(auto inst_iter = insts.begin(); inst_iter != insts.end(); inst_iter++)
        {
            auto inst = *inst_iter;
            if(inst->is_alloca())
            {
                if(!dynamic_cast<AllocaInst*>(inst)->get_alloca_type()->is_array_type() && inst->get_use_list().size() <= 1)
                {
                    deadAddrs.insert(inst);
                    deadInsts.push_back(inst_iter);
                }
            }
            else if(inst->is_store())
            {
                auto lval = dynamic_cast<StoreInst*>(inst)->get_lval();
                auto rval = dynamic_cast<StoreInst*>(inst)->get_rval();
                if(deadAddrs.find(lval) != deadAddrs.end())
                {
                    deadInsts.push_back(inst_iter);
                    rval->remove_use(inst);
                }
            }
        }
    }

    for(auto inst_iter:deadInsts)
    {
        auto inst = *inst_iter;
        inst->get_parent()->get_instructions().erase(inst_iter);
    }
}