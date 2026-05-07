#pragma once

#include <optional>

#include "Value.hh"
#include "BasicBlock.hh"
#include "logging.hh"
#include "Constant.hh"

namespace Utils 
{
     inline std::optional<int> get_const_int_val(Value *v) 
    {
        if (auto cv = dynamic_cast<ConstantInt *>(v))
            return cv->get_value();
        return std::nullopt;
    }

    inline std::optional<float> get_const_float_val(Value *v) 
    {
        if (auto cv = dynamic_cast<ConstantFP *>(v))
            return cv->get_value();
        return std::nullopt;
    }

    bool is_integer_alge_inst(Instruction *instr);

    Value* insert_add_before (Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb,
                    std::list<Instruction *>::iterator &inst_iter);

    Value* insert_sub_before(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb,
                    std::list<Instruction *>::iterator &inst_iter);

    Value* insert_mul_before(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb,
                    std::list<Instruction *>::iterator &inst_iter);

    Value* insert_sdiv_before(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb,
                    std::list<Instruction *>::iterator &inst_iter);

    Value* insert_add(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb);

    Value* insert_sub(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb);

    Value* insert_mul_before_br(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb);

    Value* insert_add_before_br(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb);

    Value* insert_sub_before_br(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb);

    Value* insert_mul_before_br(Value* v1,
                    Value* v2,
                    Module *m,
                    BasicBlock *bb);


}
