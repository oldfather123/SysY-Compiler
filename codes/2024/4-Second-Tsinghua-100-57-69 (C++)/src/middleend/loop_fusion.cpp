#include "middleend/loop_fusion.hpp"

namespace middleend {

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

static std::unordered_set<ir::Instruction *> get_effective_use(UseDefAnalysis* uda, ir::Instruction * inst, ir::BasicBlock* bb) {
    std::unordered_set<ir::Instruction *> ret;
    std::unordered_set<ir::Instruction *> visited;
    std::stack<ir::Instruction *> stack;
    stack.push(inst);
    while(!stack.empty()){
        inst = stack.top();
        stack.pop();
        if(visited.count(inst) || inst->get_parent() != bb) {
            continue;
        }
        visited.insert(inst);
        if(inst->is_output_inst()){
            auto uses = uda->get_useset(*inst->get_dst()->begin());
            for(auto each: uses){
                stack.push(each);
            }
        }
        TypeCase(arraystore, ir::instruction::ArrayStore *, inst){
            ret.insert(arraystore);
        }
        TypeCase(call, ir::instruction::Call *, inst) {
            ret.insert(call);
        }
        TypeCase(arrayload, ir::instruction::ArrayLoad *, inst) {
            ret.insert(arrayload);
        }
    }
    return ret;
}

LoopCond LoopFusion::get_loop_cond(UseDefAnalysis *uda, Loop *loop, std::unordered_set<ir::BasicBlock *> loop_bb){
    LoopCond ret;
    ret.is_const_bound = false;
    if (loop_bb.size() != 1) {
        return ret;
    }
    std::unordered_set<Temp *> defs;
    ir::BasicBlock *header = loop->header_;
    for(auto inst: *header->get_instructions()){
        if(inst->is_output_inst())
            for(auto dst: *inst->get_dst()){
                defs.insert(dst);
            }
    }
    auto inst = header->get_instructions()->back();
    TypeCase(br, ir::instruction::CondBranch *, inst) {
        if(uda->get_defsets().find(br->getcond()) == uda->get_defsets().end()){
            return ret;
        }
        auto cond_inst_list = uda->get_defset(br->getcond());
        assert(cond_inst_list.size() == 1);
        auto cond_inst = *cond_inst_list.begin();
        TypeCase(bi, ir::instruction::Binary *, cond_inst) {
            if(!is_cmp(bi)) {
                return ret;
            }
            // one of lhs or rhs must not change in the block
            if(defs.count(bi->getlhs()) == defs.count(bi->getrhs())){
                return ret;
            }
            ret.cmp = bi->get_type();
            Temp* var;
            if(defs.count(bi->getlhs())) {
                var = bi->getlhs();
                ret.end = bi->getrhs();
            } else if(defs.count(bi->getrhs())) {
                var = bi->getrhs();
                ret.end = bi->getlhs();
            }
            auto update_inst_list = uda->get_defset(var);
            assert(update_inst_list.size() == 1);
            auto update_inst = *update_inst_list.begin();
            TypeCase(update, ir::instruction::Binary *, update_inst) {
                ret.update = update->get_type();
                if(defs.count(update->getlhs()) == defs.count(update->getrhs())){
                    return ret;
                }
                Temp* phi_temp;
                if(defs.count(update->getlhs())){
                    phi_temp = update->getlhs();
                    ret.step = update->getrhs();
                } else {
                    phi_temp = update->getrhs();
                    ret.step = update->getlhs();
                }
                auto phi_inst_list = uda->get_defset(phi_temp);
                assert(phi_inst_list.size() == 1);
                auto phi_inst  = *phi_inst_list.begin();
                Temp* start;
                bool set_start = false;
                TypeCase(phi, ir::instruction::Phi *, phi_inst) {
                    ret.var = phi->getdst();
                    for(auto p: phi->getpairs()){
                        if(p.second != header) {
                            if(set_start) {
                                if(start != p.first) {
                                    return ret;
                                }
                            } else {
                                set_start = true;
                                start = p.first;
                            }
                        }
                    }
                    ret.start = start;
                    ret.is_const_bound = true;
                } else {
                    return ret;
                }
            } else {
                return ret;
            }
        } else {
            return ret;
        }
    }
    return ret;
}

void LoopFusion::fuse_loops(CFG* cfg, DominatorTree_* dom, UseDefAnalysis *uda, Loop *to, Loop *from, LoopCond to_cond, LoopCond from_cond) {
    auto b1 = to->header_;
    auto b2 = from->header_;
    auto mid_bb = dom->get_idom(b2);
    std::unordered_map<Temp*, Temp*> phi_map;
    for(auto inst: *mid_bb->get_instructions()) {
        TypeCase(phi, ir::instruction::Phi *, inst) {
            phi_map[phi->getdst()] = phi->get_src_temp(b1);
        }
    }
    copy_propagation(uda, from_cond.var, to_cond.var);
    std::unordered_map<Temp*, Temp*> reorder_map;
    ir::BasicBlock *succ;
    for(auto each_id: *cfg->get_bb_succ(b2->get_index())){
        auto each = cfg->get_bb(each_id);
        if(each == b2) continue;
        succ = each;
    }
    for(auto inst: *b2->get_instructions()){
        inst->set_parent(b1);
        auto uses = inst->get_src();
        for(auto each: *uses){
            if(*each == *from_cond.var) {
                inst->change_use(each, to_cond.var);
                uda->change_use(each, to_cond.var, inst);
            }
        }
        TypeCase(phi, ir::instruction::Phi *, inst) {
            if(!phi_map.count(phi->get_src_temp(mid_bb))) {
                continue;
            }
            auto raw = phi_map.at(phi->get_src_temp(mid_bb));
            auto dst = phi->getdst();
            auto use_list = uda->get_useset(dst);
            for(auto each: use_list){
                if(each->get_parent() == b2) {
                    each->change_use(dst, raw);
                    uda->change_use(dst, raw, each);
                }
            }
            reorder_map[raw] = phi->get_src_temp(b2);
        }
    }
    for(auto inst: *b1->get_instructions()){
        TypeCase(phi, ir::instruction::Phi*, inst) {
            auto uses = phi->get_src();
            for(auto each: *uses) {
                if(reorder_map.count(each)) {
                    inst->change_use(each, reorder_map.at(each));
                    uda->change_use(each, reorder_map.at(each), inst);
                }
            }
        } else {
            break;
        }
    }
    auto iter = mid_bb->get_instructions()->begin();
    auto idom_prev = dom->get_idom(b1);
    std::unordered_set<Temp *> phi_defs;
    for(; iter != mid_bb->get_instructions()->end();) {
        TypeCase(phi, ir::instruction::Phi *, *iter) {
            auto uses = phi->get_src();
            for(auto each: *uses) {
                if(reorder_map.count(each)) {
                    phi->change_use(each, reorder_map.at(each));
                    uda->change_use(each, reorder_map.at(each), phi);
                }
            }
            phi_defs.insert(phi->getdst());
            iter++;
        } else {
            TypeCase(ret, ir::instruction::Return *, *iter) {
                break;
            }
            TypeCase(jp, ir::instruction::Branch *, *iter) {
                break;
            }
            TypeCase(br, ir::instruction::CondBranch *, *iter) {
                break;
            }
            bool move = true;
            auto uses = (*iter)->get_src();
            for(auto each: *uses) {
                if(uda->get_defsets().count(each) && (*uda->get_defset(each).begin())->get_parent() == mid_bb) {
                    move = false;
                }
            }
            if(!move) {
                iter++;
                continue;
            }
            auto inst = *iter;
            uda->erase_use_def(inst);
            inst->set_parent(idom_prev);
            idom_prev->add_instruction_before_terminal(inst);
            uda->compute_use_def(inst);
            iter = mid_bb->get_instructions()->erase(iter);
        }
    }
    iter = succ->get_instructions()->begin();
    for(; iter != succ->get_instructions()->end();) {
        TypeCase(phi, ir::instruction::Phi *, *iter) {
            auto mid_reg = phi->get_src_temp(mid_bb);
            if(uda->get_defsets().count(mid_reg) && (*uda->get_defset(mid_reg).begin())->get_parent() == mid_bb){
                copy_propagation(uda, phi->getdst(), mid_reg);
                uda->erase_use_def(*iter);
                iter = succ->get_instructions()->erase(iter);
                continue;
            }
            for(auto prev_id: *cfg->get_bb_prev(b1->get_index())){
                auto prev = cfg->get_bb(prev_id);
                phi->add_src_and_bb(prev, phi->get_src_temp(mid_bb));
            }
            phi->add_src_and_bb(b1, phi->get_src_temp(b2));
            phi->set_parent(mid_bb);
            phi->erase_src_temp(mid_bb);
            phi->erase_src_temp(b2);
            uda->erase_use_def(phi);
            uda->compute_use_def(phi);
        } else {
            break;
        }
        iter++;
    }
    if(iter != succ->get_instructions()->begin()){
        iter--;
        for(auto i = iter;; i--){
            mid_bb->add_instruction_front(*i);
            if(i == succ->get_instructions()->begin()){
                break;
            }
        }
        iter++;
        for(auto i = succ->get_instructions()->begin(); i != iter;){
            i = succ->get_instructions()->erase(i);
        }
    }
    iter = b2->get_instructions()->begin();
    for(; iter != b2->get_instructions()->end(); iter++){
        TypeCase(phi, ir::instruction::Phi*, *iter) {
            auto change_reg = phi->get_src_temp(b2);
            phi->erase_src_temp(b2);
            phi->add_src_and_bb(b1, change_reg);
            auto raw_reg = phi->get_src_temp(dom->get_idom(b2));
            phi->erase_src_temp(dom->get_idom(b2));
            for(auto each_id: *cfg->get_bb_prev(b1->get_index())){
                auto each = cfg->get_bb(each_id);
                if(each != b1){
                    phi->add_src_and_bb(each, raw_reg);
                }
            }
            uda->erase_use_def(phi);
            uda->compute_use_def(phi);
        } else {
            break;
        }
    }
    auto br = b1->get_instructions()->back();
    b1->get_instructions()->pop_back();
    if(iter != b2->get_instructions()->begin()){
        iter--;
        for(auto i = iter;; i--){
            b1->add_instruction_front(*i);
            if(i == b2->get_instructions()->begin()){
                break;
            }
        }
        iter++;
    }
    if(iter != b2->get_instructions()->end()){
        for(auto i = iter; i != b2->get_instructions()->end(); i++){
            b1->add_instruction(*i);
        }
    }
    b2->get_instructions()->clear();
    uda->erase_use_def(*b1->get_instructions()->end());
    b1->get_instructions()->pop_back();
    b1->add_instruction(br);
    cfg->change_succ(mid_bb, b2, succ);
    cfg->change_prev(succ, b2, mid_bb);
    for(auto iter = func_->get_basic_blocks()->begin(); iter != func_->get_basic_blocks()->end();) {
        if((*iter) == b2) {
            for(auto inst: *((*iter)->get_instructions())) {
                uda->erase_use_def(inst);
            }
            iter = func_->get_basic_blocks()->erase(iter);
        } else {
            iter++;
        }
    }
}

bool LoopFusion::check_common_var(UseDefAnalysis* uda, ir::BasicBlock* b1, ir::BasicBlock* b2, ir::BasicBlock* mid_bb) {
    std::unordered_map<Temp*, Temp*> common_map;
    std::unordered_map<Temp*, Temp*> used_map;
    std::unordered_set<Temp*> common_set;
    for(auto inst: *b1->get_instructions()){
        TypeCase(phi, ir::instruction::Phi*, inst) {
            common_map[phi->getdst()] =  phi->getdst();
            common_map[phi->get_src_temp(b1)] = phi->getdst();
        }
        TypeCase(arraydef, ir::instruction::ArrayStore *, inst) {
            auto effect_inst = get_effective_use(uda, arraydef, b1);
            for(auto each_use: effect_inst) {
                TypeCase(arrayload, ir::instruction::ArrayLoad *, each_use) {
                    if(arrayload->getaddr() != arraydef->getaddr() || arrayload->getdep() != arraydef->getdep()){
                        return false;
                    }
                } else TypeCase(use_arraydef, ir::instruction::ArrayStore *, each_use) {
                    if(arraydef != use_arraydef){
                        return false;
                    }
                } else {
                    return false;
                }
            }
        }
        TypeCase(call, ir::instruction::Call *, inst) {
            return false;
        }
        if(inst->is_output_inst()){
            auto uses = inst->get_src();
            for(auto each: *uses){
                if(common_map.count(each)) {
                    used_map[*(inst->get_dst()->begin())] = common_map.at(each);
                }
            }
        }
    }
    for(auto inst: *mid_bb->get_instructions()){
       TypeCase(phi, ir::instruction::Phi*, inst) {
            common_map[phi->getdst()] =  common_map.at(phi->get_src_temp(b1));
            for(auto each: uda->get_useset(phi->getdst())){
                TypeCase(use_phi, ir::instruction::Phi*, each) {
                    continue;
                } else {
                    return false;
                }
            }
        }
        TypeCase(call, ir::instruction::Call *, inst) {
            return false;
        }
    }
    for(auto inst: *b2->get_instructions()){
        TypeCase(phi, ir::instruction::Phi*, inst) {
            if(common_map.count(phi->get_src_temp(mid_bb))){
                common_map[phi->getdst()] =  common_map.at(phi->get_src_temp(mid_bb));
                common_map[phi->get_src_temp(b2)] = common_map.at(phi->get_src_temp(mid_bb));
                common_set.insert(phi->get_src_temp(b2));
            }
        }
        TypeCase(arraydef, ir::instruction::ArrayStore *, inst) {
            auto effect_inst = get_effective_use(uda, arraydef, b2);
            for(auto each_use: effect_inst) {
                TypeCase(arrayload, ir::instruction::ArrayLoad *, each_use) {
                    if(arrayload->getaddr() != arraydef->getaddr() || arrayload->getdep() != arraydef->getdep()){
                        return false;
                    }
                } else TypeCase(use_arraydef, ir::instruction::ArrayStore *, each_use) {
                    if(arraydef != use_arraydef){
                        return false;
                    }
                } else {
                    return false;
                }
            }
        }
        TypeCase(call, ir::instruction::Call *, inst) {
            return false;
        }
        if(inst->is_output_inst()){
            auto uses = inst->get_src();
            for(auto each: *uses){
                if(common_map.count(each)) {
                    used_map[*(inst->get_dst()->begin())] = common_map.at(each);
                }
            }
        }
    }
    std::unordered_set<Temp *> common_b1_set;
    for(auto each: common_set) {
        // std::cout << each->to_str() << std::endl;
        TypeCase(phi, ir::instruction::Phi *, *(uda->get_defsets().at(common_map.at(each)).begin())){
            common_b1_set.insert(phi->get_src_temp(b1));
        } else {
            return false;
        }
    }
    for(auto e: common_b1_set){
        common_set.insert(e);
    }
    for(auto each: common_set) {
        auto inst = *(uda->get_defset(each).begin());
        TypeCase(arraydef, ir::instruction::ArrayStore *, inst){
            std::stack<Temp*> stack;
            stack.push(arraydef->getsrc());
            while(!stack.empty()){
                auto cur_reg = stack.top();
                stack.pop();
                auto def_pos = *uda->get_defset(cur_reg).begin();
                TypeCase(binary, ir::instruction::Binary*, def_pos) {
                    if(binary->get_type() == BinaryOp::Add || binary->get_type() == BinaryOp::Mul){
                        if(used_map.count(binary->getlhs()) != common_map.count(binary->getrhs())){
                            if(used_map.count(binary->getlhs())){
                                stack.push(binary->getlhs());
                            } else {
                                stack.push(binary->getrhs());
                            }
                        } else {
                            return false;
                        }
                    } else {
                        return false;
                    }
                } else TypeCase(arrayload, ir::instruction::ArrayLoad *, def_pos) {
                    if(arrayload->getaddr() != arraydef->getaddr()){
                        return false;
                    }
                } else {
                    return false;
                }
            }
        } else {
            return false;
        }
    }
    for(auto inst: *mid_bb->get_instructions()){
        bool is_phi = false;
        bool is_const = false;
        TypeCase(phi, ir::instruction::Phi *, inst) {
            common_map[phi->getdst()] = common_map.at(phi->get_src_temp(b1));
            for(auto each: uda->get_useset(phi->getdst())) {
                TypeCase(use_phi, ir::instruction::Phi*, each) {
                    is_phi = true;
                } else {
                    is_const = true;
                }
            }
            if(is_const){
                auto use_list = uda->get_useset(phi->getdst());
                for(auto each: use_list) {
                    if(each->get_parent() == b2) {
                        each->change_use(phi->getdst(), phi->get_src_temp(b1));
                        uda->change_use(phi->getdst(), phi->get_src_temp(b1), each);
                    }
                }
            }
        }
    }
    return true;
}

void LoopFusion::loop_fusion(ir::Function *func) {
    func_ = func;
    CFG *cfg = new CFG(func);
    LoopAnalysis *la = new LoopAnalysis(cfg);
    DominatorTree_ * dom_tree = new DominatorTree_(cfg);
    UseDefAnalysis *uda = new UseDefAnalysis(cfg);
    std::unordered_map<Loop*, std::unordered_set<ir::BasicBlock*>> loop_bbs;
    std::unordered_map<Loop*, LoopCond> loop_conds;
    for(auto loop: la->loops) {
        // std::cout << "loop:\n";
        // std::cout << loop->header_->get_name() << " " << loop->depth_ << std::endl;
        std::vector<ir::BasicBlock *> bb_stack;
        bb_stack.push_back(loop->header_);
        while(!bb_stack.empty()) {
            auto bb = bb_stack.back();
            bb_stack.pop_back();
            if(loop_bbs[loop].count(bb)){
                continue;
            }
            loop_bbs[loop].insert(bb);
            for(auto succ_id: *cfg->get_bb_succ(bb->get_index())){
                auto succ = cfg->get_bb(succ_id);
                if(la->get_loop(succ) == loop){
                    bb_stack.push_back(succ);
                }
            }
        }
    }
    for(auto loop: la->loops) {
        if(!loop->child_) {
            loop_conds[loop] = get_loop_cond(uda, loop, loop_bbs.at(loop));
            // if(loop_conds[loop].is_const_bound){
            //     std::cout << loop->header_->get_name() << std::endl;
            //     loop_conds[loop].print();
            // }
        }
    }
    bool has_fuse = false;
    for(int i = 1; i < la->loops.size(); i++){
        if(has_fuse) break;
        // either loops has inner loop
        if((la->loops[i - 1]->child_) || (la->loops[i]->child_)){
            continue;
        }
        auto cond_1 = loop_conds[la->loops[i]];
        auto cond_2 = loop_conds[la->loops[i - 1]];
        // either loops is not const bound
        if(!cond_1.is_const_bound || !cond_2.is_const_bound){
            continue;
        }
        // must have the same cond
        if(cond_1.cmp == cond_2.cmp && cond_1.end == cond_2.end && cond_1.start == cond_2.start && cond_1.step == cond_2.step && cond_1.update == cond_2.update){
            auto loop_1 = la->loops[i];
            auto loop_2 = la->loops[i - 1];
            // must share the same parent
            if(loop_1->parent_ != loop_2->parent_){
                continue;
            }
            auto mid_bb = dom_tree->get_idom(loop_2->header_);
            // must be adjacent
            if(!cfg->get_bb_succ(loop_1->header_->get_index())->count(mid_bb->get_index())){
                continue;
            }
            std::vector<Temp> mid_phi_defs;
            std::vector<Temp> loop_1_defs;
            std::vector<Temp> loop_2_uses;
            auto b1 = loop_1->header_;
            auto b2 = loop_2->header_;
            if(!check_common_var(uda, b1, b2, mid_bb)){
                continue;
            }
            // std::cout << "yeah!" << std::endl;
            fuse_loops(cfg, dom_tree, uda, loop_1, loop_2, cond_1, cond_2);
            has_fuse = true;
        }
    }
}

void LoopFusion::run() {
    for(auto func: *module_->get_functions()){
        // std::cout << "in " << func->get_name() << std::endl;
        loop_fusion(func);
        // for(auto bb: *func->get_basic_blocks()){
        //     std::cout << bb->get_name() << std::endl;
        //     for(auto inst: *bb->get_instructions()){
        //         inst->print(std::cout);
        //     }
        // }
        // std::cout << "over" << std::endl;
    }
}

}