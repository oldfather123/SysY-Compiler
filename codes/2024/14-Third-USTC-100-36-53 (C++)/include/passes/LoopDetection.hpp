#pragma once
#include "AnalysisPass.hpp"
#include "Dominators.hpp"
#include "PassManager.hpp"
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class BasicBlock;
class Dominators;
class Function;
class Module;

using BBset = std::set<BasicBlock *>;
using BBvec = std::vector<BasicBlock *>;
class Loop {
  private:
    BasicBlock *preheader_ = nullptr;
    BasicBlock *header_;

    std::shared_ptr<Loop> parent_ = nullptr;
    BBvec blocks_;
    BBset all_bbs;
    std::vector<std::shared_ptr<Loop>> sub_loops_;

    std::unordered_set<BasicBlock *> back_edges_nodes_;
    std::unordered_map<BasicBlock *, BasicBlock *> exits_;

  public:
    Loop(BasicBlock *header) : header_(header) { blocks_.push_back(header); }
    ~Loop() = default;
    void add_block(BasicBlock *bb) { blocks_.push_back(bb); }
    BasicBlock *get_header() { return header_; }
    BasicBlock *get_preheader() { return preheader_; }
    std::shared_ptr<Loop> get_parent() { return parent_; }
    void set_parent(std::shared_ptr<Loop> parent) { parent_ = parent; }
    void set_preheader(BasicBlock *bb) { preheader_ = bb; }
    void add_sub_loop(std::shared_ptr<Loop> loop) {
        sub_loops_.push_back(loop);
    }
    const BBvec &get_blocks() { return blocks_; }
    const std::vector<std::shared_ptr<Loop>> &get_sub_loops() {
        return sub_loops_;
    }
    const std::unordered_set<BasicBlock *> &get_back_edges_nodes() {
        return back_edges_nodes_;
    }
    void clear_back_edges_nodes() { back_edges_nodes_.clear(); }
    const std::unordered_map<BasicBlock *, BasicBlock *> &get_exits() {
        return exits_;
    }
    void add_back_edge_node(BasicBlock *bb) { back_edges_nodes_.insert(bb); }
    void add_exit(BasicBlock *bb, BasicBlock *succ) { exits_[bb] = succ; }

    // 一个 pre-header block。
    // 只有一条 back edge (因此也只有一个 latch block)。
    // exit blocks 的前继节点都来源于循环当中，也就是说，header block 支配所有的
    // exit blocks。
    bool is_loopsimplifyform() {
        if (preheader_ == nullptr)
            return false;
        if (back_edges_nodes_.size() != 1)
            return false;
        for (auto &exitbb : exits_) {
            for (auto &prebb : exitbb.second->get_pre_basic_blocks()) {
                if (std::find(blocks_.begin(), blocks_.end(), prebb) ==
                    blocks_.end())
                    return false;
            }
        }
        return true;
    };
    const BBset &get_all_bbs() const { return all_bbs; }
    void update_all_bbs() {

        for (auto &bb : blocks_) {
            all_bbs.insert(bb);
        }
        for (auto &loop : sub_loops_) {
            for (auto &bb : loop->get_all_bbs())
                all_bbs.insert(bb);
        }
    }
};

class LoopDetection : public AnalysisPassBase<Function> {
  private:
    std::vector<std::shared_ptr<Loop>> loops_;

    std::unordered_map<BasicBlock *, std::shared_ptr<Loop>> bb_to_loop_;
    void discover_loop_and_sub_loops(BasicBlock *bb, BBset &back_edges_nodes,
                                     std::shared_ptr<Loop> loop);

  public:
    using IRUnit = Function;
    ~LoopDetection() = default;

    // struct FuncLoopInfo {
    //     std::unordered_map<BasicBlock *, std::shared_ptr<Loop>> loops;
    // };
    // std::unordered_map<Function *, FuncLoopInfo> loop_info;

    void run(Function *f, AnalysisPassManager &analysis) override;

    void run_on_func(Function *f);
    void print(Function *f, std::ostream &os) const override;
    const std::vector<std::shared_ptr<Loop>> &get_loops() const {
        return loops_;
    }
};
