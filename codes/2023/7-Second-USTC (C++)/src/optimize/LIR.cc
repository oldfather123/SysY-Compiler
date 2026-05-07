#include "LIR.hh"
#include "DeadCodeEli.hh"
#include "RedundantInstsEli.hh"

void LIR::execute() {

    // std::cout << m_->print() << std::endl;
    
    for(auto func: m_->get_functions()) {
        if(func->is_declaration()) 
            continue;
        for(auto bb: func->get_basic_blocks()) {
            split_gep(bb);
        }  
    }

    // std::cout << m_->print() << std::endl;

    for(auto func: m_->get_functions()) {
        if(func->is_declaration()) 
            continue;
        for(auto bb: func->get_basic_blocks()) {
            cast_cmp(bb);
        }  
    }

    // std::cout << m_->print() << std::endl;

    for(auto func: m_->get_functions()) {
        if(func->is_declaration()) 
            continue;
        for(auto bb: func->get_basic_blocks()) {
            load_offset(bb);
            store_offset(bb);
        }
    }

    // std::cout << m_->print() << std::endl;

    for(auto func: m_->get_functions()) {
        for(auto bb: func->get_basic_blocks()) {
            merge_cmp_br(bb);
            merge_fcmp_br(bb);
        } 
    }

    // std::cout << m_->print() << std::endl;

    auto dce = DeadCodeEli(m_);
    dce.execute();

}

void LIR::split_gep(BasicBlock* bb) {
    auto &instructions = bb->get_instructions();
    for(auto iter = instructions.begin(); iter != instructions.end(); iter++) {
        auto inst_gep = *iter;
        if(inst_gep->is_gep()) {
            int offset_op_num;
            if(inst_gep->get_num_operands() == 2) {
                offset_op_num = 1;
            } else if(inst_gep->get_num_operands() == 3) {
                offset_op_num = 2;
            } else {
                LOG(ERROR) << "gep指令格式出现异常";
            }
            auto size = ConstantInt::get(inst_gep->get_type()->get_pointer_element_type()->get_size(), m_);
            auto offset = inst_gep->get_operand(offset_op_num);
            inst_gep->remove_operands(offset_op_num, offset_op_num);
            inst_gep->add_operand(ConstantInt::get(0, m_));
            auto real_offset = BinaryInst::create_mul(offset, size, bb, m_);
            bb->add_instruction(++iter, instructions.back());
            instructions.pop_back();
            auto real_ptr = BinaryInst::create_add(inst_gep, real_offset, bb, m_);
            bb->add_instruction(iter--, instructions.back());
            instructions.pop_back();
            inst_gep->remove_use(real_ptr);
            inst_gep->replace_all_use_with(real_ptr);
            inst_gep->get_use_list().clear();
            inst_gep->add_use(real_ptr);
        }
    }
}

void LIR::load_offset(BasicBlock *bb) {
    auto &instructions = bb->get_instructions();
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++){
        auto instr = *iter;
        if (instr->is_load()) {
            auto load_instr = dynamic_cast<LoadInst*>(instr);
            auto ptr = load_instr->get_lval();
            auto gep_ptr = dynamic_cast<GetElementPtrInst*>(ptr);
            auto inst_ptr = dynamic_cast<Instruction*>(ptr);
            Value *base;
            Value *offset;
            if (gep_ptr) {
                base = gep_ptr;
                if (gep_ptr->get_num_operands() == 2) {
                    offset = gep_ptr->get_operand(1);
                } else if (gep_ptr->get_num_operands() == 3) {
                    offset = gep_ptr->get_operand(2);
                } else {
                    LOG(ERROR) << "gep指令格式出现异常";
                }
            } else if (inst_ptr) {
                if (inst_ptr->is_add()) {
                    base = inst_ptr->get_operand(0);
                    auto offset_instr = dynamic_cast<Instruction*>(inst_ptr->get_operand(1));
                    if (offset_instr->is_mul()) {
                        offset = offset_instr->get_operand(0);
                    } else if (offset_instr->is_lsl()) {
                        offset = offset_instr->get_operand(0);
                    }
                }
            }
            if (gep_ptr || inst_ptr) {
                auto load_offset_instr = LoadOffsetInst::create_loadoffset(base->get_type()->get_pointer_element_type(), base, offset, bb);
                instructions.pop_back();
                bb->add_instruction(iter, load_offset_instr);
                iter--;
                instr->replace_all_use_with(load_offset_instr);
                bb->delete_instr(instr);
            }
        }
    }
}

void LIR::store_offset(BasicBlock *bb) {
    auto &instructions = bb->get_instructions();
    for (auto iter = instructions.begin(); iter != instructions.end(); iter++){
        auto instr = *iter;
        if (instr->is_store()) {
            auto store_instr = dynamic_cast<StoreInst*>(instr);
            auto ptr = store_instr->get_lval();
            auto gep_ptr = dynamic_cast<GetElementPtrInst*>(ptr);
            auto inst_ptr = dynamic_cast<Instruction*>(ptr);
            Value *base;
            Value *offset;
            if (gep_ptr) {
                base = gep_ptr;
                if (gep_ptr->get_num_operands() == 2) {
                    offset = gep_ptr->get_operand(1);
                } else if (gep_ptr->get_num_operands() == 3) {
                    offset = gep_ptr->get_operand(2);
                } else {
                    LOG(ERROR) << "gep指令格式出现异常";
                }
            } else if (inst_ptr) {
                if (inst_ptr->is_add()) {
                    base = inst_ptr->get_operand(0);
                    auto offset_instr = dynamic_cast<Instruction*>(inst_ptr->get_operand(1));
                    if (offset_instr->is_mul()) {
                        offset = offset_instr->get_operand(0);
                    } else if (offset_instr->is_lsl()) {
                        offset = offset_instr->get_operand(0);
                    }
                } 
            }
            if (gep_ptr || inst_ptr) {
                auto store_offset_instr = StoreOffsetInst::create_storeoffset(store_instr->get_rval(), base, offset, bb);
                instructions.pop_back();
                bb->add_instruction(iter, store_offset_instr);
                iter--;
                instr->replace_all_use_with(store_offset_instr);
                bb->delete_instr(instr);
            }
        }
    }
}

void LIR::cast_cmp(BasicBlock *bb) {
    auto &instructions = bb->get_instructions();
    for(auto iter = instructions.begin(); iter != instructions.end(); iter++) {
        auto instr = *iter;
        if(instr->is_cmp()) {
            auto inst_cmp = dynamic_cast<CmpInst*>(instr);
            CmpOp cmpop = inst_cmp->get_cmp_op();
            switch(cmpop) {
                case CmpOp::LE: {
                        auto new_inst = CmpInst::create_cmp(CmpOp::GE, inst_cmp->get_operand(1), inst_cmp->get_operand(0), bb, m_);
                        instructions.pop_back();
                        bb->add_instruction(iter, new_inst);
                        iter--;
                        instr->replace_all_use_with(new_inst);
                        bb->delete_instr(instr);
                    }
                    break;
                case CmpOp::GT: {
                        auto new_inst = CmpInst::create_cmp(CmpOp::LT, inst_cmp->get_operand(1), inst_cmp->get_operand(0), bb, m_);
                        instructions.pop_back();
                        bb->add_instruction(iter, new_inst);
                        iter--;
                        instr->replace_all_use_with(new_inst);
                        bb->delete_instr(instr);
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void LIR::merge_cmp_br(BasicBlock* bb) { 
    auto terminator = bb->get_terminator();
    if (terminator->is_br()){
        auto br = dynamic_cast<BranchInst *>(terminator);
        if (br->is_cond_br()){
            auto inst = dynamic_cast<Instruction *>(br->get_operand(0));
            if (inst && inst->is_cmp()) {
                auto br_operands = br->get_operands();
                auto inst_cmp = dynamic_cast<CmpInst *>(inst);
                auto cmp_ops = inst_cmp->get_operands();
                auto cmp_op = inst_cmp->get_cmp_op();
                auto true_bb = dynamic_cast<BasicBlock* >(br_operands[1]);
                auto false_bb = dynamic_cast<BasicBlock* >(br_operands[2]);
                true_bb->remove_pre_basic_block(bb);
                false_bb->remove_pre_basic_block(bb);
                bb->remove_succ_basic_block(true_bb);
                bb->remove_succ_basic_block(false_bb);
                if((*(true_bb->get_instructions().begin()))->is_phi() && (*(false_bb->get_instructions().begin()))->is_phi())
                    LOG(ERROR) << "无法处理这种情况";
                if((*(true_bb->get_instructions().begin()))->is_phi()) {
                    switch(cmp_op) {
                        case CmpOp::EQ: {
                                auto cmp_br = CmpBrInst::create_cmpbr(CmpOp::NE,cmp_ops[0],cmp_ops[1],
                                                        false_bb, true_bb,
                                                        bb,m_);
                            }
                            break;
                        case CmpOp::GE: {
                                auto cmp_br = CmpBrInst::create_cmpbr(CmpOp::LT,cmp_ops[0],cmp_ops[1],
                                                        false_bb, true_bb,
                                                        bb,m_);
                            }
                            break;
                        case CmpOp::GT: {
                                auto cmp_br = CmpBrInst::create_cmpbr(CmpOp::GE,cmp_ops[1],cmp_ops[0],
                                                        false_bb, true_bb,
                                                        bb,m_);
                            }
                            break;
                        case CmpOp::LE: {
                                auto cmp_br = CmpBrInst::create_cmpbr(CmpOp::LT,cmp_ops[1],cmp_ops[0],
                                                        false_bb, true_bb,
                                                        bb,m_);
                            }
                            break;
                        case CmpOp::LT: {
                                auto cmp_br = CmpBrInst::create_cmpbr(CmpOp::GE,cmp_ops[0],cmp_ops[1],
                                                        false_bb, true_bb,
                                                        bb,m_);
                            }
                            break;
                        case CmpOp::NE: {
                                auto cmp_br = CmpBrInst::create_cmpbr(CmpOp::EQ,cmp_ops[0],cmp_ops[1],
                                                        false_bb, true_bb,
                                                        bb,m_);
                            }
                            break;
                        default:
                            break;
                    }
                } else {
                    auto cmp_br = CmpBrInst::create_cmpbr(cmp_op,cmp_ops[0],cmp_ops[1],
                                                        true_bb,false_bb,
                                                        bb,m_);
                } 
                bb->delete_instr(inst);
                bb->delete_instr(br);
            }
        }
    }
}

void LIR::merge_fcmp_br(BasicBlock* bb) { 
    auto terminator = bb->get_terminator();
    if (terminator->is_br()){
        auto br = dynamic_cast<BranchInst *>(terminator);
        if (br->is_cond_br()){
            auto inst = dynamic_cast<Instruction *>(br->get_operand(0));

            if (inst && inst->is_fcmp()) {
                auto br_operands = br->get_operands();
                auto inst_fcmp = dynamic_cast<FCmpInst *>(inst);
                auto cmp_ops = inst_fcmp->get_operands();
                auto cmp_op = inst_fcmp->get_cmp_op();
                auto true_bb = dynamic_cast<BasicBlock* >(br_operands[1]);
                auto false_bb = dynamic_cast<BasicBlock* >(br_operands[2]);
                true_bb->remove_pre_basic_block(bb);
                false_bb->remove_pre_basic_block(bb);
                bb->remove_succ_basic_block(true_bb);
                bb->remove_succ_basic_block(false_bb);
                if((*(true_bb->get_instructions().begin()))->is_phi() && (*(false_bb->get_instructions().begin()))->is_phi())
                    LOG(ERROR) << "无法处理这种情况";
                if((*(true_bb->get_instructions().begin()))->is_phi()) {
                    switch(cmp_op) {
                        case CmpOp::EQ: {
                                auto fcmp_br = FCmpBrInst::create_fcmpbr(CmpOp::NE,cmp_ops[0],cmp_ops[1],
                                                    false_bb, true_bb,
                                                    bb,m_);
                            }
                            break;
                        case CmpOp::GE: {
                                auto fcmp_br = FCmpBrInst::create_fcmpbr(CmpOp::LT,cmp_ops[0],cmp_ops[1],
                                                    false_bb, true_bb,
                                                    bb,m_);
                            }
                            break;
                        case CmpOp::GT: {
                                auto fcmp_br = FCmpBrInst::create_fcmpbr(CmpOp::LE,cmp_ops[0],cmp_ops[1],
                                                    false_bb, true_bb,
                                                    bb,m_);
                            }
                            break;
                        case CmpOp::LE: {
                                auto fcmp_br = FCmpBrInst::create_fcmpbr(CmpOp::GT,cmp_ops[0],cmp_ops[1],
                                                    false_bb, true_bb,
                                                    bb,m_);
                            }
                            break;
                        case CmpOp::LT: {
                                auto fcmp_br = FCmpBrInst::create_fcmpbr(CmpOp::GE,cmp_ops[0],cmp_ops[1],
                                                    false_bb, true_bb,
                                                    bb,m_);
                            }
                            break;
                        case CmpOp::NE: {
                                auto fcmp_br = FCmpBrInst::create_fcmpbr(CmpOp::EQ,cmp_ops[0],cmp_ops[1],
                                                    false_bb, true_bb,
                                                    bb,m_);
                            }
                            break;
                        default:
                            break;
                    }
                } else {
                    auto fcmp_br = FCmpBrInst::create_fcmpbr(cmp_op,cmp_ops[0],cmp_ops[1],
                                                    true_bb, false_bb,
                                                    bb,m_);
                } 
                bb->delete_instr(inst);
                bb->delete_instr(br);
            }
        }
    }
}