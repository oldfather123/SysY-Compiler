#ifndef BASIC_CSE_H
#define BASIC_CSE_H

#include "../../include/cfg.h"
#include <vector>
#include <string>
#include <map>
#include <set>
#include <deque>
#include "../analysis/dominator_tree.h"
#include "../analysis/AliasAnalysis.h"
#include "../analysis/memdep.h"
// 存储指令的CSE关键信息
struct InstCSEInfo {
    int opcode;                        // 指令操作码
    std::vector<std::string> operand_list; // 操作数字符串列表
    int cond = -1;                   // 比较条件（默认-1表示无效）
    // 比较CSE是否相同
    bool operator<(const InstCSEInfo &other) const;
};

//extern std::map<std::string, CFG *> CFGMap;            // 全局函数名到CFG的映射

// 函数声明
// void CSEInit(CFG* cfg);
bool CSENotConsider(Instruction inst);
// InstCSEInfo GetCSEInfo(Instruction inst);
// void SimpleBlockCSE(CFG* cfg);
// void SimpleDomTreeWalkCSE(CFG* C);
//void SimpleCSE(CFG* cfg);

// 基本块CSE优化实现
class BasicBlockCSEOptimizer {
public:
    BasicBlockCSEOptimizer(CFG* cfg,LLVMIR *ir,AliasAnalysisPass *aa,DomAnalysis* dom,SimpleMemDepAnalyser* ma) : C(cfg), llvmIR(ir),alias_analyser(aa),domtrees(dom),memdep_analyser(ma),changed(false),flag(false),hasMemOp(true) {}
    bool optimize();
    void NoMemoptimize();
    void setMemOp(bool enable) { hasMemOp = enable; }
    bool hasMemOp;
private:
    CFG* C;
    DomAnalysis *domtrees;
    LLVMIR *llvmIR;
    AliasAnalysisPass *alias_analyser;
    SimpleMemDepAnalyser* memdep_analyser;
    void CallKillReadMemInst(Instruction I, std::map<InstCSEInfo, int> &CallInstMap,
                         std::map<InstCSEInfo, int> &LoadInstMap, std::set<Instruction> &CallInstSet,
                         std::set<Instruction> &LoadInstSet, CFG *C) ;
    void StoreKillReadMemInst(Instruction I, std::map<InstCSEInfo, int> &CallInstMap,
                          std::map<InstCSEInfo, int> &LoadInstMap, std::set<Instruction> &CallInstSet,
                          std::set<Instruction> &LoadInstSet, CFG *C);
public:
    bool changed;
    bool flag;
    std::map<int, int> reg_replace_map;
    std::set<Instruction> erase_set;
    std::map<InstCSEInfo, Instruction> inst_map;
    std::set<Instruction> InstSet;
    std::map<InstCSEInfo, Instruction> LoadInstMap;
    std::map<InstCSEInfo, int> CallInstMap;
    std::set<Instruction> LoadInstSet;
    std::set<Instruction> CallInstSet;
    void reset();
    void processAllBlocks();
    bool processBlock(LLVMBlock bb);
    void processCallInstruction(CallInstruction* callI);
    void processStoreInstruction(StoreInstruction* storeI);
    void processLoadInstruction(LoadInstruction* loadI);
    void processRegularInstruction(BasicInstruction* I);
    void removeDeadInstructions();
    void applyRegisterReplacements();
    void inst_clear();
    void clearAllCSEInfo();
    bool IsValChanged(Instruction I1, Instruction I2, CFG *C);
};

// 支配树优化
class DomTreeCSEOptimizer {
private:
    CFG* C;
    bool changed;
    bool branch_changed;
    bool hasMemOp;
    std::set<Instruction> eraseSet;
    std::map<InstCSEInfo, int> instCSEMap;
    //std::map<InstCSEInfo, std::vector<Instruction>> loadCSEMap;
    std::map<int, int> regReplaceMap;
    DomAnalysis *domtrees;
    std::map<InstCSEInfo, std::vector<Instruction>> CmpMap; // cmp信息映射
    std::map<InstCSEInfo, std::vector<Instruction>> LoadCSEMap;
    AliasAnalysisPass *alias_analyser;
    SimpleMemDepAnalyser* memdep_analyser;
    bool tmp;
    static const int MAX_RECURSION_DEPTH = 1000; // 最大递归深度
    int currentRecursionDepth = 0;              // 当前递归深度

public:
    DomTreeCSEOptimizer(CFG* cfg,AliasAnalysisPass * aa,SimpleMemDepAnalyser* ma, DomAnalysis * dom) : C(cfg),alias_analyser(aa), memdep_analyser(ma),domtrees(dom),changed(true),branch_changed(true),hasMemOp(true) {}
    void optimize();
    void branch_optimize();
    void no_mem_optimize();
    void branch_end();
    void setMemOp(bool enable) { hasMemOp = enable; }
private:
    void dfs(int bbid);
    void branch_dfs(int bbid);
    void processLoadInstruction(LoadInstruction* loadI, std::map<InstCSEInfo, int>& tmpLoadNumMap);
    void processStoreInstruction(StoreInstruction* storeI, std::map<InstCSEInfo, int>& tmpLoadNumMap);
    void processCallInstruction(CallInstruction* callI, std::set<InstCSEInfo>& tmpCSESet);
    void processIcmpInstruction(IcmpInstruction* I,std::set<InstCSEInfo>& cmpCseSet);
    void processFcmpInstruction(FcmpInstruction* I,std::set<InstCSEInfo>& cmpCseSet);
    void processRegularInstruction(BasicInstruction* I, std::set<InstCSEInfo>& tmpCSESet);
    void cleanupTemporaryEntries(const std::set<InstCSEInfo>& tmpCSESet,const std::map<InstCSEInfo, int>& tmpLoadNumMap);
    //void cleanupTemporaryEntries(const std::set<InstCSEInfo>& regularCSESet,const std::set<InstCSEInfo>& CmpCseSet);
    //void cleanupTemporaryEntries(const std::set<InstCSEInfo>& regularCSESet);
    void removeDeadInstructions();
    void applyRegisterReplacements();
    bool EmptyBlockJumping(CFG *C);
};

class SimpleCSEPass : public IRPass { 
private:
    DomAnalysis *domtrees;
    AliasAnalysisPass *alias_analyser;
    SimpleMemDepAnalyser* memdep_analyser;
    // 新增：维护函数名到其所有call指令的映射
    std::map<std::string, std::vector<CallInstruction*>> funcCallMap;
    // 新增：维护函数名到是否可以进行内存优化的映射
    std::map<std::string, bool> canMemOpMap;

public:
    SimpleCSEPass(LLVMIR *IR, DomAnalysis *dom,AliasAnalysisPass* aa) : IRPass(IR),alias_analyser(aa) { domtrees = dom; }
    void Execute();
    void BlockExecute();
    void CSEInit(CFG* cfg);
    InstCSEInfo GetCSEInfo(Instruction inst);
    void SimpleBlockCSE(CFG* cfg,LLVMIR *IR);
    void SimpleDomTreeWalkCSE(CFG* C);
    void DomtreeExecute();//包含branch
    void BNExecute();
    
    // 新增方法
    void buildFuncCallMap();
    bool canMemOp(const std::string& funcName);
};
// class BranchCSEPass : public IRPass { 
// private:
//     DomAnalysis *domtrees;
//     void branch_dfs(CFG *C,int bbid);
//     std::map<InstCSEInfo, std::vector<Instruction>> CmpMap;
//     bool changed ;  // 循环优化标志
//     void cleanupTemporaryEntries(const std::set<InstCSEInfo>& cmpCSESet);
//     void branch_end(CFG* C);
//     void cse_init(CFG *C);
// public:
//     BranchCSEPass(LLVMIR *IR, DomAnalysis *dom) : IRPass(IR),changed(true) { domtrees = dom; }
//     void Execute();
//     void BranchCSE(CFG *C);
// };

#endif