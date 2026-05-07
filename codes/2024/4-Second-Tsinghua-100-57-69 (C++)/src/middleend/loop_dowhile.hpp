#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/reverse_postorder.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/loop_analysis.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

using ir::Instruction;

class LoopDoWhile {

private:
    std::unordered_map<int, ConstValue> constant_map; // temp -> constant value
    CFG *cfg_;
    ReversePostOrder *rpo_;
    DominatorTree_ *dt_;
    LoopAnalysis *la_;
    UseDefAnalysis *uda_;
    ir::BasicBlock *root;
    ir::Function *func;
    SimpleLoopInfo get_loop_info(Loop *loop, const std::unordered_set<ir::BasicBlock *> &loop_bbs, ir::BasicBlock *exit);
    void loop_dowhile(Loop *loop, SimpleLoopInfo info, const std::unordered_set<ir::BasicBlock *> &loop_bbs);
    ir::Instruction* copy_inst(SimpleLoopInfo info, ir::Instruction *inst, const std::unordered_map<BasicBlock *, BasicBlock *> &bb_map, const std::unordered_map<Temp*, Temp*> &reg_map);
    void copy_bb(SimpleLoopInfo info, BasicBlock *bb, BasicBlock *new_bb, const std::unordered_map<BasicBlock *, BasicBlock *> &bb_map, const std::unordered_map<Temp*, Temp*> &reg_map);

public:
    ~LoopDoWhile() {
        delete rpo_;
        delete dt_;
        delete la_;
        delete uda_;
    }

    LoopDoWhile(ir::Function *func_) : func(func_) {
        cfg_ = new CFG(func);
        rpo_ = new ReversePostOrder(cfg_);
        dt_ = new DominatorTree_(cfg_);
        la_ = new LoopAnalysis(cfg_);
        uda_ = new UseDefAnalysis(cfg_);
        func = cfg_->get_func();
        root = func->get_entry();
        return;
        for (auto loop: la_->loops) {
            for (auto &bb : *func->get_basic_blocks()) {
                for (auto &insn : *bb->get_instructions()) {
                    insn->set_parent(bb);
                }
                bb->set_parent(func);
            }
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
                // std::cout << "dead loop or multiple exit\n";
                return;
            }
            auto loop_info = get_loop_info(loop, loop_bbs, exit_bb);
            loop_info.exit = exit_bb;
            // std::cout << "loop_info.start: " << loop_info.start << " loop_info.end: " << loop_info.end << " loop_info.loop_type: " << loop_info.loop_type << "\n";
            if (loop_info.loop_type == 1 && loop_info.end >= 500 && loop_info.end - loop_info.start >= 2) {
                loop_dowhile(loop, loop_info, loop_bbs);
            }
        }
    }

    bool run(BasicBlock* bb);

};

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

SimpleLoopInfo LoopDoWhile::get_loop_info(Loop *loop, const std::unordered_set<ir::BasicBlock *> &loop_bbs, ir::BasicBlock *exit) {
    SimpleLoopInfo info(0, 0);
    // std::cout << "【header】"; loop->header_->print(std::cout);
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
    // std::cout << "【entry】" << entry->get_name() << "【exit_prev】" << exit_prev->get_name() << "\n";
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
    // std::cout << "【exit_prev】"; exit_prev->print(std::cout);
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
                    if (inst_phi->get_parent() != entry) return info;
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

ir::Instruction* LoopDoWhile::copy_inst(SimpleLoopInfo info, Instruction *inst, const std::unordered_map<BasicBlock *, BasicBlock *> &bb_map, const std::unordered_map<Temp*, Temp*> &reg_map) {
    auto reg_map_if = [=](Temp* reg) {
        if (reg_map.count(reg)) return reg_map.at(reg);
        else return reg;
    };
    auto bb_map_if = [=](ir::BasicBlock *bb) {
        if (bb_map.count(bb)) return bb_map.at(bb); 
        return bb;
    };
    TypeCase (binary, ir::instruction::Binary *, inst) {
        auto new_inst = new ir::instruction::Binary(binary->get_type(), reg_map_if(binary->getdst()), reg_map_if(binary->getlhs()), reg_map_if(binary->getrhs()));
        return new_inst;
    } else TypeCase (unary, ir::instruction::Unary *, inst) {
        auto new_inst = new ir::instruction::Unary(unary->get_type(), reg_map_if(unary->getdst()), reg_map_if(unary->getsrc()));
        return new_inst;
    } else TypeCase(load, ir::instruction::Load *, inst) {
        auto new_inst = new ir::instruction::Load(reg_map_if(load->getdst()), reg_map_if(load->getaddr()), load->not_delete(), load->getoffset());
        return new_inst;
    } else if (auto convert = dynamic_cast<ir::instruction::Assign *>(inst)) {
        auto new_inst = new ir::instruction::Assign(reg_map_if(convert->getdst()), reg_map_if(convert->getsrc()));
        return new_inst;
    } else TypeCase (loadaddr, ir::instruction::LoadAddr *, inst) {
        auto new_inst = new ir::instruction::LoadAddr(reg_map_if(loadaddr->get_addr()), loadaddr->get_name(), loadaddr->type_);
        return new_inst;
    } else TypeCase (loadimm, ir::instruction::LoadImm4 *, inst) {
        auto new_inst = new ir::instruction::LoadImm4(reg_map_if(loadimm->getdst()), loadimm->getimm());
        return new_inst;
    } else TypeCase (gep, ir::instruction::ElementPtr *, inst) {
        auto indices = gep->get_indices();
        for(auto &index : indices){
            index = reg_map_if(index);
        }
        auto new_inst = new ir::instruction::ElementPtr(reg_map_if(gep->get_dst()), reg_map_if(gep->get_base()), indices);
        return new_inst;
    } else TypeCase (call, ir::instruction::Call *, inst) {
        auto args = call->get_srcs();
        for(auto &arg : args){
            arg = reg_map_if(arg);
        }
        auto new_inst = new ir::instruction::Call(reg_map_if(call->getdst()), args, call->getfunc());
        return new_inst;
    } else TypeCase (store, ir::instruction::Store *, inst) {
        auto new_inst = new ir::instruction::Store(reg_map_if(store->getaddr()), reg_map_if(store->getsrc()), store->getoffset());
        return new_inst;
    } else TypeCase (br, ir::instruction::CondBranch *, inst) {
        auto new_inst = new ir::instruction::CondBranch(br->get_type(), bb_map_if(br->get_true_bb()), bb_map_if(br->get_false_bb()), reg_map_if(br->getcond()));
        return new_inst;
    } else TypeCase (jmp, ir::instruction::Branch *, inst) {
        auto new_inst = new ir::instruction::Branch(bb_map_if(jmp->get_bb()));
        return new_inst;
    } else TypeCase (ret, ir::instruction::Return *, inst) {
        if (ret->has_return_value()) {
            auto ret_val = reg_map_if(ret->getvalue());
            auto new_inst = new ir::instruction::Return(ret_val);
            return new_inst;
        }
        else {
            return new ir::instruction::Return();
        }
    } else TypeCase (phi, ir::instruction::Phi *, inst) {
        auto new_inst = new ir::instruction::Phi(reg_map_if(phi->getdst()), {}, {});
        if (inst->get_parent() == info.header) { // new entry
            for (auto pair : phi->getpairs()) {
                if (pair.second != info.into_entry) {
                    continue;                        
                }
                new_inst->add_pair(pair.second, pair.first);
            }
        }
        else {
            for (auto pair : phi->getpairs()) {
                auto mapped_bb = bb_map_if(pair.second);
                auto mapped_reg = reg_map_if(pair.first);
                new_inst->add_pair(mapped_bb, mapped_reg);
            }
        }
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
}

void LoopDoWhile::copy_bb(SimpleLoopInfo info, BasicBlock *bb, BasicBlock *new_bb,
                    const std::unordered_map<BasicBlock *, BasicBlock *> &bb_map, const std::unordered_map<Temp*, Temp*> &reg_map) {
    // map prev & succ
    // for (auto prev_bb : cfg_->prev(bb)) {
    //     cfg_->insert_prev(new_bb, bb_map.at(prev_bb));
    // }
    // for (auto succ_bb : cfg_->succ(bb)) {
    //     cfg_->insert_succ(new_bb, bb_map.at(succ_bb));
    // }
    // copy insts
    for (auto &insn : *bb->get_instructions()) {
        // std::cout << "【insn】"; insn->print(std::cout);
        auto new_inst = copy_inst(info, insn, bb_map, reg_map);
        // std::cout << "【new_inst】"; new_inst->print(std::cout);
        new_bb->add_instruction(new_inst);
        new_inst->set_parent(new_bb);
    }
    // std::cout << "【original bb】"; bb->print(std::cout);
    // std::cout << "【new_bb】"; new_bb->print(std::cout);
}

void LoopDoWhile::loop_dowhile(Loop *loop, SimpleLoopInfo info, const std::unordered_set<ir::BasicBlock *> &loop_bbs) {
    std::cout << "run loop dpwhile\n";
    BasicBlock *entry = info.header;
    BasicBlock *exit = info.exit;
    BasicBlock *into_entry = info.into_entry;
    BasicBlock *exit_prev = info.exit_prev;
    // std::cout << "【entry】" << entry->get_name() << " 【exit】" << exit->get_name() << " 【into_entry】" << into_entry->get_name() << " 【exit_prev】" << exit_prev->get_name() << "\n";
    auto bbid = func->get_bb_used();
    auto tmp_id = func->get_temp_used();

    std::unordered_map<BasicBlock *, BasicBlock *> bb_map;
    std::unordered_map<Temp*, Temp*> reg_map;

    for (auto bb : loop_bbs) {
        BasicBlock *new_bb = new BasicBlock(func, {}, bbid++);
        func->add_basic_block(new_bb);
        bb_map[bb] = new_bb;
        for (auto &insn : *bb->get_instructions()) {
            if (insn->is_output_inst()) {
                Temp* new_reg = new Temp(tmp_id++, insn->get_dst()->front()->get_type());
                reg_map[insn->get_dst()->front()] = new_reg;
            }
        }
    }

    // info.start_imm->setimm(info.start+1);

    for (auto bb : loop_bbs) {
        copy_bb(info, bb, bb_map.at(bb), bb_map, reg_map);
    }

    TypeCase (into_entry_br, ir::instruction::CondBranch *, into_entry->get_instructions()->back()) {
        if (into_entry_br->get_true_bb() == entry) {
            into_entry_br->set_true_bb(bb_map[entry]);
        } else if (into_entry_br->get_false_bb() == entry) {
            into_entry_br->set_false_bb(bb_map[entry]);
        } else assert(false);
    } else TypeCase (into_entry_jmp, ir::instruction::Branch *, into_entry->get_instructions()->back()) {
        assert(into_entry_jmp->get_bb() == entry);
        into_entry_jmp->set_target(bb_map[entry]);
    } else assert(false);

    auto new_exit_prev = bb_map[exit_prev];
    auto new_exit_jmp = new ir::instruction::Branch(entry);
    new_exit_prev->get_instructions()->pop_back();
    new_exit_prev->add_instruction(new_exit_jmp);
    new_exit_jmp->set_parent(new_exit_prev);
    
    // TypeCase (into_entry_br, ir::instruction::CondBranch *, new_exit_prev->get_instructions()->back()) {
    //     if (into_entry_br->get_true_bb() == bb_map[entry]) {
    //         into_entry_br->set_true_bb(entry);
    //     } else if (into_entry_br->get_false_bb() == bb_map[entry]) {
    //         into_entry_br->set_false_bb(entry);
    //     } else assert(false);
    // } else TypeCase (into_entry_jmp, ir::instruction::Branch *, new_exit_prev->get_instructions()->back()) {
    //     assert(into_entry_jmp->get_bb() == bb_map[entry]);
    //     into_entry_jmp->set_target(entry);
    // } else assert(false);

    for (auto &insn : *(info.header)->get_instructions()) {
        TypeCase (phi, ir::instruction::Phi*, insn) {
            for (auto p: phi->getpairs()) {
                if (bb_map.count(p.second)) {
                    phi->add_pair(bb_map.at(p.second), reg_map.at(p.first));
                }
                else {
                    phi->erase_src_temp(p.second);
                }
            }
        }
    }
    func->set_bb_used(bbid);
    func->set_temp_used(tmp_id);

}

bool LoopDoWhile::run(BasicBlock* bb) {
    auto loop = la_->get_loop(bb);
    std::cout << "LoopDoWhile run, loop name: " << loop->header_->get_name() << "\n";
    for (auto &bb : *func->get_basic_blocks()) {
        for (auto &insn : *bb->get_instructions()) {
            insn->set_parent(bb);
        }
        bb->set_parent(func);
    }
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
        std::cout << "dead loop or multiple exit\n";
        return false;
    }
    auto loop_info = get_loop_info(loop, loop_bbs, exit_bb);
    loop_info.exit = exit_bb;
    // std::cout << "loop_info.start: " << loop_info.start << " loop_info.end: " << loop_info.end << " loop_info.loop_type: " << loop_info.loop_type << "\n";
    if (loop_info.loop_type == 1 && loop_info.end >= 30000 && loop_info.end - loop_info.start >= 2) {
        loop_dowhile(loop, loop_info, loop_bbs);
    }
    
    if (loop_info.loop_type == 0) return false;
    return true;
}

}