#include "LoopDetection.hpp"
#include "AnalysisPass.hpp"
#include "Dominators.hpp"
#include <algorithm>
#include <cassert>
#include <memory>
#include <queue>

void LoopDetection::discover_loop_and_sub_loops(BasicBlock *bb,
                                                BBset &back_edges_nodes,
                                                std::shared_ptr<Loop> loop) {
    BBvec work_list = {back_edges_nodes.begin(), back_edges_nodes.end()};
    while (!work_list.empty()) {
        auto bb = work_list.back();
        work_list.pop_back();
        if (bb_to_loop_.find(bb) == bb_to_loop_.end()) {
            loop->add_block(bb);
            bb_to_loop_[bb] = loop;
            for (auto &pred : bb->get_pre_basic_blocks()) {
                work_list.push_back(pred);
            }
        } else if (bb_to_loop_[bb] != loop) {
            auto sub_loop = bb_to_loop_[bb];
            while (sub_loop->get_parent() != nullptr) {
                sub_loop = sub_loop->get_parent();
            }
            if (sub_loop == loop) {
                continue;
            }
            sub_loop->set_parent(loop);
            loop->add_sub_loop(sub_loop);
            for (auto &bb1 : sub_loop->get_header()->get_pre_basic_blocks()) {
                work_list.push_back(bb1);
            }
        }
    }
}

void LoopDetection::run(Function *f, AnalysisPassManager &apm) {
    auto dominators_ = apm.getResult<Dominators>(f);
    for (auto &bb1 : dominators_->get_dom_post_order()) {
        auto bb = bb1;
        BBset back_edges_nodes;
        for (auto &pred : bb->get_pre_basic_blocks()) {
            if (dominators_->is_dominate(bb, pred)) {
                // pred is a back edge
                back_edges_nodes.insert(pred);
            }
        }
        if (back_edges_nodes.empty()) {
            continue;
        }
        // create loop
        auto loop = std::make_shared<Loop>(bb);
        bb_to_loop_[bb] = loop;
        for (auto &back_edge_node : back_edges_nodes) {
            loop->add_back_edge_node(back_edge_node);
        }
        loops_.push_back(loop);
        discover_loop_and_sub_loops(bb, back_edges_nodes, loop);
        BBvec block_vec_;
        std::queue<std::shared_ptr<Loop>> loop_queue_;
        loop_queue_.push(loop);
        while (!loop_queue_.empty()) {
            std::shared_ptr<Loop> front = loop_queue_.front();
            block_vec_.insert(block_vec_.begin(), front->get_blocks().begin(),
                              front->get_blocks().end());
            for (auto &sub_loop : front->get_sub_loops())
                loop_queue_.push(sub_loop);
            loop_queue_.pop();
        }
        for (auto &block : block_vec_)
            for (auto &succ : block->get_succ_basic_blocks())
                if (std::find(block_vec_.begin(), block_vec_.end(), succ) ==
                    block_vec_.end())
                    loop->add_exit(block, succ);
        auto header = loop->get_header();
        if (header->get_pre_basic_blocks().size() -
                loop->get_back_edges_nodes().size() ==
            1) {
            for (auto &pre : header->get_pre_basic_blocks()) {
                if (std::find(loop->get_back_edges_nodes().begin(),
                              loop->get_back_edges_nodes().end(),
                              pre) == loop->get_back_edges_nodes().end()) {
                    if (pre->get_succ_basic_blocks().size() == 1)
                        loop->set_preheader(pre);
                    break;
                }
            }
        }
        loop->update_all_bbs();
    }
}

void LoopDetection::print(Function *f, std::ostream &os) const {
    os << "Function: " << f->get_name() << std::endl;
    os << "Loop Detection Result:" << std::endl;
    for (auto &loop : loops_) {
        os << "Loop header: " << loop->get_header()->get_name() << std::endl;
        os << "Loop blocks: ";
        for (auto &bb : loop->get_blocks()) {
            os << bb->get_name() << " ";
        }
        os << std::endl;
        os << "Sub loops: ";
        for (auto &sub_loop : loop->get_sub_loops()) {
            os << sub_loop->get_header()->get_name() << " ";
        }
        os << std::endl;
    }
}