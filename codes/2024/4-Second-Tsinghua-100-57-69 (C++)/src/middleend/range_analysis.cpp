#include "middleend/range_analysis.hpp"
#include "middleend/loop_dowhile.hpp"
#include <iostream>

namespace middleend {

void update_phi_in_ra(ir::BasicBlock* bb, int now_bb){
    for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
        if (auto phi = dynamic_cast<ir::instruction::Phi *>(*iter)) {
            std::vector<ir::BasicBlock *> bbs;
            std::vector<Temp *> values;
            for (int i = 0; i < phi->getbbs()->size(); i++) {
                if (now_bb != phi->getbbs()->at(i)->get_index()) {
                    bbs.push_back(phi->getbbs()->at(i));
                    values.push_back(phi->getvalues()->at(i));
                }
            }
            bb->get_instructions()->erase(iter);
            if (bbs.size() == 0) {
                assert(false);
            } else if (bbs.size() == 1) {
                bb->get_instructions()->insert(iter, new ir::instruction::Assign(phi->getdst(), values[0]));
            } else {
                bb->get_instructions()->insert(iter, new ir::instruction::Phi(phi->getdst(), values, bbs));
            }
            delete phi;
        }
        else break;
    }
}

void RangeAnalysis::unite(Temp *temp, ir::BasicBlock *bb, BinaryOp op, Temp *val, ir::BasicBlock *prev_bb) {
    if (constant_map.count(temp)) return;
    auto temp_bb = std::make_pair(temp, bb);
    if (!in_bb_range.count(temp_bb)) {
        in_bb_range[temp_bb] = ValueRange();
    }
    // auto temp_prev_bb = std::make_pair(temp, prev_bb);
    // if (!branch_range.count(temp_prev_bb)) {
    //     branch_range[temp_prev_bb] = ValueRange();
    // }
    if (in_bb_range[temp_bb].update(op, val, prev_bb)) changed = true;
    // branch_range[temp_prev_bb].update(op, val, prev_bb);
}

void RangeAnalysis::unite(Temp *temp, ir::BasicBlock *bb, ValueRange range) {
    if (range.empty()) return;
    auto temp_bb = std::make_pair(temp, bb);
    if (!in_bb_range.count(temp_bb)) {
        in_bb_range[temp_bb] = ValueRange();
    }
    if (range.bounds.count(BinaryOp::Lt)) {
        for (auto &[val, other_bb]: range.bounds[BinaryOp::Lt]) {
            if (in_bb_range[temp_bb].update(BinaryOp::Lt, val, other_bb)) changed = true;
        }
    }
    if (range.bounds.count(BinaryOp::Gt)) {
        for (auto &[val, other_bb]: range.bounds[BinaryOp::Gt]) {
            if (in_bb_range[temp_bb].update(BinaryOp::Gt, val, other_bb)) changed = true;
        }
    }
    if (range.bounds.count(BinaryOp::Leq)) {
        for (auto &[val, other_bb]: range.bounds[BinaryOp::Leq]) {
            if (in_bb_range[temp_bb].update(BinaryOp::Leq, val, other_bb)) changed = true;
        }
    }
    if (range.bounds.count(BinaryOp::Geq)) {
        for (auto &[val, other_bb]: range.bounds[BinaryOp::Geq]) {
            if (in_bb_range[temp_bb].update(BinaryOp::Geq, val, other_bb)) changed = true;
        }
    }
    if (range.bounds.count(BinaryOp::Neq)) {
        for (auto &[val, other_bb]: range.bounds[BinaryOp::Neq]) {
            if (in_bb_range[temp_bb].update(BinaryOp::Neq, val, other_bb)) changed = true;
        }
    }
}

void RangeAnalysis::insert_from_bb_range(Temp *temp, ir::BasicBlock *bb, BinaryOp op, Temp *val, ir::BasicBlock *prev_bb) {
    auto temp_prev_bb = std::make_tuple(temp, prev_bb, bb);
    if (!from_bb_range.count(temp_prev_bb)) {
        from_bb_range[temp_prev_bb] = ValueRange();
    }
    if (from_bb_range[temp_prev_bb].update(op, val, prev_bb)) changed = true;
    // std::cout << "insert_from_bb_range: " << temp->to_str() << " " << prev_bb->get_name() << " " << from_bb_range[temp_prev_bb].to_str() << std::endl;
}

void RangeAnalysis::update_val(ir::BasicBlock *bb, Temp *temp, bool val) {
    auto temp_bb = std::make_pair(temp, bb);
    if (!val_map.count(temp_bb)) {
        val_map[temp_bb] = val;
        // std::cout << "update_val: " << temp->to_str() << " in " << bb->get_name() << " to " << val << std::endl;
        changed = true;
    }
}

ValueRange RangeAnalysis::get_bound(Temp *temp, ir::BasicBlock *prev_bb) {
    auto temp_prev = std::make_pair(temp, prev_bb);
    if (in_bb_range.count(temp_prev)) {
        return in_bb_range[temp_prev];
    }
    while (dt_->get_idom(prev_bb)) {
        prev_bb = dt_->get_idom(prev_bb);
        temp_prev = std::make_pair(temp, prev_bb);
        if (in_bb_range.count(temp_prev)) {
            return in_bb_range[temp_prev];
        }
    }
    return ValueRange();
}

int RangeAnalysis::check_val(ir::BasicBlock *bb, Temp *temp) {
    auto temp_bb = std::make_pair(temp, bb);
    // std::cout << "check_val: " << temp->to_str() << " in " << bb->get_name() << std::endl;
    if (val_map.count(temp_bb)) return val_map[temp_bb];
    while (dt_->get_idom(bb)) {
        bb = dt_->get_idom(bb);
        // std::cout << "check_val: " << temp->to_str() << " in " << bb->get_name() << std::endl;
        temp_bb = std::make_pair(temp, bb);
        if (val_map.count(temp_bb)) return val_map[temp_bb];
    }
    return -1;
}

int RangeAnalysis::cond_result(ValueRange range, BinaryOp op, Temp *val, ir::BasicBlock *bb) {
    std::unordered_set<std::pair<Temp *, ir::BasicBlock *>> visited, worklist;
    visited = range.get_temps(op);
    if (op == BinaryOp::Neq) {
        for (auto [v, b]: visited) {
            if (v == val) return 1;
        }
        return -1;
    }
    auto val_bb = std::make_pair(val, bb);
    for (auto [v, b]: visited) {
        if (v == val) return 1;
    }
    // worklist = visited;
    // while (!worklist.empty()) {
    //     auto tmp = *worklist.begin();
    //     worklist.erase(worklist.begin());
    //     auto &[next_temp, next_bb] = tmp;
    //     std::unordered_set<std::pair<Temp *, ir::BasicBlock *>> visited_temp = get_bound(next_temp, next_bb, bb).get_temps(op);
    //     for (auto [v, b]: visited_temp) {
    //         if (v == val) return 1;
    //     }
    //     for (auto t: visited) {
    //         if (!visited.count(t)) {
    //             visited.insert(t);
    //             worklist.insert(t);
    //         }
    //     }
    // }
    switch (op)
    {
    case BinaryOp::Lt:
        op = BinaryOp::Geq;
        break;
    case BinaryOp::Gt:
        op = BinaryOp::Leq;
        break;
    case BinaryOp::Leq:
        op = BinaryOp::Gt;
        break;
    case BinaryOp::Geq:
        op = BinaryOp::Lt;
        break;
    default:
        assert(false);
    }
    visited = range.get_temps(op);
    for (auto [v, b]: visited) {
        if (v == val) return 0;
    }
    // worklist = visited;
    // while (!worklist.empty()) {
    //     auto tmp = *worklist.begin();
    //     worklist.erase(worklist.begin());
    //     auto &[next_temp, next_bb] = tmp;
    //     std::unordered_set<std::pair<Temp *, ir::BasicBlock *>> visited_temp = get_bound(next_temp, next_bb, bb).get_temps(op);
    //     for (auto [v, b]: visited_temp) {
    //         if (v == val) return 0;
    //     }
    //     for (auto t: visited) {
    //         if (!visited.count(t)) {
    //             visited.insert(t);
    //             worklist.insert(t);
    //         }
    //     }
    // }
    return -1;
}

void RangeAnalysis::run() {
    all_changed = false;
    cfg_ = new CFG(func_);
    dt_ = new DominatorTree_(cfg_);
    ud_ = new UseDefAnalysis(cfg_);
    constant_map.clear();
    from_bb_range.clear();
    in_bb_range.clear();
    val_map.clear();
    should_split_bbs.clear();
    for (auto bb: *func_->get_basic_blocks()) {
        for (auto inst: *bb->get_instructions()) {
            TypeCase(li, ir::instruction::LoadImm4 *, inst) {
                if (li->getimm().type == Int) {
                    constant_map[li->getdst()] = li->getimm().iv;
                    unite(li->getdst(), bb, BinaryOp::Leq, li->getdst(), bb);
                    unite(li->getdst(), bb, BinaryOp::Geq, li->getdst(), bb);
                }
            }
            inst->set_parent(bb);
        }
    }
    changed = true;
    while (changed) {
        changed = false;
        // 记录每个temp在所在bb的ValueRange: cond
        for (auto &bb: *func_->get_basic_blocks()) { // bb: 当前bb
            for (auto prev_bb_idx: *cfg_->get_bb_prev(bb->get_index())) {
                auto prev_bb = cfg_->get_bb(prev_bb_idx); // prev_bb: 前一个bb
                auto end_inst = prev_bb->get_instructions()->back();
                TypeCase(cond, ir::instruction::CondBranch*, end_inst) { // cond: prev_bb的分支指令
                    if (cond->get_true_bb() == bb) { // prev_bb的true分支是当前bb
                        if (cfg_->get_bb_prev(bb->get_index())->size() == 1) {
                            update_val(bb, cond->getcond(), true);
                        }
                        if (func_->is_param(cond->getcond())) continue;
                        TypeCase(binary, ir::instruction::Binary*, *ud_->get_defset(cond->getcond()).begin()) { // 只记录比较指令，在这个bb里的lhs和rhs都符合比较指令
                            // std::cout << "unite: " << binary->getlhs()->to_str() << " in " << bb->get_name() << " with " << binary->getrhs()->to_str() << " in " << prev_bb->get_name() << std::endl;
                            switch (binary->get_type()) {
                                case BinaryOp::Lt:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Lt, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Gt, binary->getlhs(), prev_bb);
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Neq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Neq, binary->getlhs(), prev_bb);
                                    break;
                                case BinaryOp::Gt:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Gt, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Lt, binary->getlhs(), prev_bb);
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Neq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Neq, binary->getlhs(), prev_bb);
                                    break;
                                case BinaryOp::Leq:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Leq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Geq, binary->getlhs(), prev_bb);
                                    break;
                                case BinaryOp::Geq:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Geq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Leq, binary->getlhs(), prev_bb);
                                    break;
                                case BinaryOp::Neq:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Neq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Neq, binary->getlhs(), prev_bb);
                                    break;
                                case BinaryOp::Eq:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Leq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Leq, binary->getlhs(), prev_bb);
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Geq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Geq, binary->getlhs(), prev_bb);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    else if (cond->get_false_bb() == bb) {
                        if (cfg_->get_bb_prev(bb->get_index())->size() == 1) {
                            update_val(bb, cond->getcond(), false);
                        }
                        if (func_->is_param(cond->getcond())) continue;
                        TypeCase(binary, ir::instruction::Binary*, *ud_->get_defset(cond->getcond()).begin()) { // 只记录比较指令，在这个bb里的lhs和rhs都不符合比较指令
                            switch (binary->get_type()) {
                                case BinaryOp::Lt:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Geq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Leq, binary->getlhs(), prev_bb);
                                    break;
                                case BinaryOp::Gt:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Leq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Geq, binary->getlhs(), prev_bb);
                                    break;
                                    break;
                                case BinaryOp::Leq:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Gt, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Lt, binary->getlhs(), prev_bb);
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Neq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Neq, binary->getlhs(), prev_bb);
                                    break;
                                case BinaryOp::Geq:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Lt, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Gt, binary->getlhs(), prev_bb);
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Neq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Neq, binary->getlhs(), prev_bb);
                                    break;
                                case BinaryOp::Neq:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Geq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Leq, binary->getlhs(), prev_bb);
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Leq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Geq, binary->getlhs(), prev_bb);
                                    break;
                                case BinaryOp::Eq:
                                    insert_from_bb_range(binary->getlhs(), bb, BinaryOp::Neq, binary->getrhs(), prev_bb);
                                    insert_from_bb_range(binary->getrhs(), bb, BinaryOp::Neq, binary->getlhs(), prev_bb);
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                }
            }
        }
        // 记录每个temp在所在bb的ValueRange: phi
        for (auto &bb: *func_->get_basic_blocks()) { // bb: phi所在bb
            for (auto inst: *bb->get_instructions()) { // inst: 当前指令
                TypeCase(phi, ir::instruction::Phi*, inst) { // phi: 当前指令
                    // std::cout << "checking phi: " << phi->to_str() << std::endl;
                    bool undefined = false;
                    std::unordered_set<Temp*> src_temps;
                    for (auto &[src, prev_bb]: phi->getpairs()) {
                        if (src_temps.count(src)) {
                            // TODO: 这里应该分裂phi节点，因为phi节点的来源可能不止一个，如果来源不同，则需要分裂
                            std::cout << "should split: " << phi->to_str() << std::endl;
                            undefined = true;
                            break;
                        }
                        else src_temps.insert(src);
                        auto temp_prev_bb = std::make_tuple(src, prev_bb, bb);
                        if (!from_bb_range.count(temp_prev_bb) && !constant_map.count(src)) {
                            undefined = true;
                            break;
                        }
                    }
                    if (undefined) continue;
                    for (auto &[src, prev_bb]: phi->getpairs()) {
                        // std::cout << "unite: " << phi->getdst()->to_str() << " in " << bb->get_name() << " with " << src->to_str() << " in " << bb->get_name() << std::endl;
                        auto temp_prev_bb = std::make_tuple(src, prev_bb, bb);
                        auto other_range = from_bb_range[temp_prev_bb];
                        unite(phi->getdst(), bb, other_range);
                    }
                }
            }
        }
    }
    // for (auto &[temp_bb, value_range]: in_bb_range) {
    //     auto &[temp, bb] = temp_bb;
    //     std::cout << "value range of " << temp->to_str() << " in " << bb->get_name() << " is:" << std::endl;
    //     std::cout << value_range.to_str() << std::endl;
    // }
    // for (auto &[temp_prev_bb, value_range]: from_bb_range) {
    //     auto &[temp, prev_bb, bb] = temp_prev_bb;
    //     std::cout << "value range of " << temp->to_str() << " from " << prev_bb->get_name() << " to " << bb->get_name() << " is:" << std::endl;
    //     std::cout << value_range.to_str() << std::endl;
    // }
    // 利用ValueRange判断每个分支指令
    int temp_idx = func_->get_temp_used();
    for (auto bb: *func_->get_basic_blocks()) { // cond的bb
        auto &end_inst = bb->get_instructions()->back();
        TypeCase(cond, ir::instruction::CondBranch*, end_inst) {
            int val_check = check_val(bb, cond->getcond());
            // std::cout << cond->to_str() << " check_val: " << val_check << std::endl;
            if (val_check != -1) {
                auto new_temp = new Temp(temp_idx++, Int);
                auto li = new ir::instruction::LoadImm4(new_temp, ConstValue(val_check));
                bb->add_instruction_before_terminal(li);
                ud_->compute_use_def(li);
                cond->change_use(cond->getcond(), new_temp);
                ud_->change_use(cond->getcond(), new_temp, cond);
            }
            else {
                if (func_->is_param(cond->getcond())) continue;
                TypeCase(binary, ir::instruction::Binary*, *ud_->get_defset(cond->getcond()).begin()) {
                    // std::cout << binary->to_str() << std::endl;
                    if (binary->get_type() != BinaryOp::Lt && binary->get_type() != BinaryOp::Gt && binary->get_type() != BinaryOp::Leq && binary->get_type() != BinaryOp::Geq && binary->get_type() != BinaryOp::Neq) continue;
                    ir::Instruction *l_def = nullptr;
                    ir::Instruction *r_def = nullptr;
                    if (!func_->is_param(binary->getlhs())) {
                        l_def = *ud_->get_defset(binary->getlhs()).begin();
                    }
                    if (!func_->is_param(binary->getrhs())) {
                        r_def = *ud_->get_defset(binary->getrhs()).begin();
                    }
                    TypeCase(phi, ir::instruction::Phi*, l_def) {
                        int result = -1;
                        std::unordered_set<int> results;
                        for (auto &pair: phi->getpairs()) {
                            auto &[src, prev_bb] = pair;
                            auto now_range = get_bound(src, prev_bb);
                            if (now_range.empty()) result = -1;
                            else result = cond_result(now_range, binary->get_type(), binary->getrhs(), binary->get_parent());
                            // std::cout << "now_range: " << now_range.to_str() << std::endl;
                            std::cout << "result of " << src->to_str() << " in " << phi->get_parent()->get_name() << " is " << result << std::endl;
                            results.insert(result);
                        }
                        if (results.size() > 1) {
                            std::cout << "should split: " << phi->to_str() << std::endl;
                            should_split_bbs.insert(bb);
                            all_changed = true;
                        }
                        if (results.size() == 1 && result != -1) {
                            std::cout << "now can remove!" << std::endl;
                            if (result == 1) {
                                update_phi_in_ra(cond->get_false_bb(), bb->get_index());
                                end_inst = new ir::instruction::Branch(cond->get_true_bb());
                            }
                            else if (result == 0) {
                                update_phi_in_ra(cond->get_true_bb(), bb->get_index());
                                end_inst = new ir::instruction::Branch(cond->get_false_bb());
                            }
                            delete cond;
                            all_changed = true;
                            cfg_->rebuild();
                        }
                    }
                    else TypeCase(phi, ir::instruction::Phi*, r_def) {
                        int result = -1;
                        std::unordered_set<int> results;
                        for (auto &pair: phi->getpairs()) {
                            auto &[src, prev_bb] = pair;
                            auto now_range = get_bound(src, prev_bb);
                            if (now_range.empty()) result = -1;
                            else result = cond_result(now_range, binary->get_swap_type(), binary->getlhs(), binary->get_parent());
                            // std::cout << "now_range: " << now_range.to_str() << std::endl;
                            std::cout << "result of " << src->to_str() << " in " << phi->get_parent()->get_name() << " is " << result << std::endl;
                            results.insert(result);
                        }
                        if (results.size() > 1) {
                            std::cout << "should split: " << phi->to_str() << std::endl;
                            should_split_bbs.insert(bb);
                            all_changed = true;
                        }
                        if (results.size() == 1 && result != -1) {
                            std::cout << "now can remove!" << std::endl;
                            if (result == 1) {
                                update_phi_in_ra(cond->get_false_bb(), bb->get_index());
                                end_inst = new ir::instruction::Branch(cond->get_true_bb());
                            }
                            else if (result == 0) {
                                update_phi_in_ra(cond->get_true_bb(), bb->get_index());
                                end_inst = new ir::instruction::Branch(cond->get_false_bb());
                            }
                            delete cond;
                            all_changed = true;
                            cfg_->rebuild();
                        }
                    }
                    // else {
                    //     auto now_range = get_bound(binary->getlhs(), bb, binary->get_parent());
                    //     std::cout << "now_range: " << now_range.to_str() << std::endl;
                    //     int result = -1;
                    //     if (!now_range.empty()) {
                    //         result = cond_result(now_range, binary->get_type(), binary->getrhs(), binary->get_parent());
                    //     }
                    //     std::cout << "result of " << binary->getlhs()->to_str() << " in " << bb->get_name() << " is " << result << std::endl;
                    //     if (result == 1) {
                    //         std::cout << "now can remove!" << std::endl;
                    //     }
                    //     else if (result == 0) {
                    //         std::cout << "now can remove!" << std::endl;
                    //     }
                    // }

    //                 int cond_check = check_cond(bb, binary->getlhs(), binary->get_type(), binary->getrhs());
    //                 // std::cout << "check_cond: " << cond_check << std::endl;
    //                 if (cond_check != -1) {
    //                     auto li = new ir::instruction::LoadImm4(binary->getdst(), ConstValue(cond_check));
    //                     ud_->erase_use_def(binary);
    //                     ud_->compute_use_def(li);
    //                     binary->get_parent()->add_instruction_after_inst(li, binary);
    //                     li->get_parent()->remove_instruction(binary);
    //                 }
                }
            }
        }
    }
    func_->set_temp_used(temp_idx);
    if (!should_split_bbs.empty()) {
        bool flag = false;
        for (auto bb: should_split_bbs) {
            LoopDoWhile ldw(func_);
            if (ldw.run(bb)) flag = true;
        }
        if (!flag) {
            all_changed = false;
        }
    }
    if (all_changed) {
        delete cfg_;
        delete ud_;
        delete dt_;
    }
}

}