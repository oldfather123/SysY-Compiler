#include "riscv_generator.hpp"
#include "ir_type.hpp"
#include "ir_module.hpp"
#include "riscv_basic_block.hpp"
#include "riscv_function.hpp"
#include "register.hpp"
#include "reg_alloc.hpp"

void RiscvGenerator::optimize(RiscvFunction* rfunc) {
    for(auto rbb: rfunc->rbb_list) {
        for(auto it = rbb->rinst_list.begin(); it != rbb->rinst_list.end(); ) {
            auto rinst = *it;
            // cerr << rinst->print();
            it++; // 先保存下一个迭代器位置，因为 rbb->delete_rinst() 会使得 rbb->rinst_list 失效
            if(!rinst) {
                cerr << "[Error] [RiscvGenerator::optimize] Encountered a null RiscvInstruction in basic block " << rbb->name << endl;
                continue;
            }
            if(auto move_inst = dynamic_cast<RiscvMoveInst*>(rinst)) {
                if(move_inst->operands[0] == move_inst->operands[1] || move_inst->operands[0]->print() == move_inst->operands[1]->print()) {
                    rbb->delete_rinst(rinst); // 删除无效的移动指令
                }
            } else if(auto binary_inst = dynamic_cast<RiscvBinaryInst*>(rinst)) {
                auto next_inst = *it;
                if(next_inst) {
                    if(binary_inst->riid == RiscvInstID::ADDI && next_inst->riid == RiscvInstID::ADDI) {
                        if(binary_inst->ret_val == next_inst->get_op(0) || binary_inst->ret_val->print() == next_inst->get_op(0)->print()) {
                            auto imm1 = dynamic_cast<RiscvConst*>(binary_inst->get_op(1));
                            auto imm2 = dynamic_cast<RiscvConst*>(next_inst->get_op(1));
                            if(imm1 && imm2) {
                                int new_int_val = imm1->int_val + imm2->int_val;
                                // ADDI x1, x2, 3 + ADDI x3, x1, 4 = ADDI x3, x2, 7
                                // cerr << "[Info] Continuous ADDI:\n" << binary_inst->print() << next_inst->print() << "\n";
                                it++; // TODO: 连续优化
                                if(new_int_val == 0) { // 正好抵消，不需要进行操作
                                    rbb->delete_rinst(rinst);
                                    rbb->delete_rinst(next_inst);
                                } else if(new_int_val >= -2048 && new_int_val <= 2047) {
                                    rbb->add_rinst_before_rinst(new RiscvBinaryInst(RiscvInstID::ADDI, binary_inst->get_op(0), new RiscvConst(new_int_val), next_inst->ret_val, rbb), binary_inst);
                                    rbb->delete_rinst(rinst);
                                    rbb->delete_rinst(next_inst);
                                } else {
                                    rbb->add_rinst_before_rinst(new RiscvLiInst(get_reg_op_named("t6"), new_int_val, rbb), binary_inst);
                                    rbb->add_rinst_before_rinst(new RiscvBinaryInst(RiscvInstID::ADD, binary_inst->get_op(0), get_reg_op_named("t6"), next_inst->ret_val, rbb), binary_inst);
                                    rbb->delete_rinst(rinst);
                                    rbb->delete_rinst(next_inst);
                                }
                            }
                        }
                    }
                }
            } else if(auto load_inst = dynamic_cast<RiscvLoadInst*>(rinst)) {
                //*=================================  Load-Store Elimination  =================================
                // cerr << "[Info] Load 1: " << load_inst->print();
                auto next_inst = *it;
                if(next_inst) {
                    if(auto store_inst = dynamic_cast<RiscvStoreInst*>(next_inst)) {
                        // auto load_src = load_inst->get_op(1);
                        // auto store_dest = store_inst->get_op(1);
                        // cerr << "[Info] Store 1: " << store_inst->print();
                        while(true) {
                            auto next_load_it = next(it);
                            if(auto load_inst_2 = dynamic_cast<RiscvLoadInst*>(*next_load_it)) {
                                // cerr << "[Info] Load 2: " << load_inst_2->print();
                                auto next_store_it = next(next_load_it);
                                if(auto store_inst_2 = dynamic_cast<RiscvStoreInst*>(*next_store_it)) {
                                    // cerr << "[Info] Store 2: " << store_inst_2->print();
                                    bool is_same_load = load_inst->print() == load_inst_2->print();
                                    bool is_same_store = store_inst->print() == store_inst_2->print();
                                    if(is_same_load && is_same_store) {
                                        // cerr << "[Info] Abundant load-store\n";
                                        // rbb->delete_rinst(load_inst_2);
                                        // rbb->delete_rinst(store_inst_2);
                                        rbb->rinst_list.erase(next_load_it);
                                        rbb->rinst_list.erase(next_store_it);
                                    } else {
                                        break;
                                    }
                                } else {
                                    break;
                                }
                            } else {
                                break;
                            }
                        }
                    }
                }
                //*============================================================================================
            } else if(auto store_inst = dynamic_cast<RiscvStoreInst*>(rinst)) {
                auto store_src = store_inst->get_op(0);
                auto store_dest = store_inst->get_op(1);
                auto src_val = rfunc->reg_alloc->get_val_in_reg(store_src);
                auto dest_val = rfunc->reg_alloc->get_val_in_mem(store_dest);
                if(src_val && dest_val) {
                    cerr << "[Info] src: " << src_val->name << ", dest: " << dest_val->name << endl;
                    if(src_val== dest_val || src_val->name == dest_val->name) {
                        cerr << "[Info] src == dest\n";
                    }
                }
            }
        }
        for(auto it = rbb->rinst_list.begin(); it != rbb->rinst_list.end(); ) {
            auto rinst = *it;
            it++; // 先保存下一个迭代器位置，因为 rbb->delete_rinst() 会使得 rbb->rinst_list 失效
            if(auto store_inst = dynamic_cast<RiscvStoreInst*>(rinst)) {
                auto store_src = store_inst->get_op(0);
                auto store_dest = store_inst->get_op(1);
                // cerr << "[Info] Store src: " << store_src->print() << ", dest: " << store_dest->print() << endl;
                auto src_val = rfunc->reg_alloc->get_val_in_reg(store_src);
                auto dest_val = rfunc->reg_alloc->get_val_in_mem(store_dest);
                if(src_val && dest_val) {
                    cerr << "[Info] src: " << src_val->name << ", dest: " << dest_val->name << endl;
                    if(src_val== dest_val || src_val->name == dest_val->name) {
                        cerr << "[Info] src == dest\n";
                    }
                }
            }
        }
    }
}