#ifndef BASIC_DSE_H
#define BASIC_DSE_H
#include "../../include/cfg.h"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <deque>
#include "../analysis/dominator_tree.h"
#include "../analysis/AliasAnalysis.h"
#include "../analysis/memdep.h"


class SimpleDSEPass : public IRPass { 
private:
    DomAnalysis *domtrees;
    AliasAnalysisPass *alias_analyser;
    SimpleMemDepAnalyser* memdep_analyser;

    std::set<Instruction> erase_set;// 用于存储待删除的指令集合
    std::map<Operand, std::vector<Instruction>> storeptrs_map;//store_ptr->store_ins

    void EliminateIns(CFG* C);
    void DomDfs(CFG*C,int bbid);
    void BasicBlockDSE(CFG *C);
    void PostDomTreeWalkDSE(CFG *C);
    void EliminateNotUsedStore(CFG *C);
    void SimpleDSE(CFG *C);

public:
    SimpleDSEPass(LLVMIR *IR, DomAnalysis *dom,AliasAnalysisPass* aa) : IRPass(IR),alias_analyser(aa) { domtrees = dom; }
    void Execute();
};
#endif