#include "AnalysisPass.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Instruction.hpp"
#include "Value.hpp"
#include "AllUnrolling.hpp"

#include <codecvt>
#include <list>
#include <optional>

using namespace std;

std::optional <AllUnrolling::SimpleLoopInfo>
AllUnrolling::parse_simple_loop(const std::shared_ptr<Loop> loop) {
    SimpleLoopInfo ret;

    // parse header
    ret.header = loop->get_header();

    // parse bbs
    ret.bbs = loop->get_blocks();

    // parse preheader
    for (auto &pred : ret.header->get_pre_basic_blocks())
        if (std::find(ret.bbs.begin(), ret.bbs.end(), pred) == ret.bbs.end())
            ret.preheader = pred;

    // parse body
    ret.bodies.assign(ret.bbs.begin() + 1, ret.bbs.end());

    if (loop->get_sub_loops().size() > 0) {
        return nullopt;
    }

    if (loop->get_back_edges_nodes().size() > 1) {
        return nullopt;
    }

    // the loop has too many exiting edges
    if (loop->get_exits().size() > 1) {
        return nullopt;
    }

    // the exiting bb is not header
    if (loop->get_exits().begin()->first != ret.header) {
        return nullopt;
    }

    // parse exit
    ret.exit = loop->get_exits().begin()->second;

    // induction var
    auto br_inst = &ret.header->get_instructions().back();
    assert(br_inst->get_num_operand() == 3);
    auto cond = br_inst->get_operand(0);
    // the induction variable should be of int type
    if (not dynamic_cast<Instruction*>(cond)->is_cmp()){
        return nullopt;
    }

    auto icmp_inst = dynamic_cast<ICmpInst*>(cond);
    Instruction::OpID icmp_op = icmp_inst->get_instr_type();
    auto lhs = icmp_inst->get_operand(0);
    auto rhs = icmp_inst->get_operand(1);

    auto exit_cond = [&](bool is_ind_rhs) {
        Instruction::OpID opposite;
        if(icmp_op == Instruction::OpID::eq) opposite = Instruction::OpID::ne;
        else if (icmp_op == Instruction::OpID::ne) opposite = Instruction::OpID::eq;
        else if (icmp_op == Instruction::OpID::gt) opposite = Instruction::OpID::lt;
        else if (icmp_op == Instruction::OpID::lt) opposite = Instruction::OpID::gt;
        else if (icmp_op == Instruction::OpID::ge) opposite = Instruction::OpID::le;
        else if (icmp_op == Instruction::OpID::le) opposite = Instruction::OpID::ge;
        else assert(false && "Unreachable");
        auto op = is_ind_rhs ? opposite : icmp_op;
        if (br_inst->get_operand(1) == ret.exit) {
            // exit if cond is true
            return op;
        } else if (br_inst->get_operand(2) == ret.exit) {
            // exit if cond is false
            Instruction::OpID ret_op;
            if(op == Instruction::OpID::eq) ret_op = Instruction::OpID::ne;
            else if (op == Instruction::OpID::ne) ret_op = Instruction::OpID::eq;
            else if (op == Instruction::OpID::gt) ret_op = Instruction::OpID::le;
            else if (op == Instruction::OpID::lt) ret_op = Instruction::OpID::ge;
            else if (op == Instruction::OpID::ge) ret_op = Instruction::OpID::lt;
            else if (op == Instruction::OpID::le) ret_op = Instruction::OpID::gt;
            else assert(false && "Unreachable");
            return ret_op;
        } else {
            assert(false && "Unreachable");
        }
    };

    auto formal_cond = [&](bool is_ind_rhs) {
        Instruction::OpID opposite;
        if(icmp_op == Instruction::OpID::eq) opposite = Instruction::OpID::ne;
        else if (icmp_op == Instruction::OpID::ne) opposite = Instruction::OpID::eq;
        else if (icmp_op == Instruction::OpID::gt) opposite = Instruction::OpID::lt;
        else if (icmp_op == Instruction::OpID::lt) opposite = Instruction::OpID::gt;
        else if (icmp_op == Instruction::OpID::ge) opposite = Instruction::OpID::le;
        else if (icmp_op == Instruction::OpID::le) opposite = Instruction::OpID::ge;
        else assert(false && "unreachable");
        auto op = is_ind_rhs ? icmp_op : opposite;
        if (br_inst->get_operand(2) == ret.exit) {
            // exit if cond is true
            return op;
        } else if (br_inst->get_operand(1) == ret.exit) {
            // exit if cond is false
            Instruction::OpID ret_op;
            if(op == Instruction::OpID::eq) ret_op = Instruction::OpID::ne;
            else if (op == Instruction::OpID::ne) ret_op = Instruction::OpID::eq;
            else if (op == Instruction::OpID::gt) ret_op = Instruction::OpID::le;
            else if (op == Instruction::OpID::lt) ret_op = Instruction::OpID::ge;
            else if (op == Instruction::OpID::ge) ret_op = Instruction::OpID::lt;
            else if (op == Instruction::OpID::le) ret_op = Instruction::OpID::gt;
            else assert(false && "unreachable");
            return ret_op;
        } else {
            assert(false && "unreachable");
        }
    };

    // at least one op should be mutable after constant folding
    assert(dynamic_cast<ConstantInt*>(lhs)==nullptr or dynamic_cast<ConstantInt*>(rhs) == nullptr);
    if (dynamic_cast<ConstantInt*>(lhs) != nullptr){
        ret.static_unroll = true;
        ret.ind_var = rhs;
        ret.icmp_op = exit_cond(true);
        ret.bound = dynamic_cast<ConstantInt*>(lhs);
    } else if (dynamic_cast<ConstantInt*>(rhs) != nullptr) {
        ret.static_unroll = true;
        ret.ind_var = lhs;
        ret.icmp_op = exit_cond(false);
        ret.bound = dynamic_cast<ConstantInt*>(rhs);
    } else 
    {
        ret.static_unroll = false;
        if(dynamic_cast<Instruction*>(lhs) && dynamic_cast<Instruction*>(lhs)->is_phi())
        {
            ret.ind_var = lhs;
            ret.icmp_op = formal_cond(true);
            ret.bound_var = rhs;
        }
        else if(dynamic_cast<Instruction*>(rhs) && dynamic_cast<Instruction*>(rhs)->is_phi())
        {
            ret.ind_var = rhs;
            ret.icmp_op = formal_cond(false);
            ret.bound_var = lhs;
        }
        else return nullopt;
    }

    // find initial and step
    for (auto &&inst : ret.header->get_instructions()) {
        if (dynamic_cast<PhiInst *>(&inst)==nullptr) {
            break;
        }
        if (dynamic_cast<Value*>(&inst) != ret.ind_var) {
            continue;
        }
        // the phi for induction variable
        auto phi_inst = dynamic_cast<PhiInst *>(&inst);
        // the phi should has two source, one from preheader and the other from
        // loop body
        //assert(phi_inst->get_pairs().size() == 2);
        for (auto [value, source] : phi_inst->get_pairs()) {
            if (std::find(loop->get_blocks().begin(), loop->get_blocks().end(),
                          source) != loop->get_blocks().end()) {
                // step
                if (dynamic_cast<IBinaryInst *>(value) == nullptr) {
                    continue;
                }
                auto ibinary_inst = dynamic_cast<IBinaryInst *>(value);
                auto ibin_op = ibinary_inst->get_instr_op_name();
                if (ibin_op != "add") {
                    continue;
                }
                if (dynamic_cast<ConstantInt*>(ibinary_inst->get_operand(0)) and
                    ibinary_inst->get_operand(1) == ret.ind_var) {
                    ret.step = dynamic_cast<ConstantInt*>(ibinary_inst->get_operand(0));
                } else if (dynamic_cast<ConstantInt*>(ibinary_inst->get_operand(1)) and
                           ibinary_inst->get_operand(0) == ret.ind_var) {
                    ret.step = dynamic_cast<ConstantInt*>(ibinary_inst->get_operand(1));
                }
            } else {
                // initial
                if (dynamic_cast<ConstantInt *>(value) == nullptr) {
                    continue;
                }
                ret.initial = dynamic_cast<ConstantInt *>(value);
            }
        }
    }
    if (ret.initial == nullptr or ret.step == nullptr) {
        return nullopt;
    }

    return ret;
}

bool AllUnrolling::should_unroll(const SimpleLoopInfo &simple_loop) {
    int initial = simple_loop.initial->get_value();
    int step = simple_loop.step->get_value();
    int bound = simple_loop.bound->get_value();

    long long inst_cnt{0};
    for (auto bb : simple_loop.bbs) {
        inst_cnt += bb->get_instructions().size();
    }

    int estimate = (bound - initial) / step;

    return inst_cnt * estimate < UNROLL_MAX;
}

void AllUnrolling::static_unroll_simple_loop(const SimpleLoopInfo &simple_loop, Module *m_) {
    auto header = simple_loop.header;
    // topological sort
    vector<BasicBlock *> bodies_order;
    map<BasicBlock *, size_t> pre_cnt;
    for (auto bb : simple_loop.bodies) {
        if (std::find(bb->get_pre_basic_blocks().begin(), bb->get_pre_basic_blocks().end(), header)!=bb->get_pre_basic_blocks().end()) {
            pre_cnt[bb] = 0;
        } else {
            pre_cnt[bb] = bb->get_pre_basic_blocks().size();
        }
    }
    while (not pre_cnt.empty()) {
        bool changed{false};
        for (auto [bb, cnt] : pre_cnt) {
            if (cnt == 0) {
                bodies_order.push_back(bb);
                for (auto suc : bb->get_succ_basic_blocks()) {
                    if (suc != header) {
                        pre_cnt[suc]--;
                    }
                }
                pre_cnt.erase(bb);
                changed = true;
                break;
            }
        }
        assert(changed);
    }

    assert(bodies_order.size() == simple_loop.bodies.size());

    map<Value *, Value *> old2new, phi_dst2src;
    // set phi_var_map to initial value
    for (auto &inst : header->get_instructions()) {
        if (not dynamic_cast<PhiInst*>(&inst)) {
            break;
        }
        // the phi for induction variable
        auto phi_inst = dynamic_cast<PhiInst*>(&inst);
        // the phi should has two source, one from preheader and the other from
        // loop body
        assert(phi_inst->get_pairs().size() == 2);
        for (auto [value, source] : phi_inst->get_pairs()) {
            if (std::find(simple_loop.bbs.begin(), simple_loop.bbs.end(), source) != simple_loop.bbs.end()) {
                phi_dst2src.emplace(phi_inst, value);
            }
        }
        for (auto [value, source] : phi_inst->get_pairs()) {
            if (std::find(simple_loop.bbs.begin(), simple_loop.bbs.end(), source) == simple_loop.bbs.end()) {
                old2new.emplace(phi_dst2src[phi_inst], value);
            }
        }
    }

    auto should_exit = [&](int i) {
        int bound = simple_loop.bound->get_value();
        switch (simple_loop.icmp_op) {
        case ICmpInst::OpID::eq:
            return i == bound;
        case ICmpInst::OpID::ne:
            return i != bound;
        case ICmpInst::OpID::lt:
            return i < bound;
        case ICmpInst::OpID::le:
            return i <= bound;
        case ICmpInst::OpID::gt:
            return i > bound;
        case ICmpInst::OpID::ge:
            return i >= bound;
        default:
            assert(false && "Unreachable");
        }
    };

    auto func = header->get_parent();

    auto clone_bbs = [&]() {
        for (auto bb : bodies_order) {
            old2new[bb] = BasicBlock::create(m_, "", func);
            //func->add_basic_block(dynamic_cast<BasicBlock*>(old2new[bb]));
        }
        old2new[header] = BasicBlock::create(m_, "", func);
        //func->add_basic_block(dynamic_cast<BasicBlock*>(old2new[header]));
    };

    auto clone2bb = [&](BasicBlock *old_bb) {       
        auto new_bb = dynamic_cast<BasicBlock*>(old2new.at(old_bb));
        for (auto &inst : old_bb->get_instructions()) {
            if (old_bb == header) {
                if (dynamic_cast<PhiInst*>(&inst)) {
                    old2new[&inst] = old2new[phi_dst2src[&inst]];
                    continue;
                } else if (dynamic_cast<BranchInst*>(&inst)) {
                    continue;
                }
            }
            auto new_inst = Instruction::clone_inst(&inst, new_bb, false);

            new_inst->set_operand_for_each_if(
                [&](Value *op) -> pair<bool, Value *> {
                    if (old2new.find(op)!=old2new.end())
                        return {true, old2new[op]};
                    else
                        return {false, nullptr};
                });
            old2new[&inst] = new_inst;
        }
    };

    auto unroll_exit = [&]() { return dynamic_cast<BasicBlock*>(old2new[header]); };
    auto unroll_entry = [&]() {
        if (old2new.find(static_cast<Value *>(bodies_order.front()))!=old2new.end()) {
            return dynamic_cast<BasicBlock*>(old2new[bodies_order.front()]);
        } else {
            return dynamic_cast<BasicBlock*>(old2new[header]);
        }
    };
    
    old2new[header] = BasicBlock::create(m_, "", func);
    //func->add_basic_block(dynamic_cast<BasicBlock*>(old2new[header]));
    clone2bb(header);
    auto bbs_entry = unroll_entry();

    for (int i = simple_loop.initial->get_value(); not should_exit(i);
         i += simple_loop.step->get_value()) {
        // connect last exit to current entry
        auto last_exit = unroll_exit();
        clone_bbs();
        auto entry = unroll_entry();
        BranchInst::create_br(entry, last_exit);

        // clone bbs and header
        for (auto bb : bodies_order) {
            clone2bb(bb);
        }
        clone2bb(header);
    }

    for (auto [old_inst, new_inst] : old2new) {
        if (not dynamic_cast<Instruction*>(old_inst)) {
            continue;
        }
        old_inst->replace_all_use_with(new_inst);
    }

    // connect exit
    header->erase_instr(dynamic_cast<BranchInst*>(&header->get_instructions().back()));
    BranchInst::create_br(simple_loop.exit, unroll_exit());

    // connect preheader
    simple_loop.preheader->get_instructions().back().replace_operand(header, bbs_entry);
    //auto bbs = func->get_basic_blocks().size();
    // remove old bbs
    for (auto it = func->get_basic_blocks().begin(); it != func->get_basic_blocks().end(); ++it) {
        if (std::find(simple_loop.bbs.begin(), simple_loop.bbs.end(), &*it) == simple_loop.bbs.end()) {
            auto bb = dynamic_cast<BasicBlock*>(&*it);
            if(bb->get_instructions().empty()) continue;
            auto br_inst = dynamic_cast<BranchInst*>(&*bb->get_instructions().rbegin());
            if(!br_inst) continue;
            else
            {
                if(br_inst->get_num_operand() == 1)
                {
                    auto succ_bb = dynamic_cast<BasicBlock*>(br_inst->get_operand(0));
                    if(std::find(bb->get_succ_basic_blocks().begin(), bb->get_succ_basic_blocks().end(), succ_bb) == bb->get_succ_basic_blocks().end())
                        bb->add_succ_basic_block(succ_bb);
                    if(std::find(succ_bb->get_pre_basic_blocks().begin(), succ_bb->get_pre_basic_blocks().end(), bb) == succ_bb->get_pre_basic_blocks().end())
                        succ_bb->add_pre_basic_block(bb);
                }
                else 
                {
                    auto succ_bb = dynamic_cast<BasicBlock*>(br_inst->get_operand(1));
                    if(std::find(bb->get_succ_basic_blocks().begin(), bb->get_succ_basic_blocks().end(), succ_bb) == bb->get_succ_basic_blocks().end())
                        bb->add_succ_basic_block(succ_bb);
                    if(std::find(succ_bb->get_pre_basic_blocks().begin(), succ_bb->get_pre_basic_blocks().end(), bb) == succ_bb->get_pre_basic_blocks().end())
                        succ_bb->add_pre_basic_block(bb);
                    succ_bb = dynamic_cast<BasicBlock*>(br_inst->get_operand(2));
                    if(std::find(bb->get_succ_basic_blocks().begin(), bb->get_succ_basic_blocks().end(), succ_bb) == bb->get_succ_basic_blocks().end())
                        bb->add_succ_basic_block(succ_bb);
                    if(std::find(succ_bb->get_pre_basic_blocks().begin(), succ_bb->get_pre_basic_blocks().end(), bb) == succ_bb->get_pre_basic_blocks().end())
                        succ_bb->add_pre_basic_block(bb);
                }
            }
        }
    }
    for (auto it : simple_loop.bbs)
    {
        auto bb = dynamic_cast<BasicBlock*>(&*it);
        for(auto pre: bb->get_pre_basic_blocks())
            pre->remove_succ_basic_block(bb);
        for (auto succ : bb->get_succ_basic_blocks())
            succ->remove_pre_basic_block(bb);
        func->get_basic_blocks().erase(bb);
    }
}

Value *AllUnrolling::find_map(
    Instruction *inst, int i, const SimpleLoopInfo &simple_loop,
    std::map<Instruction *, std::vector<Instruction *>> oldnew_inst) {
    //assert(inst->is_phi() && "Illegal instruction type");
    if(inst->is_phi())
    {
        PhiInst *phi_inst = dynamic_cast<PhiInst *>(inst);
        for (auto [value, source] : phi_inst->get_pairs())
            if (std::find(simple_loop.bbs.begin(), simple_loop.bbs.end(), source) !=
                simple_loop.bbs.end()) {
                if (auto inst_ = dynamic_cast<Instruction *>(value)) {
                    auto parent_bb = inst_->get_parent();
                    if (parent_bb == simple_loop.header && i > 1)
                        return find_map(inst_, i - 1, simple_loop, oldnew_inst);
                    else
                        return oldnew_inst[inst_].at(i - 1);
                } else
                    return value;
            }
        return nullptr;
    }
    else {
        return oldnew_inst[inst].back();
    }
}

void AllUnrolling::dynamic_unroll_simple_loop(const SimpleLoopInfo &simple_loop, Module *m_) {
    // 0.generate new header and bodies and copy instructions from old loop
    auto func = simple_loop.header->get_parent();
    std::map<BasicBlock *, std::vector<BasicBlock *>> oldnew_bb;
    std::map<Instruction *, std::vector<Instruction *>> oldnew_inst;

    // 0.1 generate new header and bodies, copy instructions
    BasicBlock *new_header = BasicBlock::create(m_, "", func);
    BasicBlock *bodies_entry;
    oldnew_bb[simple_loop.header].push_back(new_header);
    for (auto &inst : simple_loop.header->get_instructions()) {
        Instruction *new_inst =
            Instruction::clone_inst(&inst, new_header, false);
        oldnew_inst[&inst].push_back(new_inst);
    }
    std::vector<BasicBlock *> new_bodies;
    for (int i = 0; i < DYNAMIC_UNROLL_TIMES; i++) {
        for (auto bb = simple_loop.bodies.begin();
             bb != simple_loop.bodies.end(); bb++) {
            auto new_bb = BasicBlock::create(m_, "", func);
            new_bodies.push_back(new_bb);
            oldnew_bb[*bb].push_back(new_bb);
            for (auto &inst : (*bb)->get_instructions()) {
                Instruction *new_inst =
                    Instruction::clone_inst(&inst, new_bb, false);
                oldnew_inst[&inst].push_back(new_inst);
            }
        }
    }

    // 1.build the judge bb
    auto judge_bb = BasicBlock::create(m_, "", func);
    Value* bound_var;
    if(auto temp_inst = dynamic_cast<Instruction*>(simple_loop.bound_var))
    {
        if(temp_inst->get_parent() == simple_loop.header)
            bound_var = Instruction::clone_inst(temp_inst, judge_bb, false);
        else
            bound_var = simple_loop.bound_var;
    }
    else
        bound_var = simple_loop.bound_var;
    int initial = simple_loop.initial->get_value();
    int step = simple_loop.step->get_value();
    int num = initial + step * DYNAMIC_UNROLL_TIMES;
    if(simple_loop.icmp_op == ICmpInst::OpID::lt) num -= 1;
    if(simple_loop.icmp_op == ICmpInst::OpID::gt) num += 1;
    if(simple_loop.icmp_op == ICmpInst::OpID::le) num -= 2;
    if(simple_loop.icmp_op == ICmpInst::OpID::ge) num += 2;
    ConstantInt* judge_num = ConstantInt::get(num, m_);
    Instruction* judge_inst;
    switch(simple_loop.icmp_op)
    {
        case ICmpInst::OpID::eq: return;
        case ICmpInst::OpID::ne: return;
        case ICmpInst::OpID::lt: judge_inst = ICmpInst::create_lt(judge_num, bound_var, judge_bb); break;
        case ICmpInst::OpID::le: judge_inst = ICmpInst::create_le(judge_num, bound_var, judge_bb); break;
        case ICmpInst::OpID::gt: judge_inst = ICmpInst::create_gt(judge_num, bound_var, judge_bb); break;
        case ICmpInst::OpID::ge: judge_inst = ICmpInst::create_ge(judge_num, bound_var, judge_bb); break;
        default: break;
    }
    BranchInst::create_cond_br(judge_inst, new_header, simple_loop.header, judge_bb);

    // 2.modify the operands of the new loop
    // 2.1 modify the operands of the new header
    if(auto temp_inst = dynamic_cast<Instruction*>(simple_loop.bound_var))
    {
        if(temp_inst->get_parent() == simple_loop.header)
            bound_var = oldnew_inst[temp_inst].back();
        else
            bound_var = simple_loop.bound_var;
    }
    else
        bound_var = simple_loop.bound_var;
    for (auto &new_inst : new_header->get_instructions()) {
        for (unsigned i = 0; i < (&new_inst)->get_num_operand(); i++) {
            Value *op = (&new_inst)->get_operand(i);
            if (auto inst = dynamic_cast<Instruction *>(op)) {
                auto parent_bb = inst->get_parent();
                // the instrucion is in the header
                if (parent_bb == simple_loop.header && (&new_inst)->is_phi())
                    (&new_inst)->set_operand(i, find_map(inst, DYNAMIC_UNROLL_TIMES - 1,
                                                         simple_loop,
                                                         oldnew_inst));
                else if (std::find(simple_loop.bbs.begin(),
                                   simple_loop.bbs.end(),
                                   parent_bb) != simple_loop.bbs.end())
                    (&new_inst)->set_operand(i, oldnew_inst[inst].back());
            } else if (auto bb = dynamic_cast<BasicBlock *>(op)) {
                assert(((&new_inst)->is_phi() || (&new_inst)->is_br()) &&
                       "Illegal instruction type");
                if ((&new_inst)->is_phi()) {
                    if (std::find(simple_loop.bbs.begin(),
                                  simple_loop.bbs.end(),
                                  bb) != simple_loop.bbs.end())
                        (&new_inst)->set_operand(i, oldnew_bb[bb].back());
                    else if (bb == simple_loop.preheader)
                        (&new_inst)->set_operand(i, judge_bb);
                } else {
                    if (std::find(simple_loop.bbs.begin(),
                                  simple_loop.bbs.end(),
                                  bb) != simple_loop.bbs.end()) {
                        bodies_entry = bb;
                        (&new_inst)->set_operand(i, oldnew_bb[bb].front());
                    } else if (bb == simple_loop.exit)
                        (&new_inst)->set_operand(i, simple_loop.header);
                }
            }
            if (op == simple_loop.bound_var) {
                int step =
                    dynamic_cast<ConstantInt *>(simple_loop.step)->get_value();
                auto inst1 = IBinaryInst::create_sub(bound_var, simple_loop.initial, nullptr);
                inst1->insert_before(&new_inst);
                auto inst2 = IBinaryInst::create_srem(inst1, ConstantInt::get(step * DYNAMIC_UNROLL_TIMES, m_), nullptr);
                inst2->insert_before(&new_inst);
                auto inst3 = IBinaryInst::create_sub(bound_var, inst2, nullptr);
                inst3->insert_before(&new_inst);
                if((&new_inst)->get_instr_type() == Instruction::OpID::le)
                {
                    auto inst4 = IBinaryInst::create_sub(inst3, ConstantInt::get(1, m_), nullptr);
                    inst4->insert_before(&new_inst);
                    (&new_inst)->set_operand(i, inst4);
                }
                else if((&new_inst)->get_instr_type() == Instruction::OpID::ge)
                {
                    auto inst4 = IBinaryInst::create_add(inst3, ConstantInt::get(1, m_), nullptr);
                    inst4->insert_before(&new_inst);
                    (&new_inst)->set_operand(i, inst4);
                }
                else
                    (&new_inst)->set_operand(i, inst3);
            }
        }
    }

    // 2.2 modify the operands of bodies
    for (int i = 0; i < DYNAMIC_UNROLL_TIMES; i++) {
        for (auto bb = simple_loop.bodies.begin();
             bb != simple_loop.bodies.end(); bb++) {
            auto new_bb = oldnew_bb[*bb].at(i);
            for (auto &new_inst : new_bb->get_instructions()) {
                for (unsigned j = 0; j < (&new_inst)->get_num_operand(); j++) {
                    auto op = (&new_inst)->get_operand(j);
                    if (auto inst = dynamic_cast<Instruction *>(op)) {
                        auto parent_bb = inst->get_parent();
                        // the instrucion is in the header
                        if (parent_bb == simple_loop.header) {
                            if (i == 0)
                                (&new_inst)->set_operand(
                                    j, oldnew_inst[inst].back());
                            else
                                (&new_inst)->set_operand(
                                    j, find_map(inst, i, simple_loop,
                                                oldnew_inst));
                        } else if (std::find(simple_loop.bodies.begin(),
                                             simple_loop.bodies.end(),
                                             parent_bb) !=
                                   simple_loop.bodies.end())
                            (&new_inst)->set_operand(j,
                                                     oldnew_inst[inst].at(i));
                    } else if (auto bb_ = dynamic_cast<BasicBlock *>(op)) {
                        assert(
                            ((&new_inst)->is_phi() || (&new_inst)->is_br()) &&
                            "Illegal instruction type");
                        if ((&new_inst)->is_phi())
                            (&new_inst)->set_operand(j, oldnew_bb[bb_].at(i));
                        else {
                            if (bb_ != simple_loop.header)
                                (&new_inst)->set_operand(j,
                                                         oldnew_bb[bb_].at(i));
                            else {
                                if (i < DYNAMIC_UNROLL_TIMES - 1)
                                    (&new_inst)->set_operand(
                                        j, oldnew_bb[bodies_entry].at(i + 1));
                                else
                                    (&new_inst)->set_operand(j, new_header);
                            }
                        }
                    }
                }
            }
        }
    }
    // 3.modify the operands of the old loop and the preheader
    // 3.1 modify the operands of the preheader
    for (auto &inst : simple_loop.preheader->get_instructions())
        if ((&inst)->is_br()) {
            for (unsigned i = 0; i < (&inst)->get_num_operand(); i++) {
                auto op = (&inst)->get_operand(i);
                if (dynamic_cast<BasicBlock *>(op) == simple_loop.header)
                    (&inst)->set_operand(i, judge_bb);
            }
        }
    // 3.2 modify the operands of the old loop
    for (auto &inst : simple_loop.header->get_instructions())
        if ((&inst)->is_phi()) {
            auto new_inst = oldnew_inst[&inst].back();
            auto phi_inst = dynamic_cast<PhiInst *>(&inst);
            phi_inst->add_phi_pair_operand(new_inst, new_header);
            phi_inst->replace_operand(simple_loop.preheader, judge_bb);
        }

    // 4.maintain the relation of blocks
    // 4.1 modify the succ_bb of preheader
    simple_loop.preheader->remove_succ_basic_block(simple_loop.header);
    if (std::find(simple_loop.preheader->get_succ_basic_blocks().begin(),
                  simple_loop.preheader->get_succ_basic_blocks().end(),
                  judge_bb) ==
        simple_loop.preheader->get_succ_basic_blocks().end())
        simple_loop.preheader->add_succ_basic_block(judge_bb);
    // 4.2 modify the pre_bb and succ_bb of judge bb
    if (std::find(judge_bb->get_pre_basic_blocks().begin(),
                  judge_bb->get_pre_basic_blocks().end(),
                  simple_loop.preheader) ==
        judge_bb->get_pre_basic_blocks().end())
        judge_bb->add_pre_basic_block(simple_loop.preheader);
    if (std::find(judge_bb->get_succ_basic_blocks().begin(),
                  judge_bb->get_succ_basic_blocks().end(),
                  simple_loop.header) ==
        judge_bb->get_succ_basic_blocks().end())
        judge_bb->add_succ_basic_block(simple_loop.header);
    if (std::find(judge_bb->get_succ_basic_blocks().begin(),
                  judge_bb->get_succ_basic_blocks().end(),
                  new_header) ==
        judge_bb->get_succ_basic_blocks().end())
        judge_bb->add_succ_basic_block(new_header);
    // 4.3 modify the pre_bb and succ_bb of new_header
    for (auto &new_inst : new_header->get_instructions()) {
        if ((&new_inst)->is_phi())
            for (unsigned i = 1; i < (&new_inst)->get_num_operand(); i += 2) {
                auto bb =
                    dynamic_cast<BasicBlock *>((&new_inst)->get_operand(i));
                if (std::find(new_header->get_pre_basic_blocks().begin(),
                              new_header->get_pre_basic_blocks().end(),
                              bb) == new_header->get_pre_basic_blocks().end())
                    new_header->add_pre_basic_block(bb);
            }
        else
            break;
    }
    new_header->remove_pre_basic_block(simple_loop.preheader);
    BranchInst *br_inst =
        dynamic_cast<BranchInst *>(&new_header->get_instructions().back());
    if (br_inst) {
        auto succ_bbs = new_header->get_succ_basic_blocks();
        for (auto succ_bb : succ_bbs)
            if (std::find(br_inst->get_operands().begin(),
                          br_inst->get_operands().end(),
                          dynamic_cast<Value *>(succ_bb)) ==
                br_inst->get_operands().end()) {
                new_header->remove_succ_basic_block(succ_bb);
                succ_bb->remove_pre_basic_block(new_header);
            }
        for (unsigned i = 0; i < br_inst->get_num_operand(); i++) {
            auto bb = dynamic_cast<BasicBlock *>(br_inst->get_operand(i));
            if (bb) {
                if (std::find(new_header->get_succ_basic_blocks().begin(),
                              new_header->get_succ_basic_blocks().end(),
                              bb) == new_header->get_succ_basic_blocks().end())
                    new_header->add_succ_basic_block(bb);
                if (std::find(bb->get_pre_basic_blocks().begin(),
                              bb->get_pre_basic_blocks().end(),
                              new_header) == bb->get_pre_basic_blocks().end())
                    bb->add_pre_basic_block(new_header);
            }
        }
    }
    // 4.4 modify the pre_bb and succ_bb of new_bodies
    for (int i = 0; i < DYNAMIC_UNROLL_TIMES; i++) {
        for (auto bb = simple_loop.bodies.begin();
             bb != simple_loop.bodies.end(); bb++) {
            auto new_bb = oldnew_bb[*bb].at(i);
            BranchInst *br_inst =
                dynamic_cast<BranchInst *>(&new_bb->get_instructions().back());
            if (br_inst) {
                auto succ_bbs = new_bb->get_succ_basic_blocks();
                for (auto succ_bb : succ_bbs)
                    if (std::find(br_inst->get_operands().begin(),
                                  br_inst->get_operands().end(),
                                  succ_bb) == br_inst->get_operands().end()) {
                        new_bb->remove_succ_basic_block(succ_bb);
                        succ_bb->remove_pre_basic_block(new_bb);
                    }
            }
            for (unsigned i = 0; i < br_inst->get_num_operand(); i++) {
                auto bb_ = dynamic_cast<BasicBlock *>(br_inst->get_operand(i));
                if (bb_) {
                    if (std::find(new_bb->get_succ_basic_blocks().begin(),
                                  new_bb->get_succ_basic_blocks().end(),
                                  bb_) == new_bb->get_succ_basic_blocks().end())
                        new_bb->add_succ_basic_block(bb_);
                    if (std::find(bb_->get_pre_basic_blocks().begin(),
                                  bb_->get_pre_basic_blocks().end(),
                                  new_bb) == bb_->get_pre_basic_blocks().end())
                        bb_->add_pre_basic_block(new_bb);
                }
            }
        }
    }
    // 4.5 modify the pre_bb of header
    simple_loop.header->remove_pre_basic_block(simple_loop.preheader);
    //simple_loop.header->add_pre_basic_block(judge_bb);

    // 5 record the loops that have been unrolled
    has_been_unrolled.push_back(simple_loop.header);
    has_been_unrolled.push_back(new_header);
}

void AllUnrolling::run(Function *f, AnalysisPassManager &APM) {
    bool changed = true;
    while(changed)
    {
        changed = false;
        auto loopdetection_ = APM.getResult<LoopDetection>(f);
        for(auto &loop : loopdetection_->get_loops())
        {
            auto simple_loop = parse_simple_loop(loop);
            if (not simple_loop.has_value())
                continue;
            if(simple_loop.value().static_unroll)
            {
                if (not should_unroll(simple_loop.value()))
                    continue;
                static_unroll_simple_loop(simple_loop.value(), f->get_parent());
            }
            else
            {
                if(std::find(has_been_unrolled.begin(), has_been_unrolled.end(), simple_loop.value().header) != has_been_unrolled.end())
                    continue;
                dynamic_unroll_simple_loop(simple_loop.value(), f->get_parent());
            }
            APM.invalidateAll<Function>(f);
            changed = true;
            break;
        }
    }
}