#include "middleend/osr.hpp"

namespace middleend {

bool OSR::check_in_loop(ir::BasicBlock *bb, ir::BasicBlock *header) {
    auto now_loop = loop_analysis->get_loop(bb);
    // std::cout << "check_in_loop: " << header->get_name() << " " << bb->get_name() << std::endl;
    // std::cout << "header: " << header->get_name() << " " << "now_loop: " << (now_loop ? now_loop->header_->get_name() : "nullptr") << std::endl;
    if (now_loop == nullptr) return false;
    if (now_loop->header_->get_index() == header->get_index()) return true;
    return false;
}

Temp *OSR::apply(BinaryOp op, ir::Instruction *o1, ir::Instruction *o2, Type type) {
    // std::cout << "apply: " << o1->get_dst()->at(0)->to_str() << " " << (o2 ? o2->get_dst()->at(0)->to_str() : "nullptr") << std::endl;
    auto iter = std::find(key_list.begin(), key_list.end(), std::make_tuple(op, o1, o2));
    if (iter != key_list.end()) {
        return hashTable[std::distance(key_list.begin(), iter)];
    }
    if (header[o1] != nullptr && isRC(o2, header[o1])) {
        return reduce(op, o1, o2, type);
    }
    if (o2 && header[o2] != nullptr && isRC(o1, header[o2])) {
        return reduce(op, o2, o1, type);
    }
    auto idx = func_->get_temp_used();
    Temp *result = new Temp(idx++, type);
    func_->set_temp_used(idx);
    key_list.push_back(std::make_tuple(op, o1, o2));
    hashTable.push_back(result);
    // std::cout << "result in apply: " << result->to_str() << std::endl;
    ir::Instruction *newDef = nullptr;
    // TODO: delete after array ssa
    TypeCase(la1, ir::instruction::LoadAddr*, o1) {
        o1->get_parent()->remove_instruction(o1);
        cfg->get_bb(0)->add_instruction_at(func_->get_arg_num(), o1);
    }
    TypeCase(la2, ir::instruction::LoadAddr*, o2) {
        o2->get_parent()->remove_instruction(o2);
        cfg->get_bb(0)->add_instruction_at(func_->get_arg_num(), o2);
    }
    TypeCase(li1, ir::instruction::LoadImm4*, o1) {
        TypeCase(li2, ir::instruction::LoadImm4*, o2) {
            auto b = new ir::instruction::Binary(op, result, li1->get_dst()->at(0), li2->get_dst()->at(0));
            newDef = new ir::instruction::LoadImm4(result, b->to_const(li1->getimm(), li2->getimm()));
            delete b;
            header[newDef] = nullptr;
            cfg->get_bb(0)->add_instruction_at(func_->get_arg_num(), newDef);
            ud->compute_use_def(newDef);
            all_insts.push(newDef);
            // std::cout << "end apply for li and li" << std::endl;
            return result;
        }
    }
    newDef = new ir::instruction::Binary(op, result, o1->get_dst()->at(0), o2->get_dst()->at(0));
    TypeCase(li, ir::instruction::LoadImm4*, o1) {
        o1->get_parent()->remove_instruction(o1);
        cfg->get_bb(0)->add_instruction_at(func_->get_arg_num(), o1);
        o2->get_parent()->add_instruction_before_terminal(newDef);
    }
    else TypeCase(li, ir::instruction::LoadImm4*, o2) {
        o2->get_parent()->remove_instruction(o2);
        cfg->get_bb(0)->add_instruction_at(func_->get_arg_num(), o2);
        o1->get_parent()->add_instruction_before_terminal(newDef);
    }
    else {
        if (dt->get_dom(o1->get_parent()).count(o2->get_parent())) {
            o1->get_parent()->add_instruction_before_terminal(newDef);
        }
        else if (dt->get_dom(o2->get_parent()).count(o1->get_parent())) {
            o2->get_parent()->add_instruction_before_terminal(newDef);
        }
    }
    header[newDef] = nullptr;
    ud->compute_use_def(newDef);
    all_insts.push(newDef);
    // std::cout << "end apply" << std::endl;
    return result;
}

ir::Instruction *OSR::Clone(ir::Instruction *inst, Temp *new_temp) {
    TypeCase(phi, ir::instruction::Phi*, inst) {
        auto newPhi = new ir::instruction::Phi(new_temp, *phi->getvalues(), *phi->getbbs());
        newPhi->set_parent(phi->get_parent());
        return newPhi;
    }
    else TypeCase(binary, ir::instruction::Binary*, inst) {
        auto newBinary = new ir::instruction::Binary(binary->get_type(), new_temp, binary->getlhs(), binary->getrhs());
        newBinary->set_parent(binary->get_parent());
        return newBinary;
    }
    assert(false);
}

Temp *OSR::reduce(BinaryOp op, ir::Instruction *iv, ir::Instruction *rc, Type type) {
    // std::cout << "reduce: " << " " << iv->get_dst()->at(0)->to_str() << " " << rc->get_dst()->at(0)->to_str() << std::endl;
    auto iter = std::find(key_list.begin(), key_list.end(), std::make_tuple(op, iv, rc));
    if (iter != key_list.end()) {
        // std::cout << "end reduce and return directly" << std::endl;
        return hashTable[std::distance(key_list.begin(), iter)];
    }
    auto idx = func_->get_temp_used();
    Temp *result = new Temp(idx++, type);
    func_->set_temp_used(idx);
    key_list.push_back(std::make_tuple(op, iv, rc));
    hashTable.push_back(result);
    auto newDef = Clone(iv, result);
    header[newDef] = header[iv];
    edges[iv].push_back(edge(op, rc->get_dst()->at(0), newDef));
    for (auto &o: *newDef->get_src()) {
        // std::cout << "check newDef src: " << o->to_str() << std::endl;
        auto def_inst = *(ud->get_defset(o).begin());
        if (header[def_inst] == header[iv]) {
            auto new_temp = reduce(op, def_inst, rc, type);
            // std::cout << "change " << o->to_str() << " to " << new_temp->to_str() << " in " << newDef->get_dst()->at(0)->to_str() << std::endl;
            ud->change_use(o, new_temp, newDef);
            o = new_temp;
        }
        else TypeCase(phi, ir::instruction::Phi*, newDef) { // update initial value
            auto new_temp = apply(op, def_inst, rc, type);
            // std::cout << "change " << o->to_str() << " to " << new_temp->to_str() << " in " << newDef->get_dst()->at(0)->to_str() << std::endl;
            ud->change_use(o, new_temp, newDef);
            o = new_temp;
        }
        else if (op == BinaryOp::Mul) {
            // iv * rc
            auto new_temp = apply(op, def_inst, rc, type);
            // std::cout << "change " << o->to_str() << " to " << new_temp->to_str() << " in " << newDef->get_dst()->at(0)->to_str() << std::endl;
            ud->change_use(o, new_temp, newDef);
            o = new_temp;
        }
    }
    newDef->get_parent()->add_instruction_after_inst(newDef, iv);
    ud->compute_use_def(newDef);
    all_insts.push(newDef);
    // std::cout << "end reduce" << std::endl;
    return result;
}

void OSR::replace(ir::instruction::Binary *n, ir::Instruction *iv, ir::Instruction *rc) {
    // std::cout << "replace: " << n->get_dst()->at(0)->to_str() << " " << iv->get_dst()->at(0)->to_str() << " " << rc->get_dst()->at(0)->to_str() << std::endl;
    auto result = reduce(n->get_type(), iv, rc, n->get_dst()->at(0)->get_type());

    // std::cout << "result in replace: " << result->to_str() << std::endl;

    copy_propagation(ud, n->get_dst()->at(0), result);

    header[n] = header[iv];
    // std::cout << "end replace" << std::endl;
}

bool OSR::valid_update4iv(ir::Instruction *inst, std::unordered_set<ir::Instruction*> SCC, ir::BasicBlock *head) {
    TypeCase(phi, ir::instruction::Phi*, inst) {}
    else TypeCase(binary, ir::instruction::Binary*, inst) {
        if (binary->get_type() != BinaryOp::Add && binary->get_type() != BinaryOp::Sub)
            return false;
    }
    else return false;

    for (auto o: *inst->get_src()) {
        auto def_inst = *(ud->get_defset(o).begin());
        if (!SCC.count(def_inst) && !isRC(def_inst, head)) {
            return false;
        }
    }
    return true;
}

void OSR::classifyIV(std::unordered_set<ir::Instruction*> SCC) {
    auto lowest_rpo = (*(SCC.begin()))->get_parent();
    // std::cout << "lowest_rpo: " << lowest_rpo->get_name() << std::endl;
    for (auto inst: SCC) {
        if (rpo->get_index(lowest_rpo->get_index()) > rpo->get_index(inst->get_parent()->get_index())) {
            lowest_rpo = inst->get_parent();
        }
    }
    // std::cout << "lowest_rpo: " << lowest_rpo->get_name() << std::endl;
    bool is_iv = true;
    for (auto inst: SCC) {
        if (!valid_update4iv(inst, SCC, lowest_rpo)) {
            // std::cout << "not valid update for " << inst->get_dst()->at(0)->to_str() << std::endl;
            is_iv = false;
            break;
        }
    }
    if (is_iv) {
        // std::cout << "is iv" << std::endl;
        for (auto inst: SCC) {
            header[inst] = lowest_rpo;
        }
    }
    else {
        // std::cout << "not iv" << std::endl;
        for (auto inst: SCC) {
            if (!candidate_replace(inst)) header[inst] = nullptr;
        }
    }
}

bool OSR::candidate_replace(ir::Instruction *inst) {
    TypeCase(binary, ir::instruction::Binary*, inst) {
        auto lhs_def = *(ud->get_defset(binary->getlhs()).begin());
        auto rhs_def = *(ud->get_defset(binary->getrhs()).begin());
        switch (binary->get_type()) {
            case BinaryOp::Add:
            case BinaryOp::Mul:
                // iv + rc or iv * rc
                if (header.count(lhs_def) && header[lhs_def] != nullptr && !binary->getlhs()->get_type().is_array()
                        && isRC(rhs_def, header[lhs_def])) {
                    if (!check_in_loop(inst->get_parent(), header[lhs_def])) return false;
                    // if (binary->get_parent()->get_index() >= 70) return false;
                    replace(binary, lhs_def, rhs_def);
                    return true;
                }
                // rc + iv or rc * iv
                if (header.count(rhs_def) && header[rhs_def] != nullptr && !binary->getrhs()->get_type().is_array()
                        && isRC(lhs_def, header[rhs_def])) {
                    if (!check_in_loop(inst->get_parent(), header[rhs_def])) return false;
                    // if (binary->get_parent()->get_index() >= 70) return false;
                    replace(binary, rhs_def, lhs_def);
                    return true;
                }
                break;
            case BinaryOp::Sub:
                // iv - rc
                if (header.count(lhs_def) && header[lhs_def] != nullptr && !binary->getlhs()->get_type().is_array()
                        && isRC(rhs_def, header[lhs_def])) {
                    if (!check_in_loop(inst->get_parent(), header[lhs_def])) return false;
                    // if (binary->get_parent()->get_index() >= 70) return false;
                    replace(binary, lhs_def, rhs_def);
                    return true;
                }
                break;
            default:
                break;
        }
    }
    return false;
}

void OSR::process(std::unordered_set<ir::Instruction*> SCC) {
    if (SCC.size() == 1) {
        auto inst = *(SCC.begin());
        if (!candidate_replace(inst)) header[inst] = nullptr;
    }
    else classifyIV(SCC);
}

// Tarjan's algorithm for finding strongly connected components (SCCs)
void OSR::dfs(ir::Instruction* inst) {
    num[inst] = next_num++;
    visited.insert(inst);
    low[inst] = num[inst];
    inst_stack.push(inst);
    on_stack.insert(inst);
    for (auto o: *inst->get_src()) {
        if (o == nullptr) continue;
        auto def_inst = *(ud->get_defset(o).begin());
        if (!visited.count(def_inst)) {
            // if (def_inst->get_dst()->size() > 0 && inst->get_dst()->at(0) != nullptr) std::cout << "*****dfs(" << def_inst->get_dst()->at(0)->to_str() << ") in dfs" << std::endl;
            dfs(def_inst);
            low[inst] = std::min(low[inst], low[def_inst]);
        }
        if (num[def_inst] < num[inst] && on_stack.count(def_inst)) {
            low[inst] = std::min(low[inst], num[def_inst]);
        }
    }
    // if (inst->get_dst()->size() > 0 && inst->get_dst()->at(0) != nullptr) std::cout << inst->get_dst()->at(0)->to_str() << " low: " << low[inst] << " num: " << num[inst] << std::endl;
    if (low[inst] == num[inst]) {
        std::unordered_set<ir::Instruction*> SCC;
        ir::Instruction* x;
        do {
            x = inst_stack.top();
            inst_stack.pop();
            on_stack.erase(x);
            SCC.insert(x);
            // if (x->get_dst()->size() > 0 && inst->get_dst()->at(0) != nullptr) std::cout << "SCC: " << x->get_dst()->at(0)->to_str() << std::endl;
        } while(x != inst);
        process(SCC);
    }
    // if (inst->get_dst()->size() > 0 && inst->get_dst()->at(0) != nullptr) std::cout << "end dfs for " << inst->get_dst()->at(0)->to_str() << std::endl; else std::cout << "end dfs" << std::endl;
}

void OSR::run() {
    cfg = new CFG(func_);
    insert_params();
    dt = new DominatorTree_(cfg);
    ud = new UseDefAnalysis(cfg);
    rpo = new ReversePostOrder(cfg);
    loop_analysis = new LoopAnalysis(cfg);
    next_num = 0;
    for (auto bb: *func_->get_basic_blocks()) {
        for (auto inst: *bb->get_instructions()) {
            all_insts.push(inst);
            inst->set_parent(bb);
        }
    }
    while (!all_insts.empty()) {
        auto inst = all_insts.front();
        all_insts.pop();
        if (!visited.count(inst)) {
            // if (inst->get_dst()->size() > 0 && inst->get_dst()->at(0) != nullptr) std::cout << "***dfs(" << inst->get_dst()->at(0)->to_str() << ") in run" << std::endl;
            dfs(inst);
        }
    }
    for (auto bb: *func_->get_basic_blocks()) {
        for (auto inst: *bb->get_instructions()) {
            all_insts.push(inst);
            inst->set_parent(bb);
        }
    }
    // lftr();
    for (auto bb: *func_->get_basic_blocks()) {
        for (auto inst: *bb->get_instructions()) {
            all_insts.push(inst);
            inst->set_parent(bb);
        }
    }
    lftr();
    remove_params();
}

void OSR::insert_params() {
    for (int i = 0; i < func_->get_arg_num(); i++) {
        auto new_assign = new ir::instruction::Assign(func_->get_arg_temp()->at(i), func_->get_arg_temp()->at(i));
        cfg->get_bb(0)->add_instruction_front(new_assign);
        new_assign->set_parent(cfg->get_bb(0));
    }
}

void OSR::remove_params() {
    for (int i = 0; i < func_->get_arg_num(); i++) {
        cfg->get_bb(0)->remove_instruction(cfg->get_bb(0)->get_instructions()->front());
    }
}

void OSR::lftr() {
    while (!all_insts.empty()) {
        auto inst = all_insts.front();
        all_insts.pop();
        // find comparison instructions
        TypeCase(binary, ir::instruction::Binary*, inst) {
            BinaryOp op = binary->get_type();
            if (op == BinaryOp::Lt || op == BinaryOp::Leq || op == BinaryOp::Gt || op == BinaryOp::Geq || op == BinaryOp::Eq || op == BinaryOp::Neq) {
                auto lhs_def = *(ud->get_defset(binary->getlhs()).begin());
                auto rhs_def = *(ud->get_defset(binary->getrhs()).begin());
                auto dst_use = *(ud->get_useset(binary->getdst()).begin());
                TypeCase(condbranch, ir::instruction::CondBranch*, dst_use) {
                    auto true_bb = condbranch->get_true_bb();
                    // std::cout << condbranch->to_str() << rhs_def->get_dst()->at(0)->to_str() << std::endl;
                    if (header.count(lhs_def) && header[lhs_def] == true_bb) {
                        changeCompare(binary, lhs_def, rhs_def, op);
                    }
                    if (header.count(rhs_def) && header[rhs_def] == true_bb) {
                        switch(op) {
                            case BinaryOp::Lt:
                                op = BinaryOp::Gt;
                                break;
                            case BinaryOp::Leq:
                                op = BinaryOp::Geq;
                                break;
                            case BinaryOp::Gt:
                                op = BinaryOp::Lt;
                                break;
                            case BinaryOp::Geq:
                                op = BinaryOp::Leq;
                                break;
                            default:
                                break;
                        }
                        changeCompare(binary, rhs_def, lhs_def, op);
                    }
                }
            }
        }
    }
}

void OSR::changeCompare(ir::instruction::Binary *binary, ir::Instruction *iv, ir::Instruction *rc, BinaryOp op) {
    binary->settype(op);
    // find the final induction variable and get an appropriate new region constant
    auto serach_iv = iv;
    auto new_rc = rc;
    // std::cout << "change compare: " << binary->to_str() << std::endl;
    while(edges.count(serach_iv)) {
        auto e = edges[serach_iv].front();
        serach_iv = e.next_iv;
        // std::cout << "  now region constant: " << new_rc->to_str() << std::endl;
        // std::cout << "  another region constant: " << e.rc->to_str() << std::endl;
        auto next_type = e.rc->get_type().get_lower_dim();
        if (new_rc->get_dst()->front()->get_type().is_array()) next_type = new_rc->get_dst()->front()->get_type().get_lower_dim();
        Temp *new_tmp = apply(e.op, new_rc, *ud->get_defset(e.rc).begin(), next_type);
        new_rc = *ud->get_defset(new_tmp).begin();
    }
    // std::cout << "final induction variable: " << serach_iv->to_str() << " new region constant: " << new_rc->to_str() << std::endl;
    ud->change_use(binary->getlhs(), serach_iv->get_dst()->at(0), binary);
    ud->change_use(binary->getrhs(), new_rc->get_dst()->at(0), binary);
    binary->setlhs(serach_iv->get_dst()->at(0));
    binary->setrhs(new_rc->get_dst()->at(0));
    binary->get_parent()->add_instruction_before_terminal(binary);
    binary->get_parent()->remove_instruction(binary);
}

} // namespace middleend