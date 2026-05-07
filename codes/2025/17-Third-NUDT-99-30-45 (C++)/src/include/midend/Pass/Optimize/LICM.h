#pragma once
#include "Pass.h"
#include "Loop.h"
#include "LoopCharacteristics.h"
#include "Dom.h"
#include <unordered_set>
#include <vector>

namespace sysy{

class LICMContext {
public:
  LICMContext(Function* func, Loop* loop, IRBuilder* builder, const LoopCharacteristics* chars)
    : func(func), loop(loop), builder(builder), chars(chars) {}
    // 运行LICM主流程，返回IR是否被修改
    bool run();

private:
    Function* func;
    Loop* loop;
    IRBuilder* builder;
    const LoopCharacteristics* chars; // 特征分析结果

    // 外提所有可提升指令
    bool hoistInstructions();
};


class LICM : public OptimizationPass{
private:
	IRBuilder *builder; ///< IR构建器，用于插入指令
public:
	static void *ID;
	LICM(IRBuilder *builder = nullptr) : OptimizationPass("LICM", Granularity::Function) , builder(builder) {}
	bool runOnFunction(Function *F, AnalysisManager &AM) override;
	void getAnalysisUsage(std::set<void *> &, std::set<void *> &) const override;
	void *getPassID() const override { return &ID; }
};

} // namespace sysy