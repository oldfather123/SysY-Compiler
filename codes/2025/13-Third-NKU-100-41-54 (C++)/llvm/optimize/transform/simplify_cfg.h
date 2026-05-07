#ifndef SIMPLIFY_CFG_H
#define SIMPLIFY_CFG_H
#include "../../include/ir.h"
#include "../pass.h"
#include "../analysis/dominator_tree.h"


class SimplifyCFGPass : public IRPass {
private:
    void EliminateUnreachedBlocksInsts(CFG *C);
    void EliminateNotExistPhi(CFG *C);
    void EliminateOneBrUncondBlocks(CFG *C);
	bool IsSafeToEliminate(CFG *C, LLVMBlock block, LLVMBlock pred_block, LLVMBlock nextbb);
    void EliminateOnePredPhi(CFG *C,LLVMBlock nowblock,std::unordered_set<int> regno_tobedeleted);//EOPPhi执行的函数
    void TransformOnePredPhi(CFG *C);//TOPPhi执行的函数
    
public:
    SimplifyCFGPass(LLVMIR *IR) : IRPass(IR){}

    void Execute();//消除不可达块
    void EOBB(); //消除只有一条无条件跳转指令的基本块【自带重建CFG】
    void MergeBlocks();//合并通过br_uncond相连的多个连续基本块
    void EOPPhi();//删除无用phi指令（得到了单前驱phi，这些phi得到的result都没用了，连带后续均删除）【专用于SCCP等】
    void TOPPhi();//将有用的单前驱phi指令删除，将前驱值通过use_map赋值给后续用到的指令；如果你得到了单前驱Phi,但此赋值关系有必要保留，使用这个
	void BasicBlockLayoutOptimize(); // 优化基本块布局

    void RebuildCFGforSCCP();//针对SCCP这类优化后会有一些基本块被删除的优化善后
    void RebuildCFG();//通用的重建CFG，包含了cfg+domtree（正向）+use_map的重建
};

#endif
