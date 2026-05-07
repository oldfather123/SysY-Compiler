#include "Instclone.hpp"
#include "Instruction.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include <unordered_map>
#include "IRBuilder.hpp"
#include "Module.hpp"
#include <stack>

using entry_ret = std::tuple<BasicBlock *, BasicBlock *, Value *>;
entry_ret clone_func(Module *m, Function *origin_func, Function *target_func, std::unordered_map<Value *, Value *> &map)
{
    std::vector<BasicBlock *> retblock;
    std::vector<Value *> retvalue;
    auto return_type = origin_func->get_function_type()->get_return_type();
    for (auto &bb : origin_func->get_basic_blocks())
    {
        BasicBlock *newblock = BasicBlock::create(m, "", target_func);
        map[&bb] = newblock;
    }
    std::vector<Instruction *> moveret;
    for (auto &bb : origin_func->get_basic_blocks())
    {
        std::stack<Instruction *> phis;
        auto newblock = dynamic_cast<BasicBlock *>(map[&bb]);
        for (auto &inst : bb.get_instructions())
        {
            if (inst.get_instr_type() == Instruction::ret)
            {
                retblock.push_back(static_cast<BasicBlock *>(map[&bb]));
                if (return_type->is_void_type())
                {
                    continue;
                }
                auto return_value = inst.get_operand(0);
                retvalue.push_back(return_value);
                continue;
            }
            Instruction *cloneinst = clone_inst(&inst, newblock, map);
            map[&inst] = cloneinst;
            // phi要手动插一下,保持原来顺序，用stack
            if (inst.get_instr_type() == Instruction::phi)
            {
                phis.push(cloneinst);
            }
        }
        while (!phis.empty()) {
            auto phi=phis.top();
            newblock->add_instr_begin(phi);
            phis.pop();
        }
    }
    // 再遍历一遍指令，好像有遗漏
    for (auto &bb : origin_func->get_basic_blocks())
    {
        auto newblock = dynamic_cast<BasicBlock *>(map[&bb]);
        for (auto &inst : newblock->get_instructions())
        {
            for (int i = 0; i < inst.get_num_operand(); i++)
            {
                auto op = inst.get_operand(i);
                auto it = map.find(op);
                if (it != map.end())
                {
                    Value *result = it->second;
                    inst.set_operand(i, result);
                }
            }
        }
    }

    for (int i = 0; i < retvalue.size(); i++)
    {
        auto old = retvalue[i];
        auto it = map.find(old);
        auto v_new = (it != map.end()) ? it->second : old;
        retvalue[i] = v_new;
    }

    auto entry = dynamic_cast<BasicBlock *>(map[origin_func->get_entry_block()]);
    if (retblock.size() > 1)
    {
        BasicBlock *retbb = BasicBlock::create(m, "", target_func);
        auto builder = IRBuilder(nullptr, m);
        for (auto bb : retblock)
        {
            builder.set_insert_point(bb);
            builder.create_br(retbb);
        }
        if (retvalue.size() != 0)
        {
            auto phi_ret = PhiInst::create_phi(return_type, retbb);
            retbb->add_instr_begin(phi_ret);
            for (int i = 0; i < retblock.size(); i++)
            {
                phi_ret->add_phi_pair_operand(retvalue[i], retblock[i]);
            }
        //    std::cout << retbb->print() << std::endl;
            return std::make_tuple(entry, retbb, phi_ret);
        }

        return std::make_tuple(entry, retbb, nullptr);
    }
    auto singleret = retblock.front();
    if (return_type->is_void_type())
    {
        return std::make_tuple(entry, singleret, nullptr);
    }
    return std::make_tuple(entry, singleret, retvalue.front());
}

void replace_operand(Instruction *inst, unsigned i, Value *new_operand)
{
    assert(i < inst->get_operands().size() && "replace_operand: out of index");

    Value *old_operand = inst->get_operand(i);

    if (old_operand)
    {
        old_operand->remove_use(inst, i);
    }

    if (new_operand)
    {
        new_operand->add_use(inst, i);
    }

    // 5. 更新指令中的操作数
    inst->set_operand(i, new_operand);
}
Instruction *clone_inst(Instruction *inst, BasicBlock *bb, std::unordered_map<Value *, Value *> map)
{
    std::vector<Value *> operands;
    for (int i = 0; i < inst->get_num_operand(); i++)
    {
        auto operand = inst->get_operand(i);
        auto it = map.find(operand);
        if (it != map.end())
        {
            Value *result = it->second;
            operands.push_back(result);
            // result->add_use(inst,0);
            // inst->add_use(static_cast<User *>(result),i);
        }
        else
        {
            operands.push_back(operand);
        }
    }

    switch (inst->get_instr_type())
    {
    case Instruction::add:
        return IBinaryInst::create_add(operands[0], operands[1], bb);
    case Instruction::sub:
        return IBinaryInst::create_sub(operands[0], operands[1], bb);
    case Instruction::mul:
        return IBinaryInst::create_mul(operands[0], operands[1], bb);
    case Instruction::sdiv:
        return IBinaryInst::create_sdiv(operands[0], operands[1], bb);
    case Instruction::srem:
        return IBinaryInst::create_srem(operands[0], operands[1], bb);
    case Instruction::iand:
        return IBinaryInst::create_and(operands[0], operands[1], bb);
    case Instruction::sand:
        return IBinaryInst::create_sand(operands[0], operands[1], bb);
    case Instruction::shl:
        return IBinaryInst::create_shl(operands[0], operands[1], bb);
    case Instruction::ashr:
        return IBinaryInst::create_ashr(operands[0], operands[1], bb);

    case Instruction::fadd:
        return FBinaryInst::create_fadd(operands[0], operands[1], bb);
    case Instruction::fsub:
        return FBinaryInst::create_fsub(operands[0], operands[1], bb);
    case Instruction::fmul:
        return FBinaryInst::create_fmul(operands[0], operands[1], bb);
    case Instruction::fdiv:
        return FBinaryInst::create_fdiv(operands[0], operands[1], bb);

    case Instruction::eq:
        return ICmpInst::create_eq(operands[0], operands[1], bb);
    case Instruction::ne:
        return ICmpInst::create_ne(operands[0], operands[1], bb);
    case Instruction::lt:
        return ICmpInst::create_lt(operands[0], operands[1], bb);
    case Instruction::le:
        return ICmpInst::create_le(operands[0], operands[1], bb);
    case Instruction::gt:
        return ICmpInst::create_gt(operands[0], operands[1], bb);
    case Instruction::ge:
        return ICmpInst::create_ge(operands[0], operands[1], bb);

    
    case Instruction::iiand:
        return AndInst::create_iiand(operands[0], operands[1], bb);

    case Instruction::feq:
        return FCmpInst::create_feq(operands[0], operands[1], bb);
    case Instruction::fne:
        return FCmpInst::create_fne(operands[0], operands[1], bb);
    case Instruction::flt:
        return FCmpInst::create_flt(operands[0], operands[1], bb);
    case Instruction::fle:
        return FCmpInst::create_fle(operands[0], operands[1], bb);
    case Instruction::fgt:
        return FCmpInst::create_fgt(operands[0], operands[1], bb);
    case Instruction::fge:
        return FCmpInst::create_fge(operands[0], operands[1], bb);

    case Instruction::call:
    {
        auto *func = dynamic_cast<Function *>(operands[0]);
        std::vector<Value *> args(operands.begin() + 1, operands.end()); // 从操作数中提取参数
        return CallInst::create_call(func, args, bb);
    }

    case Instruction::br:
        if (operands.size() == 3)
        {
            return BranchInst::create_cond_br(operands[0],
                                              static_cast<BasicBlock *>(operands[1]),
                                              static_cast<BasicBlock *>(operands[2]),
                                              bb);
        }
        else
        {
            return BranchInst::create_br(static_cast<BasicBlock *>(operands[0]), bb);
        }

    case Instruction::ret:
    {
        auto *ret_inst = static_cast<ReturnInst *>(inst);
        if (ret_inst->is_void_ret())
        {
            return ReturnInst::create_void_ret(bb);
        }
        else
        {
            return ReturnInst::create_ret(operands[0], bb);
        }
    }

    case Instruction::store:
        return StoreInst::create_store(operands[0], operands[1], bb);

    case Instruction::load:
        return LoadInst::create_load(operands[0], bb);

    case Instruction::alloca:
    {
        auto *ty = static_cast<PointerType *>(inst->get_type())->get_pointer_element_type();
        return AllocaInst::create_alloca(ty, bb);
    }

    case Instruction::zext:
        return ZextInst::create_zext(operands[0], inst->get_type(), bb);

    case Instruction::fptosi:
        return FpToSiInst::create_fptosi(operands[0], inst->get_type(), bb);

    case Instruction::sitofp:
        return SiToFpInst::create_sitofp(operands[0], bb);

    case Instruction::getelementptr:
    {
        std::vector<Value *> idxs(operands.begin() + 1, operands.end()); // 从操作数中提取索引
        return GetElementPtrInst::create_gep(operands[0], idxs, bb);
    }

    case Instruction::phi:
    {
        std::vector<Value *> vals;
        std::vector<BasicBlock *> val_bbs;
        for (unsigned i = 0; i < operands.size(); i += 2)
        {
            if (static_cast<UndefValue *>(inst->get_operand(i)))
            {
                vals.push_back(operands[i]);
            }
            else
            {
                vals.push_back(operands[i]);
            }
            val_bbs.push_back(static_cast<BasicBlock *>(operands[i + 1]));
        }
        return PhiInst::create_phi(inst->get_type(), bb, vals, val_bbs);
    }

    case Instruction::bitcast:
        return BitCastInst::create_bitcast(operands[0], inst->get_type(), bb);

    default:
        throw std::runtime_error("Unsupported instruction type in clone_inst()");
    }
}

void split_basic_block_into_three(BasicBlock *orig_bb, Module *m)
{
    auto func = orig_bb->get_parent();
    auto builder = IRBuilder(nullptr, m);

    // 创建三个新的基本块
    auto new_bb1 = BasicBlock::create(m, orig_bb->get_name() + "_split1", func);
    auto new_bb2 = BasicBlock::create(m, orig_bb->get_name() + "_split2", func);

    // 1. new_bb1
    for (auto pre_bb : orig_bb->get_pre_basic_blocks())
    {
        auto term = pre_bb->get_terminator();
        for (int i = 0; i < term->get_num_operand(); ++i)
        {
            if (term->get_operand(i) == orig_bb)
            {
                term->set_operand(i, new_bb1);
            }
        }
        pre_bb->remove_succ_basic_block(orig_bb);
        pre_bb->add_succ_basic_block(new_bb1);
        new_bb1->add_pre_basic_block(pre_bb);
    }

    // 2. new_bb1 -> new_bb2
    builder.set_insert_point(new_bb1);
    builder.create_br(new_bb2);
    new_bb1->add_succ_basic_block(new_bb2);

    // 4. new_bb2 -> org
    builder.set_insert_point(new_bb2);
    builder.create_br(orig_bb);
    new_bb2->add_succ_basic_block(orig_bb);
}

BasicBlock *insert_before_block(BasicBlock *orig_bb, Module *m)
{
    auto func = orig_bb->get_parent();
    auto builder = IRBuilder(nullptr, m);
    auto new_bb1 = BasicBlock::create(m, "", func);
    std::vector<BasicBlock *> to_move_pre;
    for (auto pre_bb : orig_bb->get_pre_basic_blocks())
    {
        auto term = pre_bb->get_terminator();
        for (int i = 0; i < term->get_num_operand(); ++i)
        {
            if (term->get_operand(i) == orig_bb)
            {
                term->set_operand(i, new_bb1);
            }
        }
        to_move_pre.push_back(pre_bb);
        pre_bb->remove_succ_basic_block(orig_bb);
        pre_bb->add_succ_basic_block(new_bb1);
        new_bb1->add_pre_basic_block(pre_bb);
    }
    for (auto pre : to_move_pre)
    {
        orig_bb->remove_pre_basic_block(pre);
    }

    builder.set_insert_point(new_bb1);
    builder.create_br(orig_bb);
    // new_bb1->add_succ_basic_block(orig_bb);
    if (orig_bb->get_parent()->get_entry_block() == orig_bb)
    {
        std::vector<BasicBlock *> to_move;
        to_move.push_back(new_bb1);
        auto &blocks = orig_bb->get_parent()->get_basic_blocks();
        blocks.remove(new_bb1);
        auto termIt = blocks.begin();
        for (; termIt != blocks.end(); ++termIt)
        {
            if (&*termIt == orig_bb)
            {
                break;
            }
        }
        for (auto *inst : to_move)
        {
            blocks.insert_before(termIt, inst);
        }
    }
    return new_bb1;
}
/**
 *   std::vector<Instruction *> to_move;
 *   for (auto &inst : newblock->get_instructions()) {
 *       if (inst.isTerminator()) break;
 *       to_move.push_back(&inst);
 *   }
 *   move_instrs(target_bb, newblock, to_move, target_bb->get_terminator());
 *   to_move.clear();
 */
void move_instrs(BasicBlock *insert_block, BasicBlock *inst_block,
                 std::vector<Instruction *> to_move, Instruction *insert_before_inst)
{
    auto &inst_list = insert_block->get_instructions();
    auto termIt = inst_list.begin();
    for (; termIt != inst_list.end(); ++termIt)
    {
        if (&*termIt == insert_before_inst)
            break;
    }

    for (auto *inst : to_move)
    {
        inst_block->get_instructions().remove(inst);
        inst->set_parent(insert_block);
        inst_list.insert_before(termIt, inst);
    }
}