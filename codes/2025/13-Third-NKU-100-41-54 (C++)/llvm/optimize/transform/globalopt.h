#ifndef GLOBALOPT_H
#define GLOBALOPT_H
#include "../../include/ir.h"
#include "../pass.h"
#include "../analysis/AliasAnalysis.h" 
#include "../analysis/dominator_tree.h"

struct LSInfo{
    bool defed;
    Operand load_res;
    LSInfo();
    LSInfo(bool def, Operand res):defed(def),load_res(res){}
};


//不考虑Array
class GlobalOptPass : public IRPass { 
private:
    AliasAnalysisPass *AA;
    std::unordered_set<Instruction> NoModRefGlobals;
    std::unordered_set<Instruction> RefOnlyGlobals;
    std::unordered_set<Instruction> ModOnlyGlobals;
    std::unordered_set<Instruction> ModRefGlobals;

    std::unordered_map<std::string,std::vector<std::pair<int,Instruction>>> def_blocks;//global val name ~ the block ids that store this global val
    std::unordered_map<std::string,std::vector<std::pair<int,Instruction>>> use_blocks;

    std::unordered_map<std::string,std::pair<BasicInstruction::LLVMType,Instruction>> global_map;

    void EliminateRedundantLS();
    void OneDefDomAllUses(CFG* cfg);

    void GlobalValTypeDef();
    void ProcessGlobals();
    void ApproxiMem2reg();

public:
    GlobalOptPass(LLVMIR *IR,AliasAnalysisPass* AApass) : IRPass(IR),AA(AApass) {}
    void Execute();
};

#endif
/*

global value
    - no-read-write:         delete                                                          √
    - read-only:             inline+delete                                                   √
    - write-only:            delete
    - read-write:            单函数: mem2reg        多函数：尽可能mem2reg+函数间保持mem
*/