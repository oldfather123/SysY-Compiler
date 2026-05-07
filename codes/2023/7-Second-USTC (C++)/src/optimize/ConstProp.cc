#include "ConstProp.hh"
#include "DominateTree.hh"
#include "IRBuilder.hh"
#include<algorithm>

void ConstProp::execute() {
    //清空一些集合
    deadBlock.clear();
    unchangedGlobalVal.clear();
    //收集未更改的全局变量
    collect_unchanged_global_val();

    for(auto fun:module_->get_functions()){
        if(fun->get_basic_blocks().empty())continue;
        DominateTree dom_tree(module_);
        dom_tree.get_reverse_post_order(fun);
        //& 获取逆后续
        auto reversePostOrder = dom_tree.reverse_post_order;

        for(auto block:reversePostOrder){
            ConstFold(block);
        }
        for(auto bb:reversePostOrder){
            deadBlock.insert(bb);
        }
        BlockWalk(fun,fun->get_entry_block());
        DeadBlockEli(fun);
        RemoveSinglePhi(fun);
    }

    // //数组常量传播
    // for(auto fun:module_->get_functions())
    // {
    //     for(auto bb:fun->get_basic_blocks())
    //     {
    //         ArrayCostProp(bb);
    //     }
    // }
}

ConstantInt *ConstProp::resCalc(Instruction::OpID op, ConstantInt *value1, ConstantInt *value2) {
    int const_value1 = value1->get_value();
    int const_value2 = value2->get_value();
    int res;
    switch (op) {
    case Instruction::add:
        res = const_value1 + const_value2;
        break;
    case Instruction::sub:
        res = const_value1 - const_value2;
        break;
    case Instruction::mul:
        res = const_value1 * const_value2;
        break;
    case Instruction::sdiv:
        res = (int)(const_value1 / const_value2);
        break;
    case Instruction::srem:
        res = const_value1 % const_value2;
        break;
    case Instruction::shl:
        res = const_value1 << const_value2;
        break;
    case Instruction::asr:
        res = const_value1 >> const_value2;
        break;
    case Instruction::lsr:
        res = (int)((unsigned)const_value1 >> const_value2);
        break;
    case Instruction::land:
        res = const_value1 & const_value2;
        break;
    case Instruction::lor:
        res = const_value1 | const_value2;
        break;
    case Instruction::lxor:
        res = const_value1 ^ const_value2;
        break;
    default:
        return nullptr;
        break;
    }
    return ConstantInt::get(res, module_);
}

ConstantFP *ConstProp::resCalc(Instruction::OpID op, ConstantFP *value1, ConstantFP *value2) {
    float const_value1 = value1->get_value();
    float const_value2 = value2->get_value();
    float res;
    switch (op) {
    case Instruction::fadd:
        res = const_value1 + const_value2;
        break;
    case Instruction::fsub:
        res = const_value1 - const_value2;
        break;
    case Instruction::fmul:
        res = const_value1 * const_value2;
        break;
    case Instruction::fdiv:
        res = (float)(const_value1 / const_value2);
        break;
    default:
        return nullptr;
        break;
    }
    return ConstantFP::get(res, module_);
}

ConstantInt *ConstProp::resCalc(CmpOp cmpOp, ConstantInt *value1, ConstantInt *value2){
    int const_value1 = value1->get_value();
    int const_value2 = value2->get_value();
    bool boolRes;
    switch (cmpOp) {
    case EQ:
        boolRes = const_value1 == const_value2;
        break;
    case NE:
        boolRes = const_value1 != const_value2;
        break;
    case GT:
        boolRes = const_value1 > const_value2;
        break;
    case GE:
        boolRes = const_value1 >= const_value2;
        break;
    case LT:
        boolRes = const_value1 < const_value2;
        break;
    case LE:
        boolRes = const_value1 <= const_value2;
        break;
    default:
        return nullptr;
        break;
    }
    return ConstantInt::get(boolRes, module_);
}

ConstantInt *ConstProp::resCalc(CmpOp cmpOp, ConstantFP *value1, ConstantFP *value2){
    float const_value1 = value1->get_value();
    float const_value2 = value2->get_value();
    bool boolRes;
    switch (cmpOp) {
    case EQ:
        boolRes = const_value1 == const_value2;
        break;
    case NE:
        boolRes = const_value1 != const_value2;
        break;
    case GT:
        boolRes = const_value1 > const_value2;
        break;
    case GE:
        boolRes = const_value1 >= const_value2;
        break;
    case LT:
        boolRes = const_value1 < const_value2;
        break;
    case LE:
        boolRes = const_value1 <= const_value2;
        break;
    default:
        return nullptr;
        break;
    }
    return ConstantInt::get(boolRes, module_);
}


ConstantInt *ConstProp::cast_to_const_int(Value *value) {
    auto const_int_ptr = dynamic_cast<ConstantInt *>(value);
    if (const_int_ptr) {
        return const_int_ptr;
    } else {
        return nullptr;
    }
}

ConstantFP *ConstProp::cast_to_const_float(Value *value) {
    auto const_float_ptr = dynamic_cast<ConstantFP *>(value);
    if (const_float_ptr) {
        return const_float_ptr;
    } else {
        return nullptr;
    }
}

void ConstProp::ConstFold(BasicBlock *block){
    std::set<Instruction *> deleteInst;
    std::map<Value*, std::map<Value*, Value*>*> array_map;

    for(auto inst:block->get_instructions()){
        //全局变量常量传播
        if(inst->is_load())
        {
            auto gVar = inst->get_operand(0);
            if(dynamic_cast<GlobalVariable*>(gVar) && unchangedGlobalVal.find(gVar) != unchangedGlobalVal.end())
            {
                auto val = dynamic_cast<GlobalVariable*>(gVar)->get_init();
                if(dynamic_cast<GlobalVariable*>(gVar)->is_zero_initializer())
                {
                    val = ConstantInt::get(0,module_);
                }
                inst->replace_all_use_with(val);
                deleteInst.insert(inst);
            }

        }
        //数组常量传播
        if(inst->is_storeoffset())
        {
            auto rval = inst->get_operand(0);
            auto lval = inst->get_operand(1);
            auto offset = inst->get_operand(2);
            // if(cast_to_const_int(rval))
            // {
                if(array_map.find(lval) == array_map.end())
                {
                    auto offset_map = new std::map<Value*, Value*>;
                    offset_map->insert(std::make_pair(offset, rval));
                    array_map.insert(std::make_pair(lval, offset_map));
                }
                else if(array_map[lval]->find(offset) == array_map[lval]->end())
                {
                    array_map[lval]->insert(std::make_pair(offset, rval));
                }
                else
                {
                    auto offset_map = array_map[lval];
                    (*offset_map)[offset] = rval;
                }
            // }
        }
        else if(inst->is_loadoffset())
        {
            auto lval = inst->get_operand(0);
            auto offset = inst->get_operand(1);
            if(array_map.find(lval) != array_map.end() && array_map[lval]->find(offset) != array_map[lval]->end())
            {
                auto offset_map = array_map[lval];
                auto rval = (*offset_map)[offset];
                inst->replace_all_use_with(rval);
                deleteInst.insert(inst);
            }
        }
        else if(inst->is_call())
        {
            auto opList = inst->get_operands();
            for(auto op : opList)
            {
                auto offset_map = array_map.find(op);
                if(offset_map != array_map.end())
                {
                    array_map.erase(offset_map);
                }
            }
        }
        //基本指令常量传播
        if(inst->is_int_binary()){
            // LOG(DEBUG) << "进入int_binary指令处理";
            Value* value0 = inst->get_operand(0);
            Value* value1 = inst->get_operand(1);
            if(cast_to_const_int(value0) && cast_to_const_int(value1)){
                ConstantInt * res = resCalc(inst->get_instr_type(),cast_to_const_int(value0),cast_to_const_int(value1));
                inst->replace_all_use_with(res);
                deleteInst.insert(inst);
            }
            else if(inst->is_add())
            {
                if(cast_to_const_int(value0) && cast_to_const_int(value0)->get_value() == 0)
                {
                    inst->replace_all_use_with(value1);
                    deleteInst.insert(inst);
                }
                else if(cast_to_const_int(value1) && cast_to_const_int(value1)->get_value() == 0)
                {
                    inst->replace_all_use_with(value0);
                    deleteInst.insert(inst);
                }
            }
            else if(inst->is_sub())
            {
                if(cast_to_const_int(value1) && cast_to_const_int(value1)->get_value() == 0)
                {
                    inst->replace_all_use_with(value0);
                    deleteInst.insert(inst);
                }
            }
            else if(inst->is_mul())
            {
                if(cast_to_const_int(value0) && cast_to_const_int(value0)->get_value() == 0)
                {
                    inst->replace_all_use_with(ConstantInt::get(0,module_));
                    deleteInst.insert(inst);
                }
                else if(cast_to_const_int(value1) && cast_to_const_int(value1)->get_value() == 0)
                {
                    inst->replace_all_use_with(ConstantInt::get(0,module_));
                    deleteInst.insert(inst);
                }
                else if(cast_to_const_int(value0) && cast_to_const_int(value0)->get_value() == 1)
                {
                    inst->replace_all_use_with(value1);
                    deleteInst.insert(inst);
                }
                else if(cast_to_const_int(value1) && cast_to_const_int(value1)->get_value() == 1)
                {
                    inst->replace_all_use_with(value0);
                    deleteInst.insert(inst);
                }
            }
            else if(inst->is_div())
            {
                if(cast_to_const_int(value1) && cast_to_const_int(value1)->get_value() == 1)
                {
                    inst->replace_all_use_with(value0);
                    deleteInst.insert(inst);
                }
            }
        }

        else if(inst->is_float_binary()){
            // LOG(DEBUG) << "进入float_binary指令处理";
            Value* value0 = inst->get_operand(0);
            Value* value1 = inst->get_operand(1);
            if(cast_to_const_float(value0) && cast_to_const_float(value1)){
                ConstantFP * res = resCalc(inst->get_instr_type(),cast_to_const_float(value0),cast_to_const_float(value1));
                inst->replace_all_use_with(res);
                deleteInst.insert(inst);
            }
        }

        else if(inst->is_cmp()){
            // LOG(DEBUG) << "进入cmp指令处理";
            Value* value0 = inst->get_operand(0);
            Value* value1 = inst->get_operand(1);
            if(cast_to_const_int(value0) && cast_to_const_int(value1)){
                ConstantInt * res = resCalc(dynamic_cast<CmpInst *>(inst)->get_cmp_op(),cast_to_const_int(value0),cast_to_const_int(value1));
                inst->replace_all_use_with(res);
                deleteInst.insert(inst);
            }
        }

        else if(inst->is_fcmp()){
            // LOG(DEBUG) << "进入fcmp指令处理";
            Value* value0 = inst->get_operand(0);
            Value* value1 = inst->get_operand(1);
            if(cast_to_const_float(value0) && cast_to_const_float(value1)){
                ConstantInt * res = resCalc(dynamic_cast<FCmpInst *>(inst)->get_cmp_op(),cast_to_const_float(value0),cast_to_const_float(value1));
                inst->replace_all_use_with(res);
                deleteInst.insert(inst);
            }
        }

        else if(inst->is_zext()){
            //& ZEXT指令,只处理向int、float类型的转换
            // LOG(DEBUG) << "进入zext指令处理";
            Type *destType = dynamic_cast<ZextInst *>(inst)->get_dest_type();
            Value* value = inst->get_operand(0);
            if(destType->get_type_id() == Type::IntegerTyID){//dest_type is int
                // LOG(DEBUG) << "目标类型为int";
                if(cast_to_const_int(value)){
                    int res = (int)(cast_to_const_int(value)->get_value());
                    inst->replace_all_use_with(ConstantInt::get(res,module_));
                    deleteInst.insert(inst);
                }
                else if(cast_to_const_float(value)){
                    int res = (int)(cast_to_const_float(value)->get_value());
                    inst->replace_all_use_with(ConstantInt::get(res,module_));
                    deleteInst.insert(inst);
                }
            }
            else if(destType->get_type_id() == Type::FloatTyID){//dest_type is float
                // LOG(DEBUG) << "目标类型为float";
                if(cast_to_const_int(value)){
                    float res = (float)(cast_to_const_int(value)->get_value());
                    inst->replace_all_use_with(ConstantFP::get(res,module_));
                    deleteInst.insert(inst);
                }
                else if(cast_to_const_float(value)){
                    float res = (float)(cast_to_const_float(value)->get_value());
                    inst->replace_all_use_with(ConstantFP::get(res,module_));
                    deleteInst.insert(inst); 
                }
            }
            // LOG(DEBUG) << "退出zext指令处理";
        }

        else if(inst->is_sitofp()){
            // LOG(DEBUG) << "进入sitofp指令处理";
            Value* value = inst->get_operand(0);
            if(cast_to_const_int(value)){
                float res = (float)(cast_to_const_int(value)->get_value());
                inst->replace_all_use_with(ConstantFP::get(res,module_));
                deleteInst.insert(inst);
            }
        }

        else if(inst->is_fptosi()){
            // LOG(DEBUG) << "进入fptosi指令处理";
            Value* value = inst->get_operand(0);
            Type *destType = dynamic_cast<FpToSiInst *>(inst)->get_dest_type();
            unsigned bits = static_cast<IntegerType *>(destType)->get_num_bits();
            if(cast_to_const_float(value)){
                if(bits == 1){
                    bool res = (bool)(cast_to_const_float(value)->get_value());
                    inst->replace_all_use_with(ConstantInt::get(res,module_));
                    deleteInst.insert(inst);
                }
                else{
                    int res = (int)(cast_to_const_float(value)->get_value());
                    inst->replace_all_use_with(ConstantInt::get(res,module_));
                    deleteInst.insert(inst);
                }  
            }
        }

        else if(inst->is_br()){
            // LOG(DEBUG) << "进入br指令处理";
            Value* value = inst->get_operand(0);
            if(cast_to_const_int(value)){
                //& 分支条件为常数
                BasicBlock *TrueBlock = dynamic_cast<BasicBlock *>(dynamic_cast<BranchInst *>(inst)->get_operand(1));
                BasicBlock *FalseBlock = dynamic_cast<BasicBlock *>(dynamic_cast<BranchInst *>(inst)->get_operand(2));
                //& 删除原br指令的前后继关系
                FalseBlock->remove_pre_basic_block(block);
                TrueBlock->remove_pre_basic_block(block);
                block->remove_succ_basic_block(FalseBlock);
                block->remove_succ_basic_block(TrueBlock);
                if(cast_to_const_int(value)->get_value() == 1){
                    //& 分支恒为True
                    deleteInst.insert(inst);
                    IRBuilder builder(block, module_);
                    builder.create_br(TrueBlock);
                    //& 用jump代替br
                    for(auto phi:FalseBlock->get_instructions()){
                        //& 修改phi指令操作数
                        if(!phi->is_phi())break;
                        for (int i=0; i < (int)phi->get_num_operands()/2; i++){
                            if (dynamic_cast<BasicBlock *>(phi->get_operand(i*2 + 1)) == block){
                                phi->remove_operands(2*i, 2*i + 1);
                            }
                        }

                        for (int i=0; i < (int)phi->get_num_operands()/2; i++){
                            auto value = phi->get_operand(i*2);
                            auto bb = phi->get_operand(i*2 + 1);
                            value->remove_use(phi);
                            value->add_use(phi,i*2);
                            bb->remove_use(phi);
                            bb->add_use(phi,i*2+1);
                        }
                    }

                }
                else{
                    //& 分支恒为False
                    deleteInst.insert(inst);
                    IRBuilder builder(block, module_);
                    builder.create_br(FalseBlock);
                    //& 用jump代替br
                    for(auto phi:TrueBlock->get_instructions()){
                        //& 修改phi指令操作数
                        if(!phi->is_phi())break;
                        for (int i=0; i < (int)phi->get_num_operands()/2; i++){
                            if (dynamic_cast<BasicBlock *>(phi->get_operand(i*2 + 1)) == block){
                                phi->remove_operands(2*i, 2*i + 1);
                            } 
                        }
                        for (int i=0; i < (int)phi->get_num_operands()/2; i++){
                            auto value = phi->get_operand(i*2);
                            auto bb = phi->get_operand(i*2 + 1);
                            value->remove_use(phi);
                            value->add_use(phi,i*2);
                            bb->remove_use(phi);
                            bb->add_use(phi,i*2+1);
                        }
                    }
                }
            }
        }
    }
    for(auto inst:deleteInst){
        //& 删除常量传播后的指令
        block->delete_instr(inst);
    }
}


void ConstProp::DeadBlockEli(Function *fun){
    for(auto block:fun->get_basic_blocks()){
        //& 删除phi指令中涉及死块的操作数
        for(auto inst:block->get_instructions()){
            if(!inst->is_phi())break;
            bool test = false;
            for (int i=0; i < (int)inst->get_num_operands()/2; i++){
                if (deadBlock.find(dynamic_cast<BasicBlock *>(inst->get_operand(i*2 + 1))) != deadBlock.end()){
                    inst->remove_operands(2*i, 2*i + 1);
                    i--;
                    test = true;
                } 
            }
            if(test == true)
            {
                for (int i=0; i < (int)inst->get_num_operands()/2; i++)
                {
                    auto value = inst->get_operand(i*2);
                    auto bb = inst->get_operand(i*2 + 1);
                    value->remove_use(inst);
                    value->add_use(inst,i*2);
                    bb->remove_use(inst);
                    bb->add_use(inst,i*2+1);
                }
            }
        }
    }

    for(auto bb:deadBlock){
        //& 删除死块
        std::set<Instruction *> deleteInst;
        for(auto inst : bb->get_instructions())
        {
            deleteInst.insert(inst);
        }
        for(auto inst : deleteInst)
        {
            bb->delete_instr(inst);
        }
        fun->remove_basic_block(bb);
    }
}

void ConstProp::BlockWalk(Function *fun,BasicBlock *block){
    //& 获取不可达基本块集合
    deadBlock.erase(block);
    auto succList = block->get_succ_basic_blocks();
    for(auto succBB:succList){
        if(deadBlock.find(succBB) != deadBlock.end()){
            BlockWalk(fun,succBB);
        }
    }
}

void ConstProp::RemoveSinglePhi(Function *fun){
    for(auto block:fun->get_basic_blocks()){
        std::set<Instruction *>deleteInst;
        for(auto inst:block->get_instructions()){
            if(!inst->is_phi())break;
            auto phi = dynamic_cast<PhiInst *>(inst);
            if(phi->get_num_operands() == 2){
                //& 只有一个操作数的phi指令
                phi->replace_all_use_with(phi->get_operand(0));
                deleteInst.insert(phi);
            }
        }
        for(auto inst:deleteInst){
            block->delete_instr(inst);
        }
    }
}


void ConstProp::collect_unchanged_global_val()
{
    //初始化unchangedGlobalVal
    auto globalVars = m_->get_global_variables();
    for(auto var : globalVars)
    {
        if(unchangedGlobalVal.find(var) == unchangedGlobalVal.end())
        {
            unchangedGlobalVal.insert(var);
        }
    }

    //检测被更改过的GlobalVal
    for(auto fun:m_->get_functions())
    {
        if(fun->get_basic_blocks().empty())continue;
        for(auto bb:fun->get_basic_blocks())
        {
            for(auto inst:bb->get_instructions())
            {
                if(inst->is_store())
                {
                    auto lVal = inst->get_operand(1);
                    if(dynamic_cast<GlobalVariable*>(lVal))
                    {
                        unchangedGlobalVal.erase(lVal);
                    }

                }
            }
        }
    }
}

