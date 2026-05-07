#ifndef MEM2REG_H
#define MEM2REG_H
#include "../../include/ir.h"
#include "../pass.h"
#include <unordered_set>
#include "../analysis/dominator_tree.h"

class Mem2RegPass : public IRPass {
private:
    DomAnalysis *domtrees;
    std::unordered_set<int> loaded_regno; // 所有被load过的regno
    std::unordered_set<int> array_regno;  // 所有指令为getelementptr的regno(result)，其store不能删除
    std::map<int, std::unordered_set<int>> use_map; // load过的regno与其出现的block set，用于选择优化方法
    std::map<int, std::unordered_set<int>> def_map; // store过的regno与其出现的block set，用于选择优化方法

    bool IsPromotable(Instruction AllocaInst);
    void Mem2RegNoUseAlloca(CFG *C, std::unordered_set<int> &vset);
    void Mem2RegUseDefInSameBlock(CFG *C, std::unordered_set<int> &vset, int block_id);
    void Mem2RegOneDefDomAllUses(CFG *C, std::unordered_set<int> &vset, int block_id);
    void BasicMem2Reg(CFG *C);
    void InsertPhi(CFG *C, int& max_reg);
    void DFS(CFG *C, int bbid);
    void VarRename(CFG *C);
    void Mem2Reg(CFG *C, int& max_reg);

public:
    Mem2RegPass(LLVMIR *IR, DomAnalysis *dom) : IRPass(IR) { domtrees = dom; }
    void Execute();
};

#endif
