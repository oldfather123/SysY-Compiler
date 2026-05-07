#include "StrengthReduction.hh"

void StrengthReduction::execute() {
  
    specInstReductStrict();

}

bool StrengthReduction::isNthPower(int x)
{
    return ((x & (x - 1)) == 0);
}

void StrengthReduction::specInstReduct()
{
    std::set<Instruction *> deleteInst;

    for(auto fun:module_->get_functions())
    {
        for(auto bb:fun->get_basic_blocks())
        {
            IRBuilder builder(bb, module_);
            auto &instructions = bb->get_instructions();
            for(auto inst_iter = instructions.begin(); inst_iter != instructions.end(); inst_iter++) 
            {
                auto inst = *inst_iter;
                if(!inst->is_binary())
                {
                    continue;
                }
                Value* value0 = inst->get_operand(0);
                Value* value1 = inst->get_operand(1);

                auto casted = ConstProp::cast_to_const_int(value1);
                if(casted && isNthPower(casted->get_value()))
                {
                    //将第二个操作数为2的n次方的取余操作进行重写
                    // c = a % b
                    // =>
                    // c = a & (b - 1) 
                    // if(inst->is_rem())
                    // {
                    //     auto val1Minus1 = builder.create_isub(value1, ConstantInt::get(1,module_));
                    //     instructions.pop_back();
                    //     bb->add_instruction(inst_iter++, val1Minus1);

                    //     auto res = builder.create_iand(value0, val1Minus1);
                    //     instructions.pop_back();
                    //     bb->add_instruction(inst_iter--, res);
                    //     inst->replace_all_use_with(res);
                    //     deleteInst.insert(inst);
                    // }

                    // if(inst->is_mul())
                    // {
                    //     int val = casted->get_value();
                    //     if(val == 0)
                    //     {
                    //         inst->replace_all_use_with(ConstantInt::get(0,module_));
                    //         deleteInst.insert(inst);
                    //     }
                    //     else if(val > 0)
                    //     {
                    //         int log = 0;
                    //         while(!(val & 1))
                    //         {
                    //             val >>= 1;
                    //             log++;
                    //         }
                    //         auto res = builder.create_lsl(value0, ConstantInt::get(log, module_));
                    //         instructions.pop_back();
                    //         bb->add_instruction(inst_iter, res);
                    //         inst->replace_all_use_with(res);
                    //         deleteInst.insert(inst);
                    //     }
                    // }

                    if(inst->is_div())
                    {
                        int val = casted->get_value();
                        if(val > 0)
                        {
                            int log = 0;
                            while(!(val & 1))
                            {
                                val >>= 1;
                                log++;
                            }
                            auto res = builder.create_asr(value0, ConstantInt::get(log, module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, res);
                            // inst_iter--;
                            inst->replace_all_use_with(res);
                            deleteInst.insert(inst);
                            // bb->delete_instr(inst);
                        }
                    }
                }

                casted = ConstProp::cast_to_const_int(value0);
                if(casted && isNthPower(casted->get_value()))
                {
                    LOG(DEBUG) << "inst";
                    if(inst->is_mul())
                    {
                        int val = casted->get_value();
                        if(val == 0)
                        {
                            inst->replace_all_use_with(ConstantInt::get(0,module_));
                            deleteInst.insert(inst);
                        }
                        else if(val > 0)
                        {
                            int log = 0;
                            while(!(val & 1))
                            {
                                val >>= 1;
                                log++;
                            }
                            auto res = builder.create_lsl(value1, ConstantInt::get(log, module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, res);
                            inst->replace_all_use_with(res);
                            deleteInst.insert(inst);
                        }
                    }
                }
            }

            for(auto inst:deleteInst)
            {
                bb->delete_instr(inst);
            }
        }
    }
}


void StrengthReduction::specInstReductStrict()
{
    std::set<Instruction *> deleteInst;

    for(auto fun:module_->get_functions())
    {
        for(auto bb:fun->get_basic_blocks())
        {
            IRBuilder builder(bb, module_);
            auto &instructions = bb->get_instructions();
            for(auto inst_iter = instructions.begin(); inst_iter != instructions.end(); inst_iter++) 
            {
                auto inst = *inst_iter;
                if(!inst->is_binary())
                {
                    continue;
                }
                Value* value0 = inst->get_operand(0);
                Value* value1 = inst->get_operand(1);

                auto casted = ConstProp::cast_to_const_int(value1);
                if(casted)
                {
                    // 将第二个操作数为2的n次方的取余操作进行重写
                    // c = a % b
                    // =>
                    // c = a & (b - 1) 
                    if(inst->is_rem())
                    {
                        int op2_val = casted->get_value();
                        if(op2_val == 1)
                        {
                            inst->replace_all_use_with(ConstantInt::get(0,module_));
                            deleteInst.insert(inst);
                        }
                        else if(isNthPower(casted->get_value()))
                        {
                            int k = (int)(ceil(std::log2(op2_val)))%32;
                            auto asr = builder.create_asr(value0, ConstantInt::get(31, module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, asr);
                            auto lsr = builder.create_lsr(asr, ConstantInt::get((32-k), module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, lsr);
                            auto add = builder.create_iadd(lsr, value0);
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, add);
                            auto and_ = builder.create_iand(add, ConstantInt::get(-op2_val, module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, and_);
                            auto sub = builder.create_isub(value0, and_);
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, sub);

                            inst->replace_all_use_with(sub);
                            deleteInst.insert(inst);
                        }
                        else
                        {
                            int abs_divisor = (op2_val > 0) ? op2_val : -op2_val;
                            std::cout<<"srem : "<<abs_divisor<<std::endl;
                            int c = 31 + floor(log2(abs_divisor));
                            long long L = pow(2, c);
                            int B = (op2_val > 0) ? (floor(L / abs_divisor) + 1) : - (floor(L / abs_divisor) + 1);
                            auto mulh = builder.create_imul64(value0, ConstantInt::get(B, module_));
                            bb->add_instruction(inst_iter, mulh);
                            instructions.pop_back();
                            auto asr = builder.create_asr64(mulh, ConstantInt::get(c, module_));
                            bb->add_instruction(inst_iter, asr);
                            instructions.pop_back();
                            auto lsr = builder.create_lsr64(mulh, ConstantInt::get(63, module_));
                            bb->add_instruction(inst_iter, lsr);
                            instructions.pop_back();
                            auto add = builder.create_iadd(asr, lsr);
                            bb->add_instruction(inst_iter, add);
                            instructions.pop_back();
                            auto mul = builder.create_imul(add, ConstantInt::get(abs_divisor, module_));
                            bb->add_instruction(inst_iter, mul);
                            instructions.pop_back();
                            auto sub = builder.create_isub(value0, mul);
                            bb->add_instruction(inst_iter, sub);
                            instructions.pop_back();
                            inst->replace_all_use_with(sub);
                            deleteInst.insert(inst);
                        }
                    }

                    if(inst->is_mul())
                    {
                        int op2_val = casted->get_value();
                        if(op2_val == 0)
                        {
                            inst->replace_all_use_with(ConstantInt::get(0,module_));
                            deleteInst.insert(inst);
                        }
                        else if(op2_val == 1)
                        {
                            inst->replace_all_use_with(value0);
                            deleteInst.insert(inst);
                        }
                        else if(op2_val > 0 && isNthPower(casted->get_value()))
                        {
                            int log = 0;
                            while(!(op2_val & 1))
                            {
                                op2_val >>= 1;
                                log++;
                            }
                            auto res = builder.create_lsl(value0, ConstantInt::get(log, module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, res);
                            inst->replace_all_use_with(res);
                            deleteInst.insert(inst);
                        }
                    }

                    if(inst->is_div())
                    {
                        int op2_val = casted->get_value();
                        if(op2_val == 0)
                        {
                            std::cerr<<"divided by zero!"<<std::endl;
                            exit(-1);
                        }
                        else if(op2_val == 1)
                        {
                            inst->replace_all_use_with(value0);
                            deleteInst.insert(inst);
                        }
                        else if(op2_val == 2)
                        {
                            // a / 2
                            // ->
                            // (a - (a >> 31) >> 1 
                            auto sign = builder.create_asr(value0, ConstantInt::get(31, module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, sign);
                            auto sub = builder.create_isub(value0, sign);
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, sub);
                            auto asr = builder.create_asr(value0, ConstantInt::get(1, module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, asr);
                            inst->replace_all_use_with(asr);
                            deleteInst.insert(inst);
                        }
                        else if(isNthPower(casted->get_value()))
                        {
                            int k = (int)(ceil(std::log2(op2_val)))%32;
                            auto sign = builder.create_asr(value0, ConstantInt::get(31, module_));//符号位
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, sign);
                            auto lsr = builder.create_lsr(sign, ConstantInt::get(32-k, module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, lsr);
                            auto add = builder.create_iadd(value0, lsr);
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, add);
                            auto asr = builder.create_asr(add, ConstantInt::get(k, module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, asr);
                            inst->replace_all_use_with(asr);
                            deleteInst.insert(inst);
                        }
                        else
                        {
                            int abs_divisor = (op2_val > 0) ? op2_val : -op2_val;
                            std::cout<<"divisor : "<<abs_divisor<<std::endl;
                            int c = 31 + floor(log2(abs_divisor));
                            long long L = pow(2, c);
                            int B = (op2_val > 0) ? (floor(L / abs_divisor) + 1) : - (floor(L / abs_divisor) + 1);
                            auto mulh = builder.create_imul64(value0, ConstantInt::get(B, module_));
                            bb->add_instruction(inst_iter, mulh);
                            instructions.pop_back();
                            auto asr = builder.create_asr64(mulh, ConstantInt::get(c , module_));
                            bb->add_instruction(inst_iter, asr);
                            instructions.pop_back();
                            auto lsr = builder.create_lsr64(mulh, ConstantInt::get(63, module_));
                            bb->add_instruction(inst_iter, lsr);
                            instructions.pop_back();
                            auto add = builder.create_iadd(asr, lsr);
                            bb->add_instruction(inst_iter, add);
                            instructions.pop_back();
                            inst->replace_all_use_with(add);
                            deleteInst.insert(inst);
                        }
                    }
                }

                casted = ConstProp::cast_to_const_int(value0);
                if(casted && isNthPower(casted->get_value()))
                {
                    LOG(DEBUG) << "inst";
                    if(inst->is_mul())
                    {
                        int val = casted->get_value();
                        if(val == 0)
                        {
                            inst->replace_all_use_with(ConstantInt::get(0,module_));
                            deleteInst.insert(inst);
                        }
                        else if(val > 0)
                        {
                            int log = 0;
                            while(!(val & 1))
                            {
                                val >>= 1;
                                log++;
                            }
                            auto res = builder.create_lsl(value1, ConstantInt::get(log, module_));
                            instructions.pop_back();
                            bb->add_instruction(inst_iter, res);
                            inst->replace_all_use_with(res);
                            deleteInst.insert(inst);
                        }
                    }
                }
            }

            for(auto inst:deleteInst)
            {
                bb->delete_instr(inst);
            }
        }
    }
}