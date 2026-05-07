#include "LoopSimplify.hpp"
#include "BasicBlock.hpp"
#include "Dominators.hpp"
#include "Function.hpp"
#include "Instruction.hpp"

#include "LoopDetection.hpp"
#include "Value.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>

using namespace std;
using PhiPair = pair<Value *, BasicBlock *>;
// 消除退出块前的空跳转
bool LoopSimplify::normalize_exit(Loop *L) {
    if (L->get_exits().find(L->get_header()) == L->get_exits().end())
        return false;
    auto header_exit = L->get_exits().at(L->get_header());
    auto replace_exit = [&](BasicBlock *exiting, BasicBlock *exit) -> void {
        auto succ = exit->get_succ_basic_blocks().front();
        L->add_exit(exiting, succ);
        exiting->get_terminator()->replace_operand(exit, succ);
        exit->replace_all_use_with(exiting);
        exit->erase_from_parent();
        exiting->add_succ_basic_block(succ);
        succ->add_pre_basic_block(exiting);
    };
    for (auto [exiting, exit] : L->get_exits()) {
        if (exit == header_exit)
            continue;
        if (exit->get_terminator()->is_ret())
            continue;
        if (exit->get_instructions().size() == 1 and
            exit->get_terminator()->get_num_operand() == 1) {
            replace_exit(exiting, exit);
            return true;
        }
        if (header_exit->get_instructions().size() == 1 and
            header_exit->get_succ_basic_blocks().size() == 1) {
            if (header_exit->get_succ_basic_blocks().front() == exit) {
                replace_exit(L->get_header(), header_exit);
                return true;
            }
        }
    }
    return false;
}

// 将phi指令的op按照来源是loop内外分离
pair<vector<PhiPair>, vector<PhiPair>> LoopSimplify::split_phi_op(PhiInst *phi,
                                                                  Loop &loop) {
    vector<PhiPair> inner, outer;
    for (auto &&[op, bb] : phi->get_pairs()) {
        auto loopbbs = loop.get_all_bbs();
        if (find(loopbbs.begin(), loopbbs.end(), bb) != loopbbs.end()) {
            inner.emplace_back(op, bb);
        } else {
            outer.emplace_back(op, bb);
        }
    }
    return {inner, outer};
}
/*保证一个循环有一个preheader，且head中phi引用的外部变量都来自preheader*/
BasicBlock *LoopSimplify::preheader_insertion(Loop &loop) {
    auto header = loop.get_header();
    auto func = header->get_parent();
    auto preheader = BasicBlock::create(func->get_parent(), "", func);
    loop.set_preheader(preheader);
    vector<PhiInst *> phis;
    for (auto &&inst : header->get_instructions()) {
        if (not inst.is_phi()) {
            break;
        }
        phis.push_back(dynamic_cast<PhiInst *>(&inst));
    }

    // modify phi inst
    for (auto &&phi : phis) {
        auto [inner, outer] = split_phi_op(phi, loop);
        if (outer.size() == 0) {
            continue;
        }
        if (inner.size() == 0) {
            header->remove_instr(phi);
            preheader->add_instruction(phi);
            continue;
        }
        if (outer.size() == 1) {
            for (int i = 0; i < phi->get_num_operand(); i += 2) {
                if (phi->get_operand(i + 1) == outer[0].second) {
                    phi->set_operand(i + 1, preheader);
                    break;
                }
            }

            continue;
        }
        // create a new phi in preheader
        auto phi_outer = PhiInst::create_phi(phi->get_type(), preheader);
        preheader->add_instr_begin(phi_outer);

        // for (unsigned i = 0; i < phi_outer->get_num_operand(); ++i)
        //     if (phi_outer->get_operand(i))
        //         phi_outer->get_operand(i)->remove_use(phi_outer, i);
        // phi_outer->remove_all_operands();

        for (auto [op, bb] : outer) {
            assert(op->get_type() == phi_outer->get_type());
            phi_outer->add_operand(op);
            phi_outer->add_operand(bb);
        }
        inner.emplace_back(phi_outer, preheader);
        for (unsigned i = 0; i < phi->get_num_operand(); ++i)
            if (phi->get_operand(i))
                phi->get_operand(i)->remove_use(phi, i);
        phi->remove_all_operands();

        for (auto [op, bb] : inner) {
            assert(op->get_type() == phi->get_type() and
                   "op_type must equal to phi_type");
            phi->add_operand(op);
            phi->add_operand(bb);
        }
    }

    // connect
    auto in_bbs = header->get_pre_basic_blocks();
    for (auto in_bb : in_bbs) {
        if (loop.get_back_edges_nodes().find(in_bb) ==
            loop.get_back_edges_nodes().end()) {
            // connect in_bb to preheader
            in_bb->get_terminator()->replace_operand(header, preheader);
            in_bb->remove_succ_basic_block(header);
            header->remove_pre_basic_block(in_bb);
            in_bb->add_succ_basic_block(preheader);
            preheader->add_pre_basic_block(in_bb);
            assert(find(in_bb->get_succ_basic_blocks().begin(),
                        in_bb->get_succ_basic_blocks().end(),
                        header) == in_bb->get_succ_basic_blocks().end());
        }
    }
    BranchInst::create_br(header, preheader);
    // add to parent loop
    if (loop.get_parent() != nullptr) {
        loop.get_parent()->add_block(preheader);
    }

    return preheader;
}
/* 保证循环的退出块的后继的前驱只有循环内的bb*/
BasicBlock *LoopSimplify::exit_insertion(BasicBlock *exiting,
                                         BasicBlock *outer_bb) {
    auto func = exiting->get_parent();
    auto exit = BasicBlock::create(func->get_parent(), "", func);
    exiting->get_terminator()->replace_operand(outer_bb, exit);
    BranchInst::create_br(outer_bb, exit);
    // phi
    for (auto &&inst : outer_bb->get_instructions()) {
        if (not inst.is_phi()) {
            break;
        }
        auto phi = dynamic_cast<PhiInst *>(&inst);
        for (size_t i = 0; i < phi->get_num_operand(); i++) {
            if (phi->get_operand(i) == exiting) {
                phi->set_operand(i, exit);
            }
        }
    }
    exiting->remove_succ_basic_block(outer_bb);
    outer_bb->remove_pre_basic_block(exiting);
    exiting->add_succ_basic_block(exit);
    exit->add_pre_basic_block(exiting);
    return exit;
}

// create a bb as new backdege, let all origin backedges be its prebb and header
// as its succbb
BasicBlock *LoopSimplify::insert_unique_backedgeblock(std::shared_ptr<Loop> L) {
    auto preheader = L->get_preheader();
    assert(L->get_back_edges_nodes().size() > 1 and preheader != nullptr);

    auto backedges = L->get_back_edges_nodes();
    BasicBlock *Header = L->get_header();
    Function *F = Header->get_parent();
    // create new backedge node
    BasicBlock *new_backedge = BasicBlock::create(F->get_parent(), "", F);
    // create new edge from new_backedge to header
    BranchInst::create_br(Header, new_backedge);
    // update phi
    for (auto &inst : Header->get_instructions()) {
        auto phi = dynamic_cast<PhiInst *>(&inst);
        if (phi == nullptr)
            break;
        auto newphi = PhiInst::create_phi(phi->get_type(), new_backedge);
        bool uniqueincome = true;
        Value *uniqueval = nullptr;
        Value *preheaderval = nullptr;
        // 将除了来自preheader的其余phi——op都移动到新的phi上
        for (size_t i = 0; i < phi->get_num_operand(); i++) {
            auto val = phi->get_operand(i++);
            auto bb = phi->get_operand(i);
            if (bb == preheader) {
                preheaderval = val;
                continue;
            } else {
                newphi->add_phi_pair_operand(val, bb);
                if (uniqueincome) {
                    if (!uniqueval)
                        uniqueval = val;
                    else if (uniqueval != val)
                        uniqueincome = false;
                }
            }
        }
        // assert(preheaderval != nullptr);
        for (unsigned i = 0; i < phi->get_num_operand(); ++i)
            if (phi->get_operand(i))
                phi->get_operand(i)->remove_use(phi, i);
        phi->remove_all_operands();
        if (preheaderval != nullptr)
            phi->add_phi_pair_operand(preheaderval, preheader);
        phi->add_phi_pair_operand(newphi, new_backedge);
        if (uniqueincome) {
            newphi->replace_all_use_with(uniqueval);
            new_backedge->erase_instr(newphi);
        } else
            new_backedge->add_instr_begin(newphi);
    }
    // modify the origin backedges
    for (BasicBlock *BB : backedges) {
        Instruction *ter_inst = BB->get_terminator();
        ter_inst->replace_operand(Header, new_backedge);
    }
    // update bbs relationship
    for (auto &bb : backedges) {
        bb->remove_succ_basic_block(Header);
        bb->add_succ_basic_block(new_backedge);
        new_backedge->add_pre_basic_block(bb);
        Header->remove_pre_basic_block(bb);
    }

    // only modify loop info of bbs and backedges instead of rebuild loop info
    L->clear_back_edges_nodes();
    L->add_back_edge_node(new_backedge);
    return new_backedge;
}

void LoopSimplify::run(Function *func, AnalysisPassManager &APM) {

    if (func->is_declaration()) {
        return;
    }
    bool changed = true;
    while(changed)
    {
        changed = false;
        auto &&loops = APM.getResult<LoopDetection>(func)->get_loops();
        for (auto &&loop : loops) {
            // while (normalize_exit(loop.get()));

            if (loop->get_preheader() == nullptr) {
                changed = true;
                preheader_insertion(*loop);
            }

            for (auto [exiting, exit] : loop->get_exits()) {
                for (auto &pre : exit->get_pre_basic_blocks()) {
                    if (std::find(loop->get_all_bbs().begin(),
                                loop->get_all_bbs().end(),
                                pre) == loop->get_all_bbs().end()) {
                        // auto newexit =
                        //     BasicBlock::create(func->get_parent(), "", func);
                        changed = true;
                        auto newexit = exit_insertion(exiting, exit);
                        loop->add_exit(exiting, newexit);
                        break;
                    }
                }
            }

            if (loop->get_back_edges_nodes().size() > 1)
            {
                changed = true;
                insert_unique_backedgeblock(loop);
            }
            // check LoopSimplify
            assert(loop->is_loopsimplifyform());
        }
        APM.invalidateAll<Function>(func);
    }
    
}
