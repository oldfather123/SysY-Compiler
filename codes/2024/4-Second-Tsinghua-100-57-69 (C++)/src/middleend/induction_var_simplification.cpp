#include "middleend/induction_var_simplification.hpp"
#include <cmath>

namespace middleend {

bool InductionVarSimplification::run() {
    int next_temp_id = func_->get_temp_used();
    for (auto body_bb : *func_->get_basic_blocks()) {
        auto body_term = body_bb->get_instructions()->back();
        ir::BasicBlock *head_bb = nullptr;
        ir::BasicBlock *tail_bb = nullptr;
        TypeCase(condbranch, ir::instruction::CondBranch *, body_term) {
            if (condbranch->get_true_bb() == body_bb) {
                tail_bb = condbranch->get_false_bb();
            } else if (condbranch->get_false_bb() == body_bb) {
                tail_bb = condbranch->get_true_bb();
            } else {
                continue;
            }
        } else {
            continue;
        }
        if (cfg_->get_bb_prev(tail_bb->get_index())->size() != 2) {
            continue;
        }
        if (cfg_->get_bb_prev(body_bb->get_index())->size() != 2) {
            continue;
        }
        for (auto body_prev_idx : *cfg_->get_bb_prev(body_bb->get_index())) {
            auto body_prev = cfg_->get_bb(body_prev_idx);
            if (body_prev == body_bb) {
                continue;
            }
            head_bb = body_prev;
        }
        auto head_term = head_bb->get_instructions()->back();
        TypeCase(condbranch, ir::instruction::CondBranch *, head_term) {
            if (condbranch->get_true_bb() == body_bb) {
                if (condbranch->get_false_bb() != tail_bb) {
                    continue;
                }
            } else if (condbranch->get_false_bb() == body_bb) {
                if (condbranch->get_true_bb() != tail_bb) {
                    continue;
                }
            } else {
                continue;
            }
        } else {
            continue;
        }
        // 至此得到循环的三个基本块
        Temp *low, *high, *mid, *mid_src;
        TypeCase(condbranch, ir::instruction::CondBranch *, head_term) { // TODO: 处理其他情况
            if (condbranch->get_type() != ir::instruction::CondBranch::IRInstrCondBranch::IR_Instr_BEQ) {
                continue;
            }
            if (condbranch->get_false_bb() != tail_bb) {
                continue;
            }
            if (condbranch->get_true_bb() != body_bb) {
                continue;
            }
            auto cond_inst = *uda_->get_defset(condbranch->getcond()).begin();
            TypeCase(binary, ir::instruction::Binary *, cond_inst) { // TODO: 处理binaryimm的情况
                if (binary->get_type() == BinaryOp::Lt) {
                    low = binary->getlhs();
                    high = binary->getrhs();
                } else if (binary->get_type() == BinaryOp::Gt) {
                    low = binary->getrhs();
                    high = binary->getlhs();
                } else { // TODO: 处理小于等于和大于等于的情况
                    continue;
                }
            } else {
                continue;
            }
        }
        TypeCase(condbranch, ir::instruction::CondBranch *, body_term) { // TODO: 处理其他情况
            if (condbranch->get_type() != ir::instruction::CondBranch::IRInstrCondBranch::IR_Instr_BEQ) {
                continue;
            }
            if (condbranch->get_false_bb() != tail_bb) {
                continue;
            }
            if (condbranch->get_true_bb() != body_bb) {
                continue;
            }
            auto cond_inst = *uda_->get_defset(condbranch->getcond()).begin();
            TypeCase(binary, ir::instruction::Binary *, cond_inst) { // TODO: 处理binaryimm的情况
                if (binary->get_type() == BinaryOp::Lt) {
                    mid = binary->getlhs();
                    if (binary->getrhs() != high) {
                        continue;
                    }
                } else if (binary->get_type() == BinaryOp::Gt) {
                    mid = binary->getrhs();
                    if (binary->getlhs() != high) {
                        continue;
                    }
                } else { // TODO: 处理小于等于和大于等于的情况
                    continue;
                }
            } else {
                continue;
            }
        }
        auto mid_inst = *uda_->get_defset(mid).begin();
        TypeCase(binaryimm, ir::instruction::BinaryImm *, mid_inst) {
            if (binaryimm->get_type() != BinaryOp::Add) { // TODO: 处理其他情况
                continue;
            }
            if (binaryimm->getimm() != 1) { // TODO: 处理其他情况
                continue;
            }
            mid_src = binaryimm->getlhs();
        } else {
            continue;
        }
        auto mid_src_inst = *uda_->get_defset(mid_src).begin();
        TypeCase(phi, ir::instruction::Phi *, mid_src_inst) {
            if (phi->getsize() != 2) {
                continue;
            }
            if (phi->get_src_temp(head_bb) != low) {
                continue;
            }
            if (phi->get_src_temp(body_bb) != mid) {
                continue;
            }
        } else {
            continue;
        }
        // 至此确认得到循环的结构
        // 修改浮点数累加的上界
        std::unordered_map<ir::instruction::Phi *, bool> body_phi_map;
        ir::instruction::Phi *jump_phi = nullptr;
        bool float_flag = true;
        for (auto inst : *body_bb->get_instructions()) {
            TypeCase(body_phi, ir::instruction::Phi *, inst) {
                if (body_phi->getdst()->get_type() == ScalarType::Float) {
                    body_phi_map[body_phi] = false;
                } else {
                    if (jump_phi == nullptr) {
                        jump_phi = body_phi;
                    } else {
                        float_flag = false;
                        break;
                    }
                }
            }
        }
        if (body_phi_map.size() == 0) {
            float_flag = false;
        }
        if (jump_phi == nullptr) {
            float_flag = false;
        }
        if (body_phi_map.size() * 2 + 4 != body_bb->get_instructions()->size()) {
            float_flag = false;
        }
        if (float_flag) {
            for (auto inst : *tail_bb->get_instructions()) {
                TypeCase(tail_phi, ir::instruction::Phi *, inst) {
                    for (auto &item : body_phi_map) {
                        if (tail_phi->get_src_temp(body_bb) == item.first->get_src_temp(body_bb) && tail_phi->get_src_temp(head_bb) == item.first->get_src_temp(head_bb)) {
                            item.second = true;
                            break;
                        }
                    }
                }
            }
            for (auto &item : body_phi_map) {
                if (!item.second) {
                    float_flag = false;
                    break;
                }
            }
        }
        if (float_flag) {
            Temp *new_high = new Temp(next_temp_id ++, low->get_type());
            for (auto iter = head_bb->get_instructions()->begin(); iter != head_bb->get_instructions()->end(); iter ++) {
                auto inst = *iter;
                TypeCase(phi, ir::instruction::Phi *, inst) {
                    continue;
                } else {
                    Temp *bound = new Temp(next_temp_id ++, low->get_type());
                    auto loadimm = new ir::instruction::LoadImm4(bound, 30000000);
                    loadimm->set_parent(head_bb);
                    iter = head_bb->get_instructions()->insert(iter, loadimm);
                    iter ++;
                    auto minimum = new ir::instruction::Minimum(new_high, high, bound);
                    minimum->set_parent(head_bb);
                    iter = head_bb->get_instructions()->insert(iter, minimum);
                    iter ++;
                    Temp *new_cond = new Temp(next_temp_id ++, low->get_type());
                    auto binary = new ir::instruction::Binary(BinaryOp::Lt, new_cond, low, new_high);
                    binary->set_parent(head_bb);
                    iter = head_bb->get_instructions()->insert(iter, binary);
                    iter ++;
                    TypeCase(condbranch, ir::instruction::CondBranch *, head_term) {
                        condbranch->setcond(new_cond);
                    }
                    break;
                }
            }
            for (auto iter = body_bb->get_instructions()->begin(); iter != body_bb->get_instructions()->end(); iter ++) {
                auto inst = *iter;
                TypeCase(phi, ir::instruction::Phi *, inst) {
                    continue;
                } else {
                    Temp *new_cond = new Temp(next_temp_id ++, low->get_type());
                    auto binary = new ir::instruction::Binary(BinaryOp::Lt, new_cond, mid, new_high);
                    binary->set_parent(body_bb);
                    iter = body_bb->get_instructions()->insert(iter, binary);
                    iter ++;
                    TypeCase(condbranch, ir::instruction::CondBranch *, body_term) {
                        condbranch->setcond(new_cond);
                    }
                    break;
                }
            }
            func_->set_temp_used(next_temp_id);
            return true;
        }

        // 将累加转化为乘法
        auto last_phi_iter = tail_bb->get_instructions()->begin();
        while (true) {
            auto inst = *last_phi_iter;
            TypeCase(phi, ir::instruction::Phi *, inst) {
                last_phi_iter ++;
            } else {
                break;
            }
        }
        if (last_phi_iter == tail_bb->get_instructions()->begin()) {
            continue;
        }
        last_phi_iter --;
        for (auto iter = tail_bb->get_instructions()->begin(); iter != tail_bb->get_instructions()->end(); iter++) {
            auto &inst = *iter;
            TypeCase(phi, ir::instruction::Phi *, inst) {
                Temp *mid_val = phi->get_src_temp(body_bb);
                Temp *init_val = phi->get_src_temp(head_bb);
                Temp *mid_phi_val;
                bool phi_match = false;
                for (auto body_inst : *body_bb->get_instructions()) {
                    TypeCase(body_phi, ir::instruction::Phi *, body_inst) {
                        if (body_phi->get_src_temp(head_bb) == init_val && body_phi->get_src_temp(body_bb) == mid_val) {
                            phi_match = true;
                            mid_phi_val = body_phi->getdst();
                            break;
                        }
                    } else {
                        break;
                    }
                }
                if (!phi_match) {
                    continue;
                }
                auto mid_val_inst = *uda_->get_defset(mid_val).begin();
                assert(mid_val_inst->get_parent() == body_bb);
                bool mod_end = false;
                Temp *mod_val;
                int mod_val_imm;
                TypeCase(binary, ir::instruction::Binary *, mid_val_inst) { // TODO: 处理binaryimm mod的情况
                    if (binary->get_type() == BinaryOp::Mod) {
                        mod_end = true;
                        mod_val = binary->getrhs();
                        auto mod_val_inst = *uda_->get_defset(mod_val).begin();
                        TypeCase(loadimm, ir::instruction::LoadImm4 *, mod_val_inst) {
                            if (loadimm->getimm().type != Int) {
                                continue;
                            }
                            mod_val_imm = loadimm->getimm().iv;
                        } else {
                            continue;
                        }
                        mid_val_inst = *uda_->get_defset(binary->getlhs()).begin();
                    }
                }
                TypeCase(binaryimm, ir::instruction::BinaryImm *, mid_val_inst) { // TODO: 处理binary的情况
                    if (binaryimm->getlhs() != mid_phi_val) {
                        continue;
                    }
                    if (binaryimm->get_type() != BinaryOp::Add) {
                        continue;
                    }
                    int add_val = binaryimm->getimm();
                    if (mod_end) {
                        add_val = add_val % mod_val_imm;
                        if (std::log2(std::abs(add_val)) + std::log2(std::abs(mod_val_imm)) > 31) {
                            continue;
                        }
                        Temp *dst = phi->getdst();
                        auto &last_phi = *last_phi_iter;
                        inst = last_phi;
                        Temp *interval = new Temp(next_temp_id ++, low->get_type());
                        last_phi = new ir::instruction::Binary(BinaryOp::Sub, interval, high, low);
                        last_phi->set_parent(tail_bb);
                        last_phi_iter ++;
                        Temp *interval_mod = new Temp(next_temp_id ++, low->get_type());
                        auto mod1 = new ir::instruction::Binary(BinaryOp::Mod, interval_mod, interval, mod_val);
                        mod1->set_parent(tail_bb);
                        last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, mod1);
                        last_phi_iter ++;
                        Temp *mul_val = new Temp(next_temp_id ++, dst->get_type());
                        Temp *mul_temp = new Temp(next_temp_id ++, dst->get_type());
                        auto loadimm = new ir::instruction::LoadImm4(mul_temp, add_val);
                        loadimm->set_parent(tail_bb);
                        last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, loadimm);
                        last_phi_iter ++;
                        auto mul1 = new ir::instruction::Binary(BinaryOp::Mul, mul_val, interval_mod, mul_temp);
                        mul1->set_parent(tail_bb);
                        last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, mul1);
                        last_phi_iter ++;
                        Temp *add_temp = new Temp(next_temp_id ++, dst->get_type());
                        auto add1 = new ir::instruction::Binary(BinaryOp::Add, add_temp, mul_val, init_val);
                        add1->set_parent(tail_bb);
                        last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, add1);
                        last_phi_iter ++;
                        auto mod2 = new ir::instruction::Binary(BinaryOp::Mod, dst, add_temp, mod_val);
                        mod2->set_parent(tail_bb);
                        last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, mod2);
                        last_phi_iter ++;
                        func_->set_temp_used(next_temp_id);
                        return true;
                    } else {
                        Temp *dst = phi->getdst();
                        auto &last_phi = *last_phi_iter;
                        inst = last_phi;
                        Temp *interval = new Temp(next_temp_id ++, low->get_type());
                        last_phi = new ir::instruction::Binary(BinaryOp::Sub, interval, high, low);
                        last_phi->set_parent(tail_bb);
                        last_phi_iter ++;
                        Temp *mul_temp = new Temp(next_temp_id ++, low->get_type());
                        auto loadimm = new ir::instruction::LoadImm4(mul_temp, add_val);
                        loadimm->set_parent(tail_bb);
                        last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, loadimm);
                        last_phi_iter ++;
                        Temp *mul_temp2 = new Temp(next_temp_id ++, low->get_type());
                        auto mul1 = new ir::instruction::Binary(BinaryOp::Mul, mul_temp2, interval, mul_temp);
                        mul1->set_parent(tail_bb);
                        last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, mul1);
                        last_phi_iter ++;
                        auto add1 = new ir::instruction::Binary(BinaryOp::Add, dst, mul_temp2, init_val);
                        add1->set_parent(tail_bb);
                        last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, add1);
                        last_phi_iter ++;
                        func_->set_temp_used(next_temp_id);
                        return true;
                    }
                } else TypeCase(binary, ir::instruction::Binary *, mid_val_inst) {
                    if (binary->get_type() != BinaryOp::Add) {
                        continue;
                    }
                    if (mod_end) {
                        continue;
                    }
                    Temp *add_val;
                    if (binary->getlhs() == mid_phi_val) {
                        add_val = binary->getrhs();
                    } else if (binary->getrhs() == mid_phi_val) {
                        add_val = binary->getlhs();
                    } else {
                        continue;
                    }
                    Temp *dst = phi->getdst();
                    if (dst->get_type().base_type == ScalarType::Float) {
                        continue; // 浮点数存在精度损失
                        // auto &last_phi = *last_phi_iter;
                        // inst = last_phi;
                        // Temp *interval = new Temp(next_temp_id ++, low->get_type());
                        // last_phi = new ir::instruction::Binary(BinaryOp::Sub, interval, high, low);
                        // last_phi->set_parent(tail_bb);
                        // last_phi_iter ++;
                        // Temp *interval_cast = new Temp(next_temp_id ++, dst->get_type());
                        // auto cast = new ir::instruction::Cast(interval_cast, interval, ScalarType::Float);
                        // cast->set_parent(tail_bb);
                        // last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, cast);
                        // last_phi_iter ++;
                        // Temp *mul_temp = new Temp(next_temp_id ++, dst->get_type());
                        // auto mul1 = new ir::instruction::Binary(BinaryOp::Mul, mul_temp, interval_cast, add_val);
                        // mul1->set_parent(tail_bb);
                        // last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, mul1);
                        // last_phi_iter ++;
                        // auto add1 = new ir::instruction::Binary(BinaryOp::Add, dst, mul_temp, init_val);
                        // add1->set_parent(tail_bb);
                        // last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, add1);
                        // last_phi_iter ++;
                    } else {
                        auto &last_phi = *last_phi_iter;
                        inst = last_phi;
                        Temp *interval = new Temp(next_temp_id ++, low->get_type());
                        last_phi = new ir::instruction::Binary(BinaryOp::Sub, interval, high, low);
                        last_phi->set_parent(tail_bb);
                        last_phi_iter ++;
                        Temp *mul_temp = new Temp(next_temp_id ++, dst->get_type());
                        auto mul1 = new ir::instruction::Binary(BinaryOp::Mul, mul_temp, interval, add_val);
                        mul1->set_parent(tail_bb);
                        last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, mul1);
                        last_phi_iter ++;
                        auto add1 = new ir::instruction::Binary(BinaryOp::Add, dst, mul_temp, init_val);
                        add1->set_parent(tail_bb);
                        last_phi_iter = tail_bb->get_instructions()->insert(last_phi_iter, add1);
                        last_phi_iter ++;
                    }
                    func_->set_temp_used(next_temp_id);
                    return true;
                } else {
                    continue;
                }
            } else {
                break;
            }
        }
    }
    func_->set_temp_used(next_temp_id);
    return false;
}

} // namespace middleend