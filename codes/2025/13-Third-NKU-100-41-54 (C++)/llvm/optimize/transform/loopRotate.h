#ifndef LOOPROTATE_H
#define LOOPROTATE_H
#include "../../include/ir.h"
#include "../pass.h"
#include "loop.h"
#include "licm.h"

class LoopRotate : public IRPass {
public:
	/* such as in loop : while(j < 4), condInsts:
		%2 = load i32，i32* %j，align4，!tbaa !30
		%3 = icmp slt i32 %2，4
		br i1 %3,label %6, label %13
	*/
	LoopInvariantCodeMotionPass licm_pass;
	std::deque<Instruction> condInsts;
	// 存储PHI指令的映射关系：PHI结果寄存器 -> (基本块ID -> 对应的操作数)
    std::map<Operand, std::map<int, Operand>> phi_mapping;
	std::unordered_map<Loop*, int> loop2invariantCount;
    LoopRotate(LLVMIR *IR, AliasAnalysisPass* AA): IRPass(IR), licm_pass(IR, AA){}
    void Execute() override;
private:
    void rotateLoop(Loop* loop, DominatorTree* dt, CFG* cfg);
	void calcInvarientVarNumber(Loop* loop, CFG* cfg);
    bool canRotate(Loop* loop, CFG* cfg);
	void findAndDeleteCondInsts(Loop* loop, CFG* cfg);
	void findCondInsts(Loop* loop, CFG* cfg);
	void replaceUncondByCond(LLVMBlock bb, Operand true_label, Operand false_label, int& max_reg);
    void doRotate(Loop* loop, CFG* cfg);
    void fixPhi(Loop* loop, CFG* cfg, LLVMBlock exit_bb);
	void CheckAllPhi(CFG* cfg);
};

#endif