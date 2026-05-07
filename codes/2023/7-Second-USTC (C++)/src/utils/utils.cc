#include "utils.hh"

namespace Utils {
    /*
    // inline std::optional<int> get_const_int_val(Value *v) 
    // {
    //     if (auto cv = dynamic_cast<ConstantInt *>(v))
    //         return cv->get_value();
    //     return std::nullopt;
    // }

    // inline std::optional<float> get_const_float_val(Value *v) 
    // {
    //     if (auto cv = dynamic_cast<ConstantFP *>(v))
    //         return cv->get_value();
    //     return std::nullopt;
    // }
    */

    bool is_integer_alge_inst(Instruction *instr) {
        if (!instr->get_type()->is_integer_type())
            return false;

        return static_cast<IntegerType *>(instr->get_type())->get_num_bits() == 32 &&
               (instr->is_add() || instr->is_sub() || instr->is_mul() || instr->is_div());
    }

    Value* insert_add_before (Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb,
                    std::list<Instruction *>::iterator &inst_iter) 
    {
        auto i32_type = Type::get_int32_type(m);
        if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) 
        {
            LOG(ERROR) << "both ops should be i32 type";
            return nullptr;
        }

        auto v1_c = get_const_int_val(v1);
        auto v2_c = get_const_int_val(v2);
        if (v1_c.has_value() && v2_c.has_value())
            return ConstantInt::get(v1_c.value() + v2_c.value(), m);
        if (v1_c.has_value() && v1_c == 0)
            return v2;
        if (v2_c.has_value() && v2_c == 0)
            return v1;

        auto res = BinaryInst::create_add(v1, v2, bb, m);
        auto & instructions = bb->get_instructions();
        instructions.pop_back();
        bb->add_instruction(inst_iter, res);
        return res;
    }

    Value* insert_sub_before(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb,
                    std::list<Instruction *>::iterator &inst_iter) {
        auto i32_type = Type::get_int32_type(m);
        if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) 
        {
        
            LOG(ERROR) << "both ops should be i32 type";
            return nullptr;
        }
        auto v1_c = get_const_int_val(v1);
        auto v2_c = get_const_int_val(v2);
        if (v1_c.has_value() && v2_c.has_value())
            return ConstantInt::get(v1_c.value() - v2_c.value(), m);
        if (v2_c.has_value() && v2_c == 0)
            return v1;
        
        auto res = BinaryInst::create_sub(v1, v2, bb, m);
        auto & instructions = bb->get_instructions();
        instructions.pop_back();
        bb->add_instruction(inst_iter, res);
        return res;
    }

    Value* insert_mul_before(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb,
                    std::list<Instruction *>::iterator &inst_iter) {
        auto i32_type = Type::get_int32_type(m);
        if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) {
            LOG(ERROR) << "both ops should be i32 type";
            return nullptr;
        }
        auto v1_c = get_const_int_val(v1);
        auto v2_c = get_const_int_val(v2);
        if (v1_c.has_value() && v2_c.has_value())
            return ConstantInt::get(v1_c.value() * v2_c.value(), m);
        if (v1_c.has_value() && v1_c == 1)
            return v2;
        if (v2_c.has_value() && v2_c == 1)
            return v1;
        if (v1_c.has_value() && v1_c == 0 || v2_c.has_value() && v2_c == 0)
            return ConstantInt::get(0, m);
        
        auto res = BinaryInst::create_mul(v1, v2, bb, m);
        auto & instructions = bb->get_instructions();
        instructions.pop_back();
        bb->add_instruction(inst_iter, res);
        return res;
    }

    Value* insert_sdiv_before(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb,
                    std::list<Instruction *>::iterator &inst_iter) {
        auto i32_type = Type::get_int32_type(m);
        if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) {
            LOG(ERROR) << "both ops should be i32 type";
            return nullptr;
        }
        auto v1_c = get_const_int_val(v1);
        auto v2_c = get_const_int_val(v2);
        if (v1_c.has_value() && v2_c.has_value())
            return ConstantInt::get(v1_c.value() / v2_c.value(), m);
        if (v2_c.has_value() && v2_c == 1)
            return v1;
        if (v1_c.has_value() && v1_c == 0)
            return ConstantInt::get(0, m);
        
        auto res = BinaryInst::create_sdiv(v1, v2, bb, m);
        auto & instructions = bb->get_instructions();
        instructions.pop_back();
        bb->add_instruction(inst_iter, res);
        return res;
    }





    Value* insert_add (Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb) 
    {
        auto i32_type = Type::get_int32_type(m);
        if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) 
        {
            LOG_ERROR << "both ops should be i32 type";
            return nullptr;
        }

        auto v1_c = get_const_int_val(v1);
        auto v2_c = get_const_int_val(v2);
        if (v1_c.has_value() && v2_c.has_value())
            return ConstantInt::get(v1_c.value() + v2_c.value(), m);
        if (v1_c.has_value() && v1_c == 0)
            return v2;
        if (v2_c.has_value() && v2_c == 0)
            return v1;

        return BinaryInst::create_add(v1, v2, bb, m);
    }

    Value* insert_sub(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb) {
        auto i32_type = Type::get_int32_type(m);
        if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) 
        {
        
            LOG_ERROR << "both ops should be i32 type";
            return nullptr;
        }
        auto v1_c = get_const_int_val(v1);
        auto v2_c = get_const_int_val(v2);
        if (v1_c.has_value() && v2_c.has_value())
            return ConstantInt::get(v1_c.value() - v2_c.value(), m);
        if (v2_c.has_value() && v2_c == 0)
            return v1;
        
        return BinaryInst::create_sub(v1, v2, bb, m);
    }

    Value* insert_mul(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb) {
        auto i32_type = Type::get_int32_type(m);
        if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) 
        {
            LOG_ERROR << "both ops should be i32 type";
            return nullptr;
        }
        auto v1_c = get_const_int_val(v1);
        auto v2_c = get_const_int_val(v2);
        if (v1_c.has_value() && v2_c.has_value())
            return ConstantInt::get(v1_c.value() * v2_c.value(), m);
        if (v1_c.has_value() && v1_c == 1)
            return v2;
        if (v2_c.has_value() && v2_c == 1)
            return v1;
        if (v1_c.has_value() && v1_c == 0 || v2_c.has_value() && v2_c == 0)
            return ConstantInt::get(0, m);
        
        return BinaryInst::create_mul(v1, v2, bb, m);
    }

    Value* insert_add_before_br (Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb) 
    {
        auto iter = bb->get_instructions().end();
        iter--;
        
        auto i32_type = Type::get_int32_type(m);
        if (!(v1->get_type() == i32_type && v2->get_type() == i32_type)) 
        {
            LOG_ERROR << "both ops should be i32 type";
            return nullptr;
        }

        auto v1_c = get_const_int_val(v1);
        auto v2_c = get_const_int_val(v2);
        if (v1_c.has_value() && v2_c.has_value())
            return ConstantInt::get(v1_c.value() + v2_c.value(), m);
        if (v1_c.has_value() && v1_c == 0)
            return v2;
        if (v2_c.has_value() && v2_c == 0)
            return v1;

        auto res = BinaryInst::create_add(v1, v2, bb, m);
        auto & instructions = bb->get_instructions();
        instructions.pop_back();
        bb->add_instruction(iter, res);
        return res;
        // return insert_add_before(v1, v2, m, bb, iter);
    }

    Value* insert_sub_before_br(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb) {
        auto iter = bb->get_instructions().end();
        iter--;
        
        return insert_sub_before(v1, v2, m, bb, iter);
    }

    Value* insert_mul_before_br(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb) {
        auto iter = bb->get_instructions().end();
        iter--;
        
        return insert_mul_before(v1, v2, m, bb, iter);
    }


    Value* insert_add_begin(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb) 
    {
        auto iter = bb->get_instructions().begin();
        iter++;
        
        return insert_add_before(v1, v2, m, bb, iter);
    }

    Value* insert_sub_begin(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb) {
        auto iter = bb->get_instructions().begin();
        iter++;
        
        return insert_sub_before(v1, v2, m, bb, iter);
    }

    Value* insert_mul_begin(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb) {
        auto iter = bb->get_instructions().begin();
        iter++;
        
        return insert_mul_before(v1, v2, m, bb, iter);
    }
}
