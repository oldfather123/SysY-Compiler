#include "middleend/loop_unroll.hpp"
#include <iostream>
#include <unordered_set>
#include <unordered_map>

namespace middleend {

static const int UNROLL_CNT = 5;
static int map_curid;
static std::unordered_map<Temp*, Temp*> br_map, br_map2, br_map3;
static std::unordered_map<Temp*, Temp*> reg_map[3]; // contains only regs inside loop, two maps representing now and prev, 2 used for temp storage
static std::unordered_map<ir::BasicBlock*, ir::BasicBlock*> bb_map[2]; // contains only bbs inside loop, two maps representing now and prev

static bool is_cmp(ir::instruction::Binary *binary) {
    switch (binary->get_type()) {
        case BinaryOp::Eq:
        case BinaryOp::Geq:
        case BinaryOp::Gt:
        case BinaryOp::Leq:
        case BinaryOp::Lt:
        case BinaryOp::Neq:
            return true;
        default:
            return false;
    }
}

SimpleLoopInfo LoopUnroll::get_loop_info(Loop *loop, const std::unordered_set<ir::BasicBlock *> &loop_bbs, ir::BasicBlock *exit) {
    // std::cout << "get loop info\n";
    SimpleLoopInfo info(0, 0);
    info.header = loop->header_;
    for (auto bb : loop_bbs) {
        for (auto &insn : *bb->get_instructions()) {
            if (auto call = dynamic_cast<ir::instruction::Call*>(insn)) {
                info.inst_cnt += 50;
            }
            else 
                info.inst_cnt ++;
        }
    }
    ir::BasicBlock *entry = loop->header_;
    ir::BasicBlock *exit_prev = nullptr;
    auto in_loop = [=](BasicBlock *b) {
        Loop *l = la_->get_loop(b);
        while (l != nullptr && l != loop) {
            l = l->parent_;
        }
        return l == loop;
    };
    // std::cout << "【exit】" << exit->get_name() << "\n";
    for (auto bb : cfg_->prev(exit)) {
        if (in_loop(bb)) {
            if (exit_prev == nullptr) exit_prev = bb;
            else return info; // multiple exit_prev
        }
    }
    info.exit_prev = exit_prev;
    ir::BasicBlock *into_entry = nullptr;
    for (auto bb : cfg_->prev(entry)) {
        if (in_loop(bb)) {
            if (bb != exit_prev) return info; // multiple back edge
        } else {
            if (into_entry == nullptr) into_entry = bb;
            else return info; // multiple into edge
        }
    }
    info.into_entry = into_entry;
    int type = 2;
    ir::Instruction* inst_cond = (*exit_prev->get_instructions()).back();
    if (auto br_cond = dynamic_cast<ir::instruction::CondBranch*>(inst_cond)) {
        info.brch = br_cond;
        // std::cout << (*uda_->get_defset(br_cond->getcond()).begin())->to_str() << "\n";
        if (auto binary_cond = dynamic_cast<ir::instruction::Binary*>(*uda_->get_defset(br_cond->getcond()).begin())) {
            info.cmp = binary_cond;
            info.cmp_reg = binary_cond->getrhs();
            info.upd_reg = binary_cond->getlhs();
            if (!is_cmp(binary_cond)) return info;
            if (binary_cond->getdst()->get_type().base_type != ScalarType::Int) return info;
            if (binary_cond->get_type() != BinaryOp::Lt && binary_cond->get_type() != BinaryOp::Leq) return info; // TODO: only consider i < n or i <= n now
            if (binary_cond->get_parent()->get_parent()->is_param(binary_cond->getrhs())) info.end_reg = binary_cond->getrhs();
            else if(auto end_imm = dynamic_cast<ir::instruction::LoadImm4 *>(*uda_->get_defset(binary_cond->getrhs()).begin())) {
                info.end = end_imm->getimm().iv;
                info.end_reg = binary_cond->getrhs();
                // std::cout << "【end_imm】"; end_imm->print(std::cout);
                type = 1;
            } else { 
                info.end_reg = binary_cond->getrhs();
                // std::cout << "【end reg】" << info.end_reg->to_str() << "\n";
                if (binary_cond->get_parent()->get_parent()->is_param(binary_cond->getrhs())) return info;
            }
            info.cond_op = binary_cond->get_type();
            info.cond_reg = binary_cond->getdst();
            if (binary_cond->get_parent()->get_parent()->is_param(binary_cond->getlhs())) return info;
            auto inst = (*uda_->get_defset(binary_cond->getlhs()).begin());
            // if (auto binary_update = dynamic_cast<ir::instruction::BinaryImm*>(inst)) {
            //     // std::cout << "【binary_update】" << binary_update->to_str() << "\n";
            //     info.step = binary_update->getimm();
            //     if (binary_update->get_type() != BinaryOp::Add) return info; // TODO: only consider i + c now
            //     Temp *reg_i = binary_update->getlhs();
            //     if (binary_update->get_parent()->get_parent()->is_param(reg_i)) return info;
            //     if (auto inst_phi = dynamic_cast<ir::instruction::Phi*>(*uda_->get_defset(reg_i).begin())) {
            //         // std::cout << "【inst_phi】" << inst_phi->to_str() << "\n";
            //         if (inst_phi->get_parent() != entry) return info;
            //         if (inst_phi->getvalues()->size() != 2) return info;
            //         Temp * reg_i_init;
            //         for (auto val: *inst_phi->getvalues()) {
            //             if (val != binary_update->getdst()) {
            //                 reg_i_init = val;
            //             }
            //         }
            //         // std::cout << reg_i_init->to_str() << "\n";
            //         if (binary_update->get_parent()->get_parent()->is_param(reg_i_init)) return info;
            //         if (auto init_imm = dynamic_cast<ir::instruction::LoadImm4*>(*uda_->get_defset(reg_i_init).begin())) {
            //             // std::cout << "【init_imm】" << init_imm->to_str() << "\n";
            //             info.start = init_imm->getimm().iv;
            //         } else return info; // i init is not imm
            //     } else return info; // no phi inst
            //     info.indvar_reg = binary_update->getdst();
            // } else 
            if (auto binary_update = dynamic_cast<ir::instruction::Binary*>(inst)) {
                // std::cout << "【binary_update】" << binary_update->to_str() << "\n";
                info.upd = binary_update;
                if (binary_update->getdst()->get_type().base_type != ScalarType::Int) return info;
                if (binary_update->get_type() != BinaryOp::Add) return info; // TODO: only consider i + c now
                Temp *reg_i = binary_update->getlhs(), *reg_c = binary_update->getrhs();
                if (binary_update->get_parent()->get_parent()->is_param(reg_i) || binary_update->get_parent()->get_parent()->is_param(reg_c)) return info;
                if (auto dummy = dynamic_cast<ir::instruction::LoadImm4*>(*uda_->get_defset(reg_i).begin())) {
                    std::swap(reg_i, reg_c);
                }
                if (auto update_imm = dynamic_cast<ir::instruction::LoadImm4*>(*uda_->get_defset(reg_c).begin())) {
                    if (!update_imm->getimm().iv == 1) return info; // TODO: only consider c = 1 now
                    info.step = update_imm->getimm().iv;
                } else return info;
                if (auto inst_phi = dynamic_cast<ir::instruction::Phi*>(*uda_->get_defset(reg_i).begin())) {
                    if (inst_phi->get_parent() != entry) { std::cout << "parent not entry\n"; return info; }
                    if (inst_phi->getvalues()->size() != 2) return info;
                    Temp * reg_i_init;
                    for (auto val: *inst_phi->getvalues()) {
                        if (val != binary_update->getdst()) {
                            reg_i_init = val;
                        }
                    }
                    // std::cout << "【inst_phi】"; inst_phi->print(std::cout);
                    if (binary_update->get_parent()->get_parent()->is_param(reg_i_init)) return info;
                    if (auto init_imm = dynamic_cast<ir::instruction::LoadImm4*>(*uda_->get_defset(reg_i_init).begin())) {
                        info.start_imm = init_imm;
                        info.imm_reg = init_imm->getdst();
                        info.start = init_imm->getimm().iv;
                        // std::cout << "【init_imm】"; init_imm->print(std::cout);
                    } else return info; // i init is not imm
                } else return info; // no phi inst
                info.indvar_reg = binary_update->getdst();
            } else return info; // update not binary inst
        } else return info;
    } else assert(false);
    if (type == 2) return info;
    info.loop_type = type;
    info.valid = true;
    return info;
}

int last_turn_temp_used = 0;

ir::Instruction* LoopUnroll::copy_inst(Loop* loop, SimpleLoopInfo& info, ir::Instruction *inst, ir::BasicBlock *entry, ir::BasicBlock *exit, bool is_last, ir::BasicBlock* for_br, bool is_br) {
    auto reg_map_if = [=](Temp* reg) {
        if (is_br) {
            if (br_map3.count(reg)) return br_map3[reg];
            if (last_turn_temp_used == 0) 
                last_turn_temp_used = func->get_temp_used();
            auto res = new Temp(last_turn_temp_used++, Int);
            br_map3[reg] = res;
            return res;
        }
        if (reg_map[map_curid].count(reg)) return reg_map[map_curid].at(reg);
        else return reg;
    };
    auto bb_map_to = [=](ir::BasicBlock *bb) {
        if (bb == exit) return exit;
        if (bb == entry && info.loop_type != 3) return entry;
        return bb_map[map_curid].at(bb);
    };
    if (auto binary = dynamic_cast<ir::instruction::Binary *>(inst)) {
        auto new_inst = new ir::instruction::Binary(binary->get_type(), reg_map_if(binary->getdst()), reg_map_if(binary->getlhs()), reg_map_if(binary->getrhs()));
        return new_inst;
    } else if (auto binaryimm = dynamic_cast<ir::instruction::BinaryImm *>(inst)) {
        auto new_inst = new ir::instruction::BinaryImm(binaryimm->get_type(), reg_map_if(binaryimm->getdst()), reg_map_if(binaryimm->getlhs()), binaryimm->getconst());
        return new_inst;
    } else if (auto unary = dynamic_cast<ir::instruction::Unary *>(inst)) {
        auto new_inst = new ir::instruction::Unary(unary->get_type(), reg_map_if(unary->getdst()), reg_map_if(unary->getsrc()));
        return new_inst;
    } else if (auto convert = dynamic_cast<ir::instruction::Assign *>(inst)) {
        auto new_inst = new ir::instruction::Assign(reg_map_if(convert->getdst()), reg_map_if(convert->getsrc()));
        return new_inst;
    } else if (auto loadimm = dynamic_cast<ir::instruction::LoadImm4 *>(inst)) {
        auto new_inst = new ir::instruction::LoadImm4(reg_map_if(loadimm->getdst()), loadimm->getimm());
        return new_inst;
    } else if (auto loadaddr = dynamic_cast<ir::instruction::LoadAddr *>(inst)) {
        auto new_inst = new ir::instruction::LoadAddr(reg_map_if(loadaddr->get_addr()), loadaddr->get_name(), loadaddr->type_);
        return new_inst;
    } else if (auto load = dynamic_cast<ir::instruction::Load *>(inst)) {
        auto new_inst = new ir::instruction::Load(reg_map_if(load->getdst()), reg_map_if(load->getaddr()), load->not_delete(), load->getoffset());
        return new_inst;
    } else if (auto call = dynamic_cast<ir::instruction::Call *>(inst)) {
        auto args = call->get_srcs();
        for(auto &arg : args){
            arg = reg_map_if(arg);
        }
        auto new_inst = new ir::instruction::Call(reg_map_if(call->getdst()), args, call->getfunc());
        return new_inst;
    } else if (auto store = dynamic_cast<ir::instruction::Store *>(inst)) {
        auto new_inst = new ir::instruction::Store(reg_map_if(store->getaddr()), reg_map_if(store->getsrc()), store->not_delete(), store->getoffset());
        return new_inst;
    } else if (auto br = dynamic_cast<ir::instruction::CondBranch *>(inst)) {
        auto new_inst = new ir::instruction::CondBranch(br->get_type(), bb_map_to(br->get_true_bb()), bb_map_to(br->get_false_bb()), reg_map_if(br->getcond()));
        return new_inst;
    } else if (auto br = dynamic_cast<ir::instruction::Branch *>(inst)) {
        auto new_inst = new ir::instruction::Branch(bb_map_to(br->get_bb()));
        return new_inst;
    } else if (auto ret = dynamic_cast<ir::instruction::Return *>(inst)) {
        auto ret_val = ret->getvalue();
        if (ret->has_return_value()) ret_val = reg_map_if(ret_val);
        auto new_inst = new ir::instruction::Return(ret_val);
    } else if (auto ep = dynamic_cast<ir::instruction::ElementPtr *>(inst)) {
        auto indices = ep->get_indices();
        for(auto &index : indices){
            index = reg_map_if(index);
        }
        auto new_inst = new ir::instruction::ElementPtr(reg_map_if(ep->get_dst()), reg_map_if(ep->get_base()), indices);
        return new_inst;
    } else if (auto phi = dynamic_cast<ir::instruction::Phi *>(inst)) {
        auto new_inst = new ir::instruction::Phi(reg_map_if(phi->getdst()), {}, {});
        // if (is_br) {
        //      if (inst->get_parent() == entry) { // new entry
        //         for (auto pair : phi->getpairs()) {
        //             if (la_->get_loop(pair.second) == nullptr || la_->get_loop(pair.second) != la_->get_loop(entry)) {
        //                 br_map[pair.first] = reg_map_if(phi->getdst());
        //                 new_inst->add_pair(pair.second, pair.first);
        //             }
        //             else {
        //                 br_map2[reg_map[!map_curid].at(pair.first)] = reg_map_if(phi->getdst());
        //                 new_inst->add_pair(bb_map[!map_curid].at(pair.second), reg_map[!map_curid].at(pair.first));
        //             }
        //         }
        //     } else {
        //         for (auto pair : phi->getpairs()) {
        //             auto mapped_bb = pair.second;
        //             if (bb_map[map_curid].count(pair.second))
        //                 mapped_bb = bb_map[map_curid].at(pair.second);
        //             auto mapped_reg = pair.first;
        //             if (reg_map[map_curid].count(mapped_reg)) 
        //                 mapped_reg = reg_map[map_curid].at(mapped_reg);
        //             new_inst->add_pair(mapped_bb, mapped_reg);
        //         }
        //     }
        // }
        // else if (is_last) {
        //     if (inst->get_parent() == entry) { // new entry
        //         for (auto pair : phi->getpairs()) {
        //             if (la_->get_loop(pair.second) == nullptr || la_->get_loop(pair.second) != la_->get_loop(entry)) 
        //                 continue;
        //             else {
        //                 assert(for_br != nullptr);
        //                 auto mapped_bb = for_br;
        //                 auto mapped_reg = reg_map[!map_curid].at(pair.first);
        //                 if (pair.first == info.upd_reg)
        //                     mapped_reg = info.for_last_bb;
        //                 else if (br_map2.count(mapped_reg))
        //                     mapped_reg = br_map2[mapped_reg];
        //                 new_inst->add_pair(for_br, mapped_reg);
        //                 new_inst->add_pair(bb_map[map_curid].at(pair.second), reg_map[map_curid].at(pair.first));
        //             }
        //         }
        //     } else {
        //         for (auto pair : phi->getpairs()) {
        //             auto mapped_bb = pair.second;
        //             if (bb_map[map_curid].count(pair.second))
        //                 mapped_bb = bb_map[map_curid].at(pair.second);
        //             auto mapped_reg = pair.first;
        //             if (reg_map[map_curid].count(mapped_reg)) 
        //                 mapped_reg = reg_map[map_curid].at(mapped_reg);
        //             new_inst->add_pair(mapped_bb, mapped_reg);
        //         }
        //     }
        // }
        // else {
        auto in_loop = [=](BasicBlock *b) {
            Loop *l = la_->get_loop(b);
            while (l != nullptr && l != loop) {
                l = l->parent_;
            }
            return l == loop;
        };
        if (inst->get_parent() == entry) { // new entry
            for (auto pair : phi->getpairs()) {
                if (!in_loop(pair.second)) {
                    continue;                        
                }
                auto mapped_bb = bb_map[!map_curid].at(pair.second);
                auto mapped_reg = reg_map[!map_curid].at(pair.first);
                new_inst->add_pair(mapped_bb, mapped_reg);
            }
        } else {
            for (auto pair : phi->getpairs()) {
                auto mapped_bb = pair.second;
                if (bb_map[map_curid].count(pair.second))
                    mapped_bb = bb_map[map_curid].at(pair.second);
                auto mapped_reg = pair.first;
                if (reg_map[map_curid].count(mapped_reg)) 
                    mapped_reg = reg_map[map_curid].at(mapped_reg);
                new_inst->add_pair(mapped_bb, mapped_reg);
            }
        }
        // }
        if (new_inst->getpairs().size() == 1) {
            auto new_rst = new ir::instruction::Assign(new_inst->getdst(), new_inst->getvalues()->front());
            return new_rst;
        }
        return new_inst;
    } else if (auto alloca = dynamic_cast<ir::instruction::Alloca *>(inst)) {
        auto new_inst = new ir::instruction::Alloca(reg_map_if(alloca->getaddr()), alloca->gettype());
        return new_inst;
    } else if (auto cast = dynamic_cast<ir::instruction::Cast *>(inst)) {
        auto new_inst = new ir::instruction::Cast(reg_map_if(cast->getdst()), reg_map_if(cast->getsrc()), cast->gettype());
        return new_inst;
    } else assert(false);
    __builtin_unreachable();
}

void LoopUnroll::copy_bb_br(SimpleLoopInfo& info, ir::BasicBlock *bb, ir::BasicBlock *new_bb, ir::BasicBlock *entry, ir::BasicBlock *exit, ir::BasicBlock *for_br) {
    // map prev & succ
    for (auto prev_bb : cfg_->prev(bb)) {
        if (bb == entry) continue; // new_entry's prev has been done
        cfg_->insert_prev(new_bb, bb_map[map_curid].at(prev_bb));
    }
    for (auto succ_bb : cfg_->succ(bb)) {
        if (succ_bb == entry) {
            if (info.loop_type == 0 || (info.loop_type == 2)) {
                cfg_->insert_succ(new_bb, succ_bb);
                cfg_->insert_prev(succ_bb, new_bb);
            } else if (info.loop_type == 3) {
                cfg_->insert_succ(new_bb, bb_map[map_curid].at(succ_bb));
            }
        } else if (succ_bb == exit) {
            if (info.loop_type == 0 || (info.loop_type == 3)) {
                cfg_->insert_succ(new_bb, succ_bb);
                cfg_->insert_prev(succ_bb, new_bb);
            }
        } else {
            cfg_->insert_succ(new_bb, bb_map[map_curid].at(succ_bb));
        }
    }
    // copy insts
    
}


void LoopUnroll::copy_bb_last(SimpleLoopInfo& info, ir::BasicBlock *bb, ir::BasicBlock *new_bb, ir::BasicBlock *entry, ir::BasicBlock *exit, ir::BasicBlock *for_br) {
    // map prev & succ
    for (auto prev_bb : cfg_->prev(bb)) {
        if (bb == entry) continue; // new_entry's prev has been done
        cfg_->insert_prev(new_bb, bb_map[map_curid].at(prev_bb));
    }
    for (auto succ_bb : cfg_->succ(bb)) {
        if (succ_bb == entry) {
            if (info.loop_type == 0 || (info.loop_type == 2)) {
                cfg_->insert_succ(new_bb, succ_bb);
                cfg_->insert_prev(succ_bb, new_bb);
            } else if (info.loop_type == 3) {
                cfg_->insert_succ(new_bb, bb_map[map_curid].at(succ_bb));
            }
        } else if (succ_bb == exit) {
            if (info.loop_type == 0 || (info.loop_type == 3)) {
                cfg_->insert_succ(new_bb, succ_bb);
                cfg_->insert_prev(succ_bb, new_bb);
            }
        } else {
            cfg_->insert_succ(new_bb, bb_map[map_curid].at(succ_bb));
        }
    }
    auto tempid = func->get_temp_used();
    // Temp* new_imm_reg = new Temp(tempid++, Int);
    Temp* new_reg = new Temp(tempid++, Int);
    Temp* new_reg2 = new Temp(tempid++, Int);
    info.for_last_bb = new_reg2;
    Temp* new_reg3 = new Temp(tempid++, Int);
    Temp* new_reg5 = new Temp(tempid++, Int);
    func->set_temp_used(tempid);
    // info.new_cmp_reg = new_imm_reg;
    auto new_imm_i = new ir::instruction::BinaryImm(BinaryOp::Sub, info.new_cmp_reg, info.cmp_reg, UNROLL_CNT);
    new_imm_i->set_parent(info.into_entry);
    uda_->compute_use_def(new_imm_i);
    (info.into_entry)->add_instruction_before_terminal(new_imm_i);

    auto new_i = new ir::instruction::Phi(new_reg2, {reg_map[!map_curid].at(info.upd_reg), info.imm_reg}, {bb_map[!map_curid].at(bb), info.into_entry});
    auto new_i3 = new ir::instruction::Binary(BinaryOp::Lt, new_reg3, new_reg2, info.cmp_reg);
    auto new_i4 = new ir::instruction::CondBranch(ir::instruction::CondBranch::IRInstrCondBranch::IR_Instr_BEQ, bb_map[map_curid][bb], info.exit, new_reg3);
    new_i->set_parent(for_br);
    uda_->compute_use_def(new_i);
    for_br->add_instruction(new_i);
    new_i3->set_parent(for_br);
    uda_->compute_use_def(new_i3);
    for_br->add_instruction(new_i3);
    new_i4->set_parent(for_br);
    uda_->compute_use_def(new_i4);
    for_br->add_instruction(new_i4);
    // copy insts
    // for (auto &insn : *bb->get_instructions()) {
    //     auto new_inst_br = copy_inst(info, insn, entry, exit, false, for_br, true);
    //     TypeCase (phi, ir::instruction::Phi*, new_inst_br) {
    //         new_inst_br->set_parent(for_br);
    //         uda_->compute_use_def(new_inst_br);
    //         for_br->add_instruction_before_terminal(new_inst_br);
    //     }
    //     auto new_inst = copy_inst(info, insn, entry, exit, true, for_br);
    //     new_inst->set_parent(new_bb);
    //     uda_->compute_use_def(new_inst);
    //     new_bb->add_instruction(new_inst);
    // }
    // auto new_cmpr = new ir::instruction::Binary(BinaryOp::Lt, new_reg5, info.imm_reg, info.new_cmp_reg);
    // new_cmpr->set_parent(info.into_entry);
    // uda_->compute_use_def(new_cmpr);
    // info.into_entry->add_instruction_before_terminal(new_cmpr);

    // TypeCase (br, ir::instruction::CondBranch*, (*(info.into_entry)->get_instructions()).back())
    //     br->setcond(new_reg5);
    // TypeCase (br, ir::instruction::CondBranch*, (*(new_bb)->get_instructions()).back()) {
    //     br->set_false_bb(info.exit);
    //     br->set_true_bb(new_bb);
    // }
}

void LoopUnroll::copy_bb(Loop* loop, SimpleLoopInfo& info, bool last_turn, ir::BasicBlock *bb, ir::BasicBlock *new_bb, ir::BasicBlock *entry, ir::BasicBlock *exit) {
    // map prev & succ
    for (auto prev_bb : cfg_->prev(bb)) {
        if (bb == entry) continue; // new_entry's prev has been done
        cfg_->insert_prev(new_bb, bb_map[map_curid].at(prev_bb));
    }
    for (auto succ_bb : cfg_->succ(bb)) {
        if (succ_bb == entry) {
            if (info.loop_type == 0 || (info.loop_type == 2 && last_turn)) {
                cfg_->insert_succ(new_bb, succ_bb);
                cfg_->insert_prev(succ_bb, new_bb);
            } else if (info.loop_type == 3) {
                cfg_->insert_succ(new_bb, bb_map[map_curid].at(succ_bb));
            }
        } else if (succ_bb == exit) {
            if (info.loop_type == 0 || (info.loop_type == 3 && last_turn)) {
                cfg_->insert_succ(new_bb, succ_bb);
                cfg_->insert_prev(succ_bb, new_bb);
            }
        } else {
            cfg_->insert_succ(new_bb, bb_map[map_curid].at(succ_bb));
        }
    }
    // copy insts
    for (auto &insn : *bb->get_instructions()) {
        // std::cout << "<< "; insn->print(std::cout);
        auto new_inst = copy_inst(loop, info, insn, entry, exit);
        // std::cout << ">> "; new_inst->set_parent(new_bb);
        // new_inst->print(std::cout);
        uda_->compute_use_def(new_inst);
        new_bb->add_instruction(new_inst);
        new_inst->set_parent(new_bb);
    }
    // std::cout << "【bb】"; bb->print(std::cout);
    // std::cout << "【new bb】"; new_bb->print(std::cout);
    // std::cout << "end copy bb\n";
    new_bb->set_parent(func);
    if ((last_turn) && (info.loop_type == 0)) {
        auto tempid = func->get_temp_used();
        Temp* new_reg4a = new Temp(tempid++, Int);
        Temp* new_reg5a = new Temp(tempid++, Int);
        func->set_temp_used(tempid);
        info.new_cmp_reg = new_reg4a;
        // auto new_adda = new ir::instruction::BinaryImm(BinaryOp::Add, new_reg4a, reg_map[map_curid][info.upd_reg], UNROLL_CNT);
        auto new_cmpra = new ir::instruction::Binary(BinaryOp::Lt, new_reg5a, reg_map[map_curid][info.upd_reg], info.new_cmp_reg);
        // new_adda->set_parent(new_bb);
        // uda_->compute_use_def(new_adda);
        // new_bb->add_instruction_before_terminal(new_adda);
        new_cmpra->set_parent(new_bb);
        uda_->compute_use_def(new_cmpra);
        new_bb->add_instruction_before_terminal(new_cmpra);
        TypeCase (br, ir::instruction::CondBranch*, (*(new_bb)->get_instructions()).back())
            br->setcond(new_reg5a);
    }
}

int first = false;

void LoopUnroll::loop_unroll(Loop *loop, SimpleLoopInfo info, const std::unordered_set<ir::BasicBlock *> &loop_bbs, const int unroll_cnt) {
    if (first) return;
    success = true;
    last_turn_temp_used = 0;
    br_map.clear(); br_map2.clear(); br_map3.clear();
    std::cout << "loop unroll\n";
    map_curid = 0;
    bb_map[0].clear();
    bb_map[1].clear();
    reg_map[0].clear();
    reg_map[1].clear();
    ir::BasicBlock *exit_bb = info.exit;
    for (auto bb : loop_bbs) {
        bb_map[map_curid][bb] = bb;
        // std::cout << "map bb " << bb->get_name() << "\n";
        for (auto &insn : *bb->get_instructions()) {
            if (insn->is_output_inst()) {
                Temp* reg = insn->get_dst()->front();
                reg_map[map_curid][reg] = reg;
            }
        }
    }
    // std::cout << "【entry】" << info.header->get_name() << " 【exit】" <<info. exit->get_name() << " 【into_entry】" << info.into_entry->get_name() << " 【exit_prev】" << info.exit_prev->get_name() << "\n";
    // if (info.loop_type == 0) {
    //     // Add phi for regs defined in loop but used outside
    //     for (auto bb : loop_bbs) {
    //         for (auto &insn : *bb->get_instructions()) {
    //             if (insn->is_output_inst()) {
    //                 Temp* reg = insn->get_dst()->front(), *newreg;
    //                 if (!uda_->get_usesets().count(reg)) continue;
    //                 bool flag = false;
    //                 std::vector<ir::Instruction*> insts_to_change_use;
    //                 for (auto i : uda_->get_useset(reg)) {
    //                     if (la_->get_loop(i->get_parent()) != nullptr && la_->get_loop(i->get_parent()) == loop) continue; // in loop
    //                     if (auto phi = dynamic_cast<ir::instruction::Phi*>(i)) {
    //                         if (i->get_parent() == exit_bb) continue;
    //                     }
    //                     if (!flag) { // create phi
    //                         auto reg_num = func->get_temp_used();
    //                         auto new_reg = new Temp(reg_num++, reg->get_type());
    //                         auto phi_i = new ir::instruction::Phi(new_reg, {}, {});
    //                         for (auto exit_prev: cfg_->prev(exit_bb)) {
    //                             assert(la_->get_loop(exit_prev) != nullptr && la_->get_loop(exit_prev) == loop);
    //                             phi_i->add_src_and_bb(exit_prev, reg);
    //                         }
    //                         exit_bb->add_instruction_front(phi_i);
    //                         phi_i->set_parent(exit_bb);
    //                         func->set_temp_used(reg_num);
    //                     }
    //                     flag = true;
    //                     insts_to_change_use.push_back(i);
    //                 }
    //                 for (auto i : insts_to_change_use) {
    //                     uda_->change_use(reg, newreg, i);
    //                 }
    //             }
    //         }
    //     }
    // }
    auto in_loop = [=](BasicBlock *b) {
        Loop *l = la_->get_loop(b);
        while (l != nullptr && l != loop) {
            l = l->parent_;
        }
        return l == loop;
    };
    std::unordered_set<ir::BasicBlock *> back_paths; // bbs in original loop that go back to entry
    for (auto entry_prev_bb : cfg_->prev(loop->header_)) {
        if (in_loop(entry_prev_bb)) { // bb in loop
            back_paths.insert(entry_prev_bb);
        }
    }
    std::unordered_set<ir::BasicBlock *> exit_paths; // bbs in original loop that go to exit
    for (auto exit_prev_bb : cfg_->prev(exit_bb)) {
        if (in_loop(exit_prev_bb)) { // bb in loop
            exit_paths.insert(exit_prev_bb);
        }
    }
    std::unordered_set<ir::BasicBlock *> loop0_to_entry; // bb in loop0 -> loop1-entry, inserted after for loop to make loop0 clean
    ir::BasicBlock *loop1_entry = nullptr;
    auto bbid = func->get_bb_used();
    ir::BasicBlock *bb_for_br = new ir::BasicBlock(func, {}, bbid++);
    func->add_basic_block(bb_for_br);
    func->set_bb_used(bbid);
    for (int l = 1; l < unroll_cnt + 2; l++) {
        if (l >= unroll_cnt && info.loop_type != 0) continue;
        // std::cout << "Unroll #" << l << "\n";
        map_curid ^= 1;
        // 1. Create new bbs
        auto bbid = func->get_bb_used();
        for (auto bb : loop_bbs) {
            if (bb == loop->header_ && l == unroll_cnt + 1) continue;
            ir::BasicBlock *new_bb = new ir::BasicBlock(bb->get_parent(), {}, bbid++);
            la_->set_loop(new_bb, loop);
            func->add_basic_block(new_bb);
            bb_map[map_curid][bb] = new_bb;
        }
        func->set_bb_used(bbid);
        // 2. Connect entry with prev loop
        auto entry = loop->header_;
        if (l == unroll_cnt + 1) bb_map[map_curid][loop->header_] = bb_for_br;
        auto new_entry = bb_map[map_curid][loop->header_];
        if (l == 1) 
            loop1_entry = new_entry;
        for (auto entry_prev_bb : back_paths) {
            if (l == 1) { // delayed to make loop0 clean
                loop0_to_entry.insert(bb_map[!map_curid][entry_prev_bb]);
            } 
            else if (l == unroll_cnt) {
                cfg_->erase_from_succ(bb_map[!map_curid][entry_prev_bb], entry);
                cfg_->insert_succ(bb_map[!map_curid][entry_prev_bb], new_entry);
                auto ter_inst = (*bb_map[!map_curid][entry_prev_bb]->get_instructions()).back();
                if (auto br = dynamic_cast<ir::instruction::CondBranch *>(ter_inst)) {
                    br->set_false_bb(bb_for_br);
                }
            }
            else if (l < unroll_cnt) {
                for (auto succ: cfg_->succ(bb_map[!map_curid][entry_prev_bb])) {}
                cfg_->erase_from_succ(bb_map[!map_curid][entry_prev_bb], entry);
                cfg_->insert_succ(bb_map[!map_curid][entry_prev_bb], new_entry);
                auto ter_inst = (*bb_map[!map_curid][entry_prev_bb]->get_instructions()).back();
                if (auto br = dynamic_cast<ir::instruction::Branch*>(ter_inst)) {
                    assert(br->get_bb() == entry);
                    br->set_target(new_entry);
                } 
                else if (auto br = dynamic_cast<ir::instruction::CondBranch *>(ter_inst)) {
                    assert(br->get_false_bb() == entry || br->get_true_bb() == entry);
                    auto insts = bb_map[!map_curid][entry_prev_bb]->get_instructions();
                    uda_->erase_use_def(insts->back());
                    auto jmp = new ir::instruction::Branch(new_entry);
                    jmp->set_parent(bb_map[!map_curid][entry_prev_bb]);
                    insts->pop_back();
                    insts->push_back(jmp);
                    uda_->compute_use_def(jmp);
                } else assert(false);
            }
            cfg_->insert_prev(new_entry, bb_map[!map_curid][entry_prev_bb]);
        }
        // 3. Fill new bbs
        for (auto bb : loop_bbs) {
            auto reg_idx = bb->get_parent()->get_temp_used();
            for (auto &insn : *bb->get_instructions()) {
                if (insn->get_dst()->size() > 0 && insn->get_dst()->front()) {
                    Temp * origin_reg = insn->get_dst()->front();
                    Temp * reg = new Temp(reg_idx++, origin_reg->get_type());
                    reg_map[map_curid][origin_reg] = reg;
                }
            }
            bb->get_parent()->set_temp_used(reg_idx);
        }
        // std::cout << "copy bb\n";
        for (auto bb : loop_bbs) {
            // if (l < unroll_cnt)
                copy_bb(loop, info, (l == unroll_cnt - 1), bb, bb_map[map_curid][bb], loop->header_, exit_bb);
            // else if (l == unroll_cnt)
            //     copy_bb_last(info, bb, bb_map[map_curid][bb], loop->header_, exit_bb, bb_for_br);
            // else 
            //     copy_bb_br(info, bb, bb_map[map_curid][bb], loop->header_, exit_bb, bb_for_br);
        }
        // std::cout << "modify entry\n";
        // 4. Modify entry's phi (Delayed), exit's phi
        if ((info.loop_type == 0 && l >= unroll_cnt) || (info.loop_type == 1 && l == unroll_cnt - 1)) {
            for (auto &insn : *exit_bb->get_instructions()) {
                if (auto phi = dynamic_cast<ir::instruction::Phi *>(insn)) {
                    uda_->erase_use_def(phi);
                    if (l == unroll_cnt) {
                        for (auto bb : exit_paths) {
                            Temp* reg = phi->get_src_temp(bb);
                            if (reg_map[map_curid].count(reg))
                                reg = reg_map[map_curid].at(reg);
                            // std::cout << bb->get_name() << ", " << bb_map[map_curid].at(bb)->get_name() << ", " << reg->to_str() << "\n";
                            phi->set_src_temp(bb, bb_map[map_curid].at(bb), reg);
                        }
                    }
                    Temp* reg = phi->get_src_temp(info.into_entry);
                    if (reg != nullptr && l == unroll_cnt + 1) {
                        if (br_map.count(reg))
                            phi->set_src_temp(info.into_entry, bb_for_br, br_map.at(reg));
                    }
                    uda_->compute_use_def(phi);
                } else break;
            }
        }
        // std::cout << "restore loop end\n";
        // 5. Restore loop end
        if (info.loop_type != 0) {
            Temp* new_cond_reg = reg_map[map_curid].at(info.cond_reg);
            ir::Instruction* binary_cond = (*uda_->get_defset(new_cond_reg).begin());
            Temp* new_end_reg = info.new_end_reg;
            if (reg_map[map_curid].count(info.new_end_reg)) {
                new_end_reg = reg_map[map_curid].at(info.new_end_reg);
            }
            Temp* old_end_reg = info.end_reg;
            if (reg_map[map_curid].count(info.end_reg)) {
                old_end_reg = reg_map[map_curid].at(info.end_reg);
            }
            uda_->change_use(new_end_reg, old_end_reg, binary_cond);
        }
        // std::cout << "modify entry's br\n";
        if ((l == unroll_cnt && info.loop_type == 0) || (info.loop_type == 1 && l == unroll_cnt - 1)) {
            // TypeCase (br, ir::instruction::CondBranch*, (*(info.into_entry)->get_instructions()).back()) {
            //     br->set_false_bb(bb_for_br);
            // }
            if (info.loop_type == 0) map_curid ^= 1;
            // Modify Loop0 entry's phi & prev
            if (info.loop_type != 0) { // delete phi & prev
                // delete loop0 entry's prev in loop0
                for (auto bb : back_paths) {
                    cfg_->erase_from_prev(loop->header_, bb);
                }
                // delete exit's prev in loop0
                assert(exit_paths.size() == 1);
                for (auto bb : exit_paths) { // only one exit_prev
                    cfg_->erase_from_prev(exit_bb, bb);
                    cfg_->erase_from_succ(bb, exit_bb); // delete entry's succ to exit
                }
                if (info.loop_type == 1) {
                    for (auto bb : exit_paths) { // only one exit_prev
                        cfg_->insert_prev(exit_bb, bb_map[map_curid].at(bb));
                        cfg_->insert_succ(bb_map[map_curid].at(bb), exit_bb);
                        cfg_->erase_from_succ(bb, exit_bb); // delete loop0 to exit
                    }
                }
                if (info.loop_type == 2) {
                    cfg_->erase_from_prev(exit_bb, info.into_entry); // because into_entry now points to new_into_entry
                }
                // modify exit's phi
                for (auto &insn : *exit_bb->get_instructions()) {
                    if (auto phi = dynamic_cast<ir::instruction::Phi *>(insn)) {
                        uda_->erase_use_def(phi);
                        for (auto bb : exit_paths) {
                            Temp* new_reg = phi->get_src_temp(bb);
                            if (new_reg != nullptr) {
                                if (reg_map[map_curid].count(new_reg)) new_reg = reg_map[map_curid].at(new_reg);
                                phi->add_pair(bb_map[map_curid].at(bb), new_reg);
                            }
                            phi->erase_src_temp(bb);
                        }
                        if (info.loop_type == 2) {
                            phi->erase_src_temp(info.into_entry); // because into_entry now points to new_into_entry
                        }
                        uda_->compute_use_def(phi);
                        if (phi->getpairs().size() == 1) {
                            insn = new ir::instruction::Assign(phi->getdst(), phi->getvalues()->front());
                        }
                    } else break;
                }
            }
            // std::cout << "delete last bb's Br\n";
            if (info.loop_type == 1) {
                // delete last bb's Br
                auto insts = bb_map[map_curid].at(info.exit_prev)->get_instructions();
                uda_->erase_use_def(insts->back());
                insts->pop_back();
                auto jmp = new ir::instruction::Branch(exit_bb);
                insts->push_back(jmp);
                jmp->set_parent(bb_map[map_curid].at(info.exit_prev));
                uda_->compute_use_def(jmp);
                for (auto &insn : *loop->header_->get_instructions()) {
                    if (auto phi = dynamic_cast<ir::instruction::Phi *>(insn)) {
                        uda_->erase_use_def(phi);
                        for (auto bb : back_paths) {
                            phi->erase_src_temp(bb);
                        }
                        uda_->compute_use_def(phi);
                    } else break;
                }
            } 
            else {
                if (info.loop_type == 2) {
                    map_curid ^= 1; // correct map_cur_id
                    reg_map[map_curid] = reg_map[2]; // restore map
                }
                std::vector<ir::BasicBlock *> entry_prev_bb_to_insert;
                for (auto bb : back_paths) {
                    entry_prev_bb_to_insert.push_back(bb_map[map_curid].at(bb));
                }
                for (auto bb : entry_prev_bb_to_insert) {
                    cfg_->insert_prev(loop->header_, bb);
                }
                for (auto &insn : *loop->header_->get_instructions()) {
                    if (auto phi = dynamic_cast<ir::instruction::Phi *>(insn)) {
                        uda_->erase_use_def(phi);
                        for (auto bb : back_paths) {
                            Temp* reg = phi->get_src_temp(bb);
                            if (reg_map[map_curid].count(reg)) 
                                reg = reg_map[map_curid].at(reg);
                            phi->erase_src_temp(bb);
                            phi->add_src_and_bb(bb_map[map_curid].at(bb), reg);
                        }
                        uda_->compute_use_def(phi);
                    } else break;
                }
                if (info.loop_type == 2) map_curid ^= 1;
            }
            for (auto bb : loop0_to_entry) {
                cfg_->erase_from_succ(bb, loop->header_);
                cfg_->erase_from_succ(bb, loop1_entry);
                auto ter_inst = bb->get_instructions()->back();
                if (auto jmp = dynamic_cast<ir::instruction::Branch *>(ter_inst)) {
                    assert(jmp->get_bb() == loop->header_);
                    jmp->set_target(loop1_entry);
                } 
                else if (auto br = dynamic_cast<ir::instruction::CondBranch *>(ter_inst)) {
                    assert(br->get_true_bb() == loop->header_ || br->get_false_bb() == loop->header_);
                    uda_->erase_use_def(bb->get_instructions()->back());
                    auto jmp = new ir::instruction::Branch(loop1_entry);
                    jmp->set_parent(bb);
                    bb->get_instructions()->pop_back();
                    bb->get_instructions()->push_back(jmp);
                    uda_->compute_use_def(jmp);
                } else assert(false);
            }
            if (info.loop_type != 0) {
                // Modify regs defined in loop but used outside
                for (auto bb : loop_bbs) {
                    for (auto &insn : *bb->get_instructions()) {
                        if (insn->is_output_inst()) {
                            Temp* reg = insn->get_dst()->front();
                            Temp* newreg = reg_map[map_curid].at(reg);
                            if (uda_->get_useset(reg).size() == 0) {
                                continue;
                            }
                            std::vector<ir::Instruction *> insts_to_change_use;
                            for (auto i : uda_->get_useset(reg)) {
                                if (in_loop(i->get_parent())) continue; // in loop
                                insts_to_change_use.push_back(i);
                            }
                            for (auto i : insts_to_change_use) {
                                i->change_use(reg, newreg);
                            }
                        }
                    }
                }
            }
        }
    }
}

void LoopUnroll::run() {
    int total_unroll_inst = 0;
    // func->print(std::cout);
    for (auto &bb : *func->get_basic_blocks()) {
        for (auto &insn : *bb->get_instructions()) {
            insn->set_parent(bb);
        }
        bb->set_parent(func);
    }
    for (auto &loop : la_->loops) {
        if (loop->child_ != nullptr) // only perform on deepest loop
            continue;
        // auto l = loop;
        // bool checked = false;
        // while (l) {
        //     if (l->unrolled) checked = true;
        //     l = l->child_;
        // }
        // // if (loop->parent_ && loop->parent_->unrolled) continue;
        // if (checked) continue;
        std::unordered_set<ir::BasicBlock *> loop_bbs; // add bbs into loop_bbs
        std::vector<ir::BasicBlock *> stack;
        stack.push_back(loop->header_);
        auto in_loop = [=](BasicBlock *b) {
        Loop *l = la_->get_loop(b);
            while (l != nullptr && l != loop) {
                l = l->parent_;
            }
            return l == loop;
        };
        while (stack.size()) {
            auto bb = stack.back();
            stack.pop_back();
            if (in_loop(bb)) {
                loop_bbs.insert(bb);
                for (auto each : dt_->get_dominate(bb)) {
                    stack.push_back(each);
                }
            }
        }
        
        // std::cout << "get loop_bbs\n";
        BasicBlock *exit_bb = nullptr;
        bool check = true; // check loop has only one exit
        for (auto bb : loop_bbs) {
            if (!check) break;
            for (auto succ_bb : cfg_->succ(bb)) {
                if (!in_loop(succ_bb)) {
                    if (exit_bb != nullptr && succ_bb != exit_bb) {
                        check = false;
                        break;
                    }
                    exit_bb = succ_bb;
                }
            }
        }
        if (exit_bb == nullptr || !check)  { // dead loop or multiple exit
            return;
        }

        // Unroll this loop
        auto loop_info = get_loop_info(loop, loop_bbs, exit_bb);
        for (auto bb: loop_bbs) {
            for (auto i: *bb->get_instructions()) {
                TypeCase (phi, ir::instruction::Phi*, i) {
                    for (auto u: phi->getpairs()) {
                        if (!in_loop(u.second)) continue;
                        bool found = false;
                        for (auto bb: loop_bbs) {
                            if (uda_->get_defsets().count(u.first) == 0 || std::find(bb->get_instructions()->begin(), bb->get_instructions()->end(), *uda_->get_defset(u.first).begin()) != bb->get_instructions()->end()) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) { loop_info.valid = false; break; }
                    }
                }
            }
        }
        // std::cout << "start = " << loop_info.start << ", end = " << loop_info.end << ", step = " << loop_info.step << "\n";
        // std::cout << "inst_cnt = " << loop_info.inst_cnt << ", type = " << loop_info.loop_type << "\n";
        if (loop_info.inst_cnt > 100) continue;
        // if (loop_info.start == loop_info.end) continue;
        // if (loop_info.loop_type == 0) continue;
        loop_info.exit = exit_bb;
        int unroll_cnt = UNROLL_CNT;
        if (loop_info.loop_type == 1) {
            int full_cnt;
            if (loop_info.cond_op == BinaryOp::Lt) {
                full_cnt = (loop_info.end - loop_info.start + loop_info.step - 1) / loop_info.step;
                if (loop_info.start >= loop_info.end) full_cnt = 0;
            } else if (loop_info.cond_op == BinaryOp::Leq) {
                full_cnt = (loop_info.end - loop_info.start) / loop_info.step + 1;
                if (loop_info.start > loop_info.end) full_cnt = 0;
            } else assert(false);
            assert(full_cnt >= 0);
            // std::cout << "【full_cnt】" << full_cnt << "\n";
            if (full_cnt == 0 || full_cnt == 1) continue;
            if (full_cnt <= 100 && full_cnt * loop_info.inst_cnt <= 3000 && total_unroll_inst <= 3) {
                unroll_cnt = full_cnt; // fully unroll
            } else continue; // TODO: temporarily disabled
            total_unroll_inst += 1;
        } 
        else {
            continue;
        }
        // std::cout << "【unroll_cnt】" << unroll_cnt << "\n";
        if (loop_info.loop_type == 1 && !loop_info.valid) continue;
        if (loop_info.end_reg == nullptr) continue;
        // if (loop_info.loop_type == 0 && loop_info.inst_cnt >= 100) continue;
        loop->unrolled = true;
        loop_unroll(loop, loop_info, loop_bbs, unroll_cnt);
        // func->print(std::cout);
        // if (loop_info.loop_type == 2 && loop_bbs.size() > 1) continue; // not unroll type 2 loop with branches
        // if (loop_info.loop_type == 2) continue; // TODO: temporarily stop unrolling type 2 loop
        // if (loop_info.loop_type == 2) { // Decrease end by unroll * step, therefore erase branches jump out in middle
        //     auto binary_cond = *uda_->get_defset(loop_info.cond_reg).begin();
        //     //  func->def_list.at(*loop_info.cond_reg);
        //     auto idx = func->get_temp_used();
        //     Temp* new_end = new Temp(idx++, ScalarType::Int);
        //     Temp* imm = new Temp(idx++, ScalarType::Int);
        //     auto loadimm_offset = new ir::instruction::LoadImm4(imm, ConstValue(loop_info.step * unroll_cnt));
        //     auto binary_new_end = new ir::instruction::Binary(BinaryOp::Sub, new_end, loop_info.end_reg, imm);
        //     if (func->is_param(loop_info.end_reg)) {
        //         (*func->get_basic_blocks()).front()->add_instruction_front(binary_new_end);
        //         (*func->get_basic_blocks()).front()->add_instruction_front(loadimm_offset);
        //     } else {
        //         auto inst_end = *uda_->get_defset(loop_info.end_reg).begin();
        //         inst_end->get_parent()->add_instruction_after_inst(inst_end, binary_new_end);
        //         inst_end->get_parent()->add_instruction_after_inst(inst_end, loadimm_offset);
        //     }
        //     uda_->change_use(loop_info.end_reg, new_end, binary_cond);
        //     loop_info.new_end_reg = new_end; // save for later use
        //     // into_entry cond change
        //     Temp* new_cond_reg = new Temp(idx++, ScalarType::Int);
        //     func->set_temp_used(idx);
        //     auto new_into_cond = new ir::instruction::Binary(loop_info.into_cond->get_type(), new_cond_reg, loop_info.into_cond->getlhs(), loop_info.into_cond->getrhs());
        //     loop_info.into_cond->get_parent()->add_instruction_after_inst(loop_info.into_cond, new_into_cond); // create new cond
        //     uda_->change_use(loop_info.end_reg, new_end, new_into_cond);
        //     uda_->change_use(loop_info.into_cond->getdst(), new_into_cond->getdst(), loop_info.into_br);
        // }
    }
    // std::cout << "end of run()\n";
    // func->print(std::cout);
}

}