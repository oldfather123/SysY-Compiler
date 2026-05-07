#ifndef LLVM_IR_MEM2REG_H
#define LLVM_IR_MEM2REG_H

#include <map>
#include <set>
#include <stack>
#include <vector>
#include <unordered_map>
#include "cfg.h"
#include "tca2llvm.h"

namespace llvm_ir {
namespace mem2reg {

void fixupIncompletePhis(Function& F);

class Mem2Reg {
public:
    bool runOnModule(Module& M);

private:
    bool runOnFunction(Function& F);
    void findPromotableAllocas(Function& F);
    void placePhiNodes(Function& F, cfg::DominatorTree& DT);
    void renameVariables(Function& F, cfg::DominatorTree& DT);
    void renameRecursive(BasicBlock* B, cfg::DominatorTree& DT);
    void cleanup(Function& F);

    // 每个变量的当前值
    std::unordered_map<AllocaInst*, std::stack<Value*>> valueStacks;
    // 需要提升的 alloca 指令
    std::vector<AllocaInst*> promotableAllocas;
    // 待删除的指令
    std::set<Instruction*> deadInstructions;
    // PHI节点与对应alloca的映射关系
    std::unordered_map<PhiInst*, AllocaInst*> phiToAlloca;
    // 为PHI节点生成唯一名称的计数器
    std::unordered_map<AllocaInst*, int> phiNameCounter;
};

}  // namespace mem2reg
}  // namespace llvm_ir

#endif  // LLVM_IR_MEM2REG_H
