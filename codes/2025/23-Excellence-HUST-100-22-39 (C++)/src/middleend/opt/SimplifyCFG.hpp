#ifndef __SIMPLIFY_CFG_H__
#define __SIMPLIFY_CFG_H__

#include "opt.hpp"

class SimplifyCFG: public Optimization {
private:
	// 删除冗余的Phi指令
	void delete_rebundant_phi(Function* func);
	// 合并只有一个前驱且该前驱只有一个后继的基本块
	bool merge_single_way_blocks(Function* func);
	// 确保删除中继基本块时不会破坏控制流图的完整性
	bool check_useless_jump(BasicBlock* bb);
	// 删除中继基本块：从前驱基本块确定跳转到该块且该块只有一条无条件跳转指令的基本块
	bool delete_relay_blocks(Function* func);
public:
	explicit SimplifyCFG(Module* m): Optimization(m) {}
	void execute() override;
};

#endif // __SIMPLIFY_CFG_H__