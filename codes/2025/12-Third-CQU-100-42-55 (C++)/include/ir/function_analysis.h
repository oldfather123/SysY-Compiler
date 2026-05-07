#ifndef FUNCTION_ANALYSIS_H
#define FUNCTION_ANALYSIS_H

#include "llvm_ir.h"
#include <set>
#include <map>
#include <queue>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace llvm_ir {

class Module;

namespace function_analysis {

// 函数属性分析器
class FunctionAttributeAnalyzer {
public:
    FunctionAttributeAnalyzer(Module* M = nullptr) : module(M) {}
    
    // 分析整个模块的函数属性（使用工作列表算法）
    void analyzeModule(Module& M);
    
    // 获取函数的属性
    std::set<FunctionAttribute> getFunctionAttributes(const Function& F) const;
    
    // 检查函数是否具有特定属性
    bool hasAttribute(const Function& F, FunctionAttribute attr) const;
    
    // 检查函数调用是否安全（用于LICM等优化）
    bool isCallSafe(const CallInst* call) const;

private:
    // 模块引用，用于函数查找
    Module* module;
    
    // 单一数据源：存储每个函数的属性
    std::map<const Function*, std::set<FunctionAttribute>> functionAttributes;
    
    // 高效的函数查找表
    std::unordered_map<std::string, Function*> functionNameMap;
    
    // 内置函数库集合（直接初始化为固定集合，不再动态插入）
    inline static const std::unordered_set<std::string> builtinFunctions = {
        "getint", "getch", "getfloat", "getarray", "getfarray",
        "putint", "putch", "putarray", "putfloat", "putfarray", "putf",
        "before_main", "after_main", "_sysy_starttime", "_sysy_stoptime",
        "scanf", "printf", "fprintf", "vfprintf", "gettimeofday"
    };
    
    // 调用图：记录函数之间的调用关系
    std::map<const Function*, std::set<const Function*>> callGraph;
    std::map<const Function*, std::set<const Function*>> reverseCallGraph;
    
    // 工作列表算法相关
    std::queue<Function*> worklist;
    std::set<Function*> inWorklist;
    
    // 初始化分析
    void initializeAnalysis(Module& M);
    
    // 构建函数查找表和调用图
    void buildFunctionMaps(Module& M);
    
    // 检查是否为内置函数
    bool isBuiltinFunction(const std::string& functionName) const;
    
    // 获取内置函数的属性
    std::set<FunctionAttribute> getBuiltinFunctionAttributes(const std::string& functionName) const;
    
    // 工作列表迭代算法
    void worklistIteration();
    
    // 重新分析函数并检查属性是否变化
    bool reanalyzeFunction(Function& F);
    
    // 将函数及其调用者加入工作列表
    void addToWorklist(Function* F);
    void addCallersToWorklist(const Function* F);
    
    // 提交最终结果到Function对象
    void commitResults();
    
    // 分析函数的内存访问模式
    void analyzeMemoryAccess(Function& F);
    
    // 检查函数是否只读取内存
    bool isReadOnly(Function& F);
    
    // 检查函数是否不访问内存
    bool isReadNone(Function& F);
    
    // 检查指令的内存访问模式
    bool instructionMayWriteMemory(const Instruction* I, const Function* currentFunc = nullptr);
    bool instructionMayReadMemory(const Instruction* I, const Function* currentFunc = nullptr);
    
    // 检查函数调用是否安全
    bool isCallSafeForHoisting(const CallInst* call);
    
    // 递归检查函数调用是否写入内存
    bool isCallMayWriteMemory(const Instruction* I, const Function* currentFunc);
    
    // 递归检查函数调用是否读取内存
    bool isCallMayReadMemory(const Instruction* I, const Function* currentFunc);
    
    // 高效的函数查找
    Function* findCalledFunction(const std::string& functionName);
    
    // 比较函数属性集合（用于检测属性变化）
    bool attributesChanged(const Function& F, const std::set<FunctionAttribute>& oldAttrs);
};

// 便捷函数：分析模块中所有函数的属性
void analyzeFunctionAttributes(Module& M);

} // namespace function_analysis
} // namespace llvm_ir

#endif // FUNCTION_ANALYSIS_H 