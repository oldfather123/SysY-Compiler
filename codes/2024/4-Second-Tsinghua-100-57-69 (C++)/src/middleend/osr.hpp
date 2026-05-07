#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/reverse_postorder.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/use_def_analysis.hpp"
#include "middleend/copy_propagation.hpp"
#include "middleend/loop_analysis.hpp"

namespace middleend {

struct edge {
    BinaryOp op;
    Temp *rc;
    ir::Instruction *next_iv;
    edge(BinaryOp op, Temp *rc, ir::Instruction *next_iv) : op(op), rc(rc), next_iv(next_iv) {}
};

class OSR {
private:
    ir::Function *func_;
    int next_num;
    std::unordered_map<ir::Instruction *, int> num, low;
    std::stack<ir::Instruction *> inst_stack;
    std::unordered_set<ir::Instruction *> on_stack;
    std::queue<ir::Instruction *> all_insts;
    std::unordered_set<ir::Instruction *> visited;
    std::unordered_map<ir::Instruction *, ir::BasicBlock *> header;
    std::vector<std::tuple<BinaryOp, ir::Instruction *, ir::Instruction *>> key_list;
    std::vector<Temp *> hashTable;
    std::unordered_map<ir::Instruction *, std::vector<edge>> edges;
    CFG *cfg;
    DominatorTree_ *dt;
    ReversePostOrder *rpo;
    UseDefAnalysis *ud;
    LoopAnalysis *loop_analysis;

    bool isRC(ir::Instruction *inst, ir::BasicBlock *header) {
        TypeCase(li, ir::instruction::LoadImm4*, inst) return true;
        TypeCase(alloc, ir::instruction::Alloca*, inst) return true;
        return header != inst->get_parent() && dt->get_dom(header).count(inst->get_parent());
    }

    void dfs(ir::Instruction *inst);
    void process(std::unordered_set<ir::Instruction*> SCC);
    void classifyIV(std::unordered_set<ir::Instruction*> SCC);
    bool candidate_replace(ir::Instruction *inst);
    bool valid_update4iv(ir::Instruction *inst, std::unordered_set<ir::Instruction *> SCC, ir::BasicBlock *head);
    void replace(ir::instruction::Binary *n, ir::Instruction *iv, ir::Instruction *rc);
    Temp *reduce(BinaryOp op, ir::Instruction *iv, ir::Instruction *rc, Type type);
    ir::Instruction *Clone(ir::Instruction *inst, Temp *new_temp);
    Temp *apply(BinaryOp op, ir::Instruction *o1, ir::Instruction *o2, Type type);
    void lftr();
    void changeCompare(ir::instruction::Binary *binary, ir::Instruction *iv, ir::Instruction *rc, BinaryOp op);
    void insert_params();
    void remove_params();
    bool check_in_loop(ir::BasicBlock *bb, ir::BasicBlock *header);

    void run();
public:
    ~OSR() {
        delete cfg;
        delete dt;
        delete ud;
        delete rpo;
        delete loop_analysis;
    }
    OSR(ir::Function *func) : func_(func) {
        run();
    }
};

} // namespace middleend