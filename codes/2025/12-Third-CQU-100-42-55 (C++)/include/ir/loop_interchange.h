#ifndef LLVM_IR_LOOP_INTERCHANGE_H
#define LLVM_IR_LOOP_INTERCHANGE_H

#include "llvm_ir.h"
#include "loop_analysis.h"
#include "scev.h"
#include <vector>
#include <set>
#include <map>

// 该Pass尝试对简单两层嵌套循环进行循环交换（Loop Interchange），以改善访存局部性。
// 设计目标：
//   1. 仅处理已经过 normalizeLoops / LCSSA 的简单结构循环（单一 latch、单一 exit、唯一 preheader）。
//   2. 仅当父循环恰好只有一个直接子循环，且外层循环除去子循环本身没有额外副作用指令时才考虑交换。
//   3. 使用 SCEV 获取两个循环的 primary induction variable 以及上下界，要求二者均为简单线性递增（步长常量 >0）。
//   4. 安全性极端保守：发现任何 Store / Call / 复杂指令使用两个 induction variable 之间可能存在顺序依赖则放弃。
//   5. 当前实现的实际重写部分采取“占位”策略：预留 transformNestedLoops() 接口，后续可补充真正 CFG 重构。
//      现阶段只做合法性检测并输出调试信息，返回是否满足交换条件。若未来实现，可在 transformNestedLoops 内完成：
//        - 构建新的循环框架（交换 header / latch 的层次）
//        - 复制/调整 phi, icmp, branch
//        - 更新 use-def 与前驱/后继列表
//
// 这样做的原因：完整 loop interchange 在当前自定义 IR 上需要大量 CFG 精细改写与 phi 维护；分阶段提交更安全。

namespace llvm_ir {
namespace loop_interchange {

struct InterchangeCandidateInfo {
    Loop* outerLoop = nullptr;
    Loop* innerLoop = nullptr; // outerLoop 的直接子循环
    Value* outerIndVar = nullptr; // primary var
    Value* innerIndVar = nullptr; // primary var
    SCEVLoopVariableInfo* outerVarInfo = nullptr;
    SCEVLoopVariableInfo* innerVarInfo = nullptr;
    bool profitable = false;   // 经过简单启发式判定是否“可能”获益
    bool safe = false;         // 是否通过安全性检查
};

class LoopInterchangePass {
public:
    LoopInterchangePass(Module& M) : module(M) {}

    bool run();
    bool runOnFunction(Function& F);
    // 暴露单次两层交换（供多层相邻交换调用）
    bool transformNestedLoops(InterchangeCandidateInfo& info);

    // 公共判定接口：多层相邻交换过程需要复用
    bool isStructurallySimplePair(Loop* outer, Loop* inner) const;
    bool isSafeToInterchange(const InterchangeCandidateInfo& info) const;
    bool isProfitable(const InterchangeCandidateInfo& info) const;

private:
    Module& module;

    // 收集候选
    void collectCandidates(Function& F, LoopAnalysis& LA, SCEVAnalysis& SCEVA, std::vector<InterchangeCandidateInfo>& out);
};

} // namespace loop_interchange
} // namespace llvm_ir

#endif // LLVM_IR_LOOP_INTERCHANGE_H
