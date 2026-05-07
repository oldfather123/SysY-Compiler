#pragma once
#include "Dominators.hpp"
#include "PassManager.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class BasicBlock;
class Dominators;
class Function;
class Module;
class Value;
class Constant;
class Instruction;
class Argument;

using BBset = std::set<BasicBlock *>;
using BBvec = std::vector<BasicBlock *>;

class Loop {
  private:
    BasicBlock *preheader_ = nullptr;
    BasicBlock *header_;
    
    std::shared_ptr<Loop> parent_ = nullptr;
    BBvec blocks_;
    std::vector<std::shared_ptr<Loop>> sub_loops_;
    
    std::unordered_set<BasicBlock *> latches_;
    int depth_ = 0;  // 循环嵌套深度
    
  public:
    Loop(BasicBlock *header) : header_(header) {
        blocks_.push_back(header);
    }
    ~Loop() = default;
    
    void add_block(BasicBlock *bb) { blocks_.push_back(bb); }
    BasicBlock *get_header() { return header_; }
    BasicBlock *get_preheader() { return preheader_; }
    std::shared_ptr<Loop> get_parent() { return parent_; }
    void set_parent(std::shared_ptr<Loop> parent) { parent_ = parent; }
    void set_preheader(BasicBlock *bb) { preheader_ = bb; }
    void add_sub_loop(std::shared_ptr<Loop> loop) { sub_loops_.push_back(loop); }
    const BBvec& get_blocks() { return blocks_; }
    BBvec& get_blocks_mut() { return blocks_; }  // 用于修改blocks_
    const std::vector<std::shared_ptr<Loop>>& get_sub_loops() { return sub_loops_; }
    const std::unordered_set<BasicBlock *>& get_latches() { return latches_; }
    void add_latch(BasicBlock *bb) { latches_.insert(bb); }
    
    int get_depth() const { return depth_; }
    void set_depth(int depth) { depth_ = depth; }
    
    // 判断基本块是否在循环中
    bool contains(BasicBlock *bb) const {
        return std::find(blocks_.begin(), blocks_.end(), bb) != blocks_.end();
    }
    
    //允许访问私有成员
    friend class LoopDetection;
};

class LoopDetection : public Pass {
  private:
    Function *func_;
    std::unique_ptr<Dominators> dominators_;
    std::vector<std::shared_ptr<Loop>> loops_;
    // map from basic block to its innermost loop
    std::unordered_map<BasicBlock *, std::shared_ptr<Loop>> bb_to_loop_;
    
    void discover_loop_and_sub_loops(BasicBlock *header, const BBset &latches, std::shared_ptr<Loop> loop);
    void find_and_set_preheader(std::shared_ptr<Loop> loop);
    void remove_duplicate_blocks();
    void calculate_loop_depth();
    void calculate_depth_recursive(std::shared_ptr<Loop> loop, int depth);
    void print_loop(std::shared_ptr<Loop> loop, int indent);
    
  public:
    LoopDetection(Module *m) : Pass(m) {}
    ~LoopDetection() = default;

    void run() override;
    void run_on_func(Function *f);
    void print();
    
    // 获取所有循环
    std::vector<std::shared_ptr<Loop>>& get_loops() { return loops_; }
    std::vector<std::shared_ptr<Loop>> all_loops_;
    
    // 获取包含指定基本块的最内层循环
    std::shared_ptr<Loop> get_loop_of(BasicBlock *bb) {
        if (bb_to_loop_.find(bb) != bb_to_loop_.end()) {
            return bb_to_loop_[bb];
        }
        return nullptr;
    }
    
    // 获取包含指定基本块的循环的header
    BasicBlock* get_loop_header_of(BasicBlock *bb) {
        auto loop = get_loop_of(bb);
        return loop ? loop->get_header() : nullptr;
    }
    
    // 判断值是否是循环不变量
    bool is_loop_invariant(Value *val, std::shared_ptr<Loop> loop);
    
    // 获取支配信息
    Dominators* get_dominators() { return dominators_.get(); }
};