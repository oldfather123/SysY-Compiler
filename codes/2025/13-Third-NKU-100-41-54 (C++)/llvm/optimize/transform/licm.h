#ifndef LICM_H
#define LICM_H
#include "../../include/ir.h"
#include "../pass.h"
#include "loop.h"
#include "AliasAnalysis.h"

// This pass uses alias analysis for two purposes:
// 本优化过程通过别名分析实现两个目的：
// 1. Moving loop invariant loads and calls out of loops.
//    将循环不变的加载(load)和调用(call)移出循环
// 2. Scalar Promotion of Memory.
//    内存的标量提升

/* 
 * For purpose (1):
 * If we can determine that a load or call inside of a loop never aliases 
 * anything stored to, we can hoist it or sink it like any other instruction.
 * 
 * 对于目的(1):
 * 如果能确定循环内的load/call与任何store操作不存在别名关系，
 * 则可以像普通指令一样将其提升(hoist)或下沉(sink)出循环
 */

/*
 * For purpose (2) Scalar Promotion:
 * Conditions for moving store instruction AFTER loop:
 * 对于目的(2)标量提升，将store移到循环后的条件：
 * 
 * a) The pointer stored through is loop invariant.
 *    a) 存储操作的指针是循环不变量
 *    
 * b) No stores/loads in the loop may alias the pointer.
 *    b) 循环内没有可能与该指针存在别名关系的stores/loads操作
 *    
 * c) No calls in the loop mod/ref the pointer.
 *    c) 循环内没有会修改/引用该指针的函数调用
 * 
 * If satisfied:
 * 若满足条件：
 * - Promote loads/stores to use a temporary alloca'd variable
 *   将loads/stores提升为使用临时alloca变量
 * - Apply mem2reg to construct SSA form
 *   应用mem2reg功能构建SSA形式
 */

class LoopInvariantCodeMotionPass : public IRPass {
	AliasAnalysisPass* aa;
public:
	std::unordered_map<Operand, Instruction> inloop_defs;
	std::unordered_map<Operand, std::unordered_set<Instruction>> inloop_uses;
	std::unordered_set<Instruction> eraseInsts;
	std::vector<Instruction> writeInsts;
	std::unordered_map<std::string, CFG*> cfgTable;
	
    void RunLICMOnLoop(Loop* loop, DominatorTree* dt, CFG* cfg);
	bool dominatesAllExits(LLVMBlock bb, Loop* loop, DominatorTree* dt);
	bool canHoistWithAlias(Instruction inst, Loop* loop, CFG* cfg);
	bool isInvariant(Instruction inst, Loop* loop, CFG* cfg);
	void getInloopDefUses(Loop* loop);
	void getInloopWriteInsts(Loop* loop);
	void updateWriteInsts(Instruction* inst);
	void updateInloopDefUses(Instruction inst);
	void eraseBlockOnLoop(Loop* loop);
    LoopInvariantCodeMotionPass(LLVMIR *IR) : IRPass(IR) {}
	LoopInvariantCodeMotionPass(LLVMIR *IR, AliasAnalysisPass* AA) : IRPass(IR), aa(AA) {}
    void Execute();
};

#endif