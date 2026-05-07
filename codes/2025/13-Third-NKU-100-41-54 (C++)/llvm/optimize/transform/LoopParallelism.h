#ifndef LOOP_PARALLELISM_PASS_H
#define LOOP_PARALLELISM_PASS_H

#include "../pass.h"
#include "../analysis/ScalarEvolution.h"
#include "../analysis/loop.h"
#include "../analysis/LoopDependenceAnalysis.h"
#include "../../include/cfg.h"
#include <functional>
#include <vector>
#include <map>
#include <string>

class LoopParallelismPass : public IRPass {
public:
	int thread_id_regNo = 0;
	int start_regNo = 0;
	int end_regNo = 0;
	int my_start_regNo = 0;
	int my_end_regNo = 0;
	int new_header_block_id = 0;
	int new_preheader_block_id = 0;
	int new_latch_block_id = 0;
	std::map<std::string, int> name_counter;
	std::set<int> i32set, i64set, float32set;
	LoopDependenceAnalysisPass* loop_dependence_analyser;

    LoopParallelismPass(LLVMIR* IR, LoopDependenceAnalysisPass* LDA = nullptr) : IRPass(IR), loop_dependence_analyser(LDA) {}
    void Execute() override;

private:
    // 主要处理函数
    void ProcessFunction(CFG* cfg);
    void ProcessLoop(Loop* loop, CFG* cfg, ScalarEvolution* SE);
    
    // 循环可并行化性检查
    bool CanParallelizeLoop(Loop* loop, CFG* cfg, ScalarEvolution* SE);
    bool IsSimpleLoop(Loop* loop, CFG* cfg);
    bool IsConstantIterationCount(Loop* loop, CFG* cfg, ScalarEvolution* SE);
    bool HasLoopExternalUses(Loop* loop, CFG* cfg);
        
    // 循环体提取和函数生成
    void ExtractLoopBodyToFunction(Loop* loop, CFG* cfg, ScalarEvolution* SE);
    std::string GenerateFunctionName(CFG* cfg, Loop* loop);
    void CreateParallelFunction(Loop* loop, CFG* cfg, const std::string& func_name, ScalarEvolution* SE);
	void BuildCFGforParallelFunction(FunctionDefineInstruction* func_def);
    void CreateParallelCall(Loop* loop, CFG* cfg, const std::string& func_name, ScalarEvolution* SE);
    
    // 循环体指令处理
    void CopyLoopBodyInstructions(Loop* loop, FunctionDefineInstruction* func_def, std::map<int, int>& reg_mapping);
    Instruction CloneInstruction(Instruction inst, FunctionDefineInstruction* func_def, std::map<int, int>& reg_mapping);
    
    // 参数和返回值处理
    void AddFunctionParameters(FunctionDefineInstruction* func_def, std::map<int, int>& reg_mapping);
    void AddThreadRangeCalculation(FunctionDefineInstruction* func_def, std::map<int, int>& reg_mapping);
    
    // 辅助函数
    std::string GenerateUniqueName(const std::string& base_name);
    
    // 运行时库函数声明
    void AddRuntimeLibraryDeclarations();
    
    // 分支结构生成
    void CreateConditionalParallelization(Loop* loop, CFG* cfg, const std::string& func_name, 
                                         ScalarEvolution* SE);

	// 收集循环外部变量
    void CollectLoopExternalVariables(Loop* loop, CFG* cfg);
};

#endif // LOOP_PARALLELISM_PASS_H
