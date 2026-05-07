#pragma once

#include "Module.hpp"
#include "Function.hpp"
#include "GlobalVariable.hpp"
#include "Instruction.hpp"
#include "Type.hpp"
#include "Value.hpp"
#include "BasicBlock.hpp"

#include <map>
#include <set>
#include <vector>

struct LiveRange {
    int start;
    int end;
    unsigned priority{0}; // 优先级，越大越优先
    std::vector<int> uses; // 使用该值的指令索引
};

class LiveVariableAnalysis {
  public:
    explicit LiveVariableAnalysis(Module *module) : m(module) {}

    void run();

    const std::map<Value *, LiveRange> &get_live_ranges() const {
        return live_ranges;
    }

  private:
    Module *m;
    std::map<Value *, LiveRange> live_ranges;
    // std::set<Value *> live_set;
    std::map<BasicBlock *, bool> visited;
    std::vector<std::pair<BasicBlock *, BasicBlock *>> circles;
    std::vector<std::pair<int, int>> circle_intervals; // 用于记录循环的起止点
    std::vector<BasicBlock *> bb_order;
    BasicBlock *prev_bb{nullptr};
    std::map<Instruction *, int> inst_to_index; // 指令到索引的映射

    void analyze_function(Function *f);
    void clear();
    void prepare_bb(Function *f);
    void dfs_function(Function *f);
    void dfs_basic_block(BasicBlock *bb);
    void make_inst_index();
    void make_circle_intervals();
    void check_instruction(Instruction *inst);
};
