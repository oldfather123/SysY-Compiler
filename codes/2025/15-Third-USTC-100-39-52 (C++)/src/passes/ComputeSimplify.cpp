
#include "ComputeSimplify.hpp"
#include <cmath>
#include "Instclone.hpp"
#include "IRBuilder.hpp"

void ComputeSimplify::run()
{
    for (auto &f : m_->get_functions())
    {
        BasicBlock *newblock = BasicBlock::create(m_, "", &f);
        for (auto &bb : f.get_basic_blocks())
        {
            for (auto &inst : bb.get_instructions())
            {
                switch (inst.get_instr_type())
                {
                case Instruction::add:
                    add_simplify(&inst, newblock);
                    break;
                case Instruction::sub:
                    sub_simplify(&inst, newblock);
                    break;
                case Instruction::mul:
                    mul_simplify(&inst, newblock);
                    break;
                // case Instruction::sdiv:
                //     div_simplify(&inst, newblock);
                //     break;
                case Instruction::srem:
                    srem_simplify(&inst, newblock);
                    break;

                default:
                    break;
                }
            }
        }
        newblock->erase_from_parent();
    }
}

void ComputeSimplify::add_simplify(Instruction *inst, BasicBlock *putinstbb)
{
    auto op1 = inst->get_operand(0);
    auto op2 = inst->get_operand(1);

    // 0+x
    if (auto constnum = dynamic_cast<ConstantInt *>(op1))
    {
        if (constnum->get_value() == 0)
        {
            inst->replace_all_use_with(op2);
            return;
        }
    }
    // x+0
    if (auto constnum = dynamic_cast<ConstantInt *>(op2))
    {
        if (constnum->get_value() == 0)
        {
            inst->replace_all_use_with(op1);
            return;
        }
    }
}

void ComputeSimplify::sub_simplify(Instruction *inst, BasicBlock *putinstbb)
{
    auto op1 = inst->get_operand(0);
    auto op2 = inst->get_operand(1);

    // x-0
    if (auto constnum = dynamic_cast<ConstantInt *>(op2))
    {
        if (constnum->get_value() == 0)
        {
            inst->replace_all_use_with(op1);
            return;
        }
    }
    // 0-x和-x开销差不多
}

// void ComputeSimplify::mul_simplify(Instruction *inst,BasicBlock* putinstbb)
// {
//     auto op1 = inst->get_operand(0);
//     auto op2 = inst->get_operand(1);

//     if (auto constnum = dynamic_cast<ConstantInt*>(op1)) {
//         // 0*x
//         if (constnum->get_value() == 0) {
//             inst->replace_all_use_with(op1);
//             return;
//         }
//         // 1*x
//         if (constnum->get_value() == 1) {
//             inst->replace_all_use_with(op2);
//             return;
//         }
//         if (constnum->get_value() == -1) {
//             auto zero = ConstantInt::get(0,m_);
//             auto sub_inst = IBinaryInst::create_sub(zero,op2,putinstbb);
//             to_move.push_back(sub_inst);
//             move_instrs(inst->get_parent(),putinstbb,to_move,inst);
//             to_move.clear();
//             inst->replace_all_use_with(sub_inst);
//             return;
//         }

//         // x * 2^n => x << n
//         auto val = constnum->get_value();
//         if (val > 0 && (val & (val - 1)) == 0) { // 判断是否 2 的 n 次方
//             int n = static_cast<int>(log2(val));
//             auto const_n = ConstantInt::get(n,m_);
//             auto shl_inst = IBinaryInst::create_shl(op2, const_n,putinstbb);
//             to_move.push_back(shl_inst);
//             move_instrs(inst->get_parent(),putinstbb,to_move,inst);
//             to_move.clear();
//             inst->replace_all_use_with(shl_inst);
//             return;
//         }
//     }

//     if (auto constnum = dynamic_cast<ConstantInt*>(op2)) {
//         if (constnum->get_value() == 0) {
//             inst->replace_all_use_with(op2);
//             return;
//         }
//         if (constnum->get_value() == 1) {
//             inst->replace_all_use_with(op1);
//             return;
//         }
//         if (constnum->get_value() == -1) {
//             auto zero = ConstantInt::get(0,m_);
//             auto sub_inst = IBinaryInst::create_sub(zero,op1,putinstbb);
//             to_move.push_back(sub_inst);
//             move_instrs(inst->get_parent(),putinstbb,to_move,inst);
//             to_move.clear();
//             inst->replace_all_use_with(sub_inst);
//             return;
//         }
//         auto val = constnum->get_value();
//         if (val > 0 && (val & (val - 1)) == 0) {
//             int n = static_cast<int>(log2(val));
//             auto const_n = ConstantInt::get(n,m_);
//             auto shl_inst = IBinaryInst::create_shl(op1, const_n,putinstbb);
//             to_move.push_back(shl_inst);
//             move_instrs(inst->get_parent(),putinstbb,to_move,inst);
//             to_move.clear();
//             inst->replace_all_use_with(shl_inst);
//             return;
//         }
//     }
// }

void ComputeSimplify::mul_simplify(Instruction *inst, BasicBlock *putinstbb)
{
    auto op1 = inst->get_operand(0);
    auto op2 = inst->get_operand(1);

    auto simplify_with_const = [&](Value *val_operand, ConstantInt *constnum) -> bool
    {
        int c = constnum->get_value();
        auto zero = ConstantInt::get(0, m_);

        // 0*x
        if (c == 0)
        {
            inst->replace_all_use_with(constnum);
            return true;
        }
        // 1*x
        if (c == 1)
        {
            inst->replace_all_use_with(val_operand);
            return true;
        }
        // -1*x
        if (c == -1)
        {
            auto sub_inst = IBinaryInst::create_sub(zero, val_operand, putinstbb);
            to_move.push_back(sub_inst);
            move_instrs(inst->get_parent(), putinstbb, to_move, inst);
            to_move.clear();
            inst->replace_all_use_with(sub_inst);
            return true;
        }

        // 2^n
        if (c > 0 && (c & (c - 1)) == 0)
        {
            int n = static_cast<int>(log2(c));
            auto shl_inst = IBinaryInst::create_shl(val_operand, ConstantInt::get(n, m_), putinstbb);
            to_move.push_back(shl_inst);
            move_instrs(inst->get_parent(), putinstbb, to_move, inst);
            to_move.clear();
            inst->replace_all_use_with(shl_inst);
            return true;
        }

        // 两个 2 的幂的和：c = 2^a + 2^b
        if (__builtin_popcount(c) == 2)
        {
            int low_bit = c & -c;
            int b = static_cast<int>(log2(low_bit));      // 低位
            int a = static_cast<int>(log2(c ^ (1 << b))); // 高位

            Instruction *shl_a = IBinaryInst::create_shl(val_operand, ConstantInt::get(a, m_), putinstbb);
            to_move.push_back(shl_a);

            Instruction *shl_b = nullptr;
            if (b != 0)
            {
                shl_b = IBinaryInst::create_shl(val_operand, ConstantInt::get(b, m_), putinstbb);
                to_move.push_back(shl_b);
            }

            Instruction *add_inst = (b == 0) ? IBinaryInst::create_add(shl_a, val_operand, putinstbb)
                                             : IBinaryInst::create_add(shl_a, shl_b, putinstbb);

            to_move.push_back(add_inst);
            move_instrs(inst->get_parent(), putinstbb, to_move, inst);
            to_move.clear();
            inst->replace_all_use_with(add_inst);
            return true;
        }

        // 检查是否是差形式 2^a - 2^b
        if (c > 0)
        {
            int a = static_cast<int>(ceil(log2(c)));
            int pow2a = 1 << a;
            int rem = pow2a - c;
            if (rem > 0 && (rem & (rem - 1)) == 0)
            {
                int b = static_cast<int>(log2(rem));

                Instruction *shl_a = IBinaryInst::create_shl(val_operand, ConstantInt::get(a, m_), putinstbb);
                to_move.push_back(shl_a);

                Instruction *shl_b = nullptr;
                if (b != 0)
                {
                    shl_b = IBinaryInst::create_shl(val_operand, ConstantInt::get(b, m_), putinstbb);
                    to_move.push_back(shl_b);
                }
                Instruction *sub_inst = (b == 0) ? IBinaryInst::create_sub(shl_a, val_operand, putinstbb)
                                                 : IBinaryInst::create_sub(shl_a, shl_b, putinstbb);

                to_move.push_back(sub_inst);
                move_instrs(inst->get_parent(), putinstbb, to_move, inst);
                to_move.clear();
                inst->replace_all_use_with(sub_inst);
                return true;
            }
        }

        return false;
    };

    if (auto constnum = dynamic_cast<ConstantInt *>(op1))
    {
        if (simplify_with_const(op2, constnum))
            return;
    }
    if (auto constnum = dynamic_cast<ConstantInt *>(op2))
    {
        if (simplify_with_const(op1, constnum))
            return;
    }
}

void ComputeSimplify::srem_simplify(Instruction *inst, BasicBlock *putinstbb)
{
    auto op1 = inst->get_operand(0); // 被除数
    auto op2 = inst->get_operand(1); // 除数

    // x % ±1 => 0
    if (auto constnum = dynamic_cast<ConstantInt *>(op2))
    {
        if (constnum->get_value() == 1 || constnum->get_value() == -1)
        {
            auto zero = ConstantInt::get(0, m_);
            inst->replace_all_use_with(zero);
            return;
        }

        // x % 2^n
        auto val = constnum->get_value();
        if (val > 0 && (val & (val - 1)) == 0)
        {
            int n = static_cast<int>(log2(val));
            auto const_n = ConstantInt::get(n, m_);
            auto const_mask = ConstantInt::get(val - 1, m_);
            auto const_bias = ConstantInt::get((1 << n) - 1, m_);
            auto zero = ConstantInt::get(0, m_);

            auto getbb = inst->get_parent();
            auto frontbb = insert_before_block(getbb, m_);

            for (auto &getinst : getbb->get_instructions())
            {
                if (&getinst == inst)
                    break;
                to_move.push_back(&getinst);
            }
            move_instrs(frontbb, getbb, to_move, frontbb->get_terminator());
            to_move.clear();

            BasicBlock *trueblock = BasicBlock::create(m_, "", inst->get_parent()->get_parent());  // 非负数
            BasicBlock *falseblock = BasicBlock::create(m_, "", inst->get_parent()->get_parent()); // 负数

            frontbb->remove_instr(frontbb->get_terminator());
            frontbb->remove_succ_basic_block(getbb);
            getbb->remove_pre_basic_block(frontbb);

            auto builder = IRBuilder(nullptr, m_);
            builder.set_insert_point(frontbb);
            auto icmp = builder.create_icmp_ge(op1, zero);
            builder.create_cond_br(icmp, trueblock, falseblock);

            // trueblock（非负数）：直接位与
            builder.set_insert_point(trueblock);
            auto and_inst = IBinaryInst::create_sand(op1, const_mask, trueblock);
            builder.create_br(getbb);

            // falseblock（负数）：加偏置再算术右移再乘移位再减
            builder.set_insert_point(falseblock);
            auto add_bias = IBinaryInst::create_add(op1, const_bias, falseblock);
            auto shr_neg = IBinaryInst::create_ashr(add_bias, const_n, falseblock);
            auto mul_neg = IBinaryInst::create_shl(shr_neg, const_n, falseblock);
            auto rem_neg = IBinaryInst::create_sub(op1, mul_neg, falseblock);
            builder.create_br(getbb);

            // getbb：phi 合并
            auto phi = PhiInst::create_phi(inst->get_type(), getbb);
            phi->add_phi_pair_operand(and_inst, trueblock);
            phi->add_phi_pair_operand(rem_neg, falseblock);
            getbb->add_instr_begin(phi);

            inst->replace_all_use_with(phi);
            return;
        }
    }

    // 0 % x => 0
    if (auto constnum = dynamic_cast<ConstantInt *>(op1))
    {
        if (constnum->get_value() == 0)
        {
            inst->replace_all_use_with(op1);
            return;
        }
    }
}
