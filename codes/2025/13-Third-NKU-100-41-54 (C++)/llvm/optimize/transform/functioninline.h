#ifndef FUNCTIONINLINE_H
#define FUNCTIONINLINE_H
#include "../../include/ir.h"
#include "../pass.h"
#include "../lib_function_names.h"


class FunctionInlinePass : public IRPass { 
private:
    // 存储函数调用图  //NOTE:指针类型的unordered_map
    std::unordered_map<FuncDefInstruction, std::set<FuncDefInstruction>> callGraph;
    // 存储被调用函数的返回指令
    std::unordered_map<FuncDefInstruction, std::pair<int,RetInstruction*>> calleeReturn;
    // 存储函数的大小（指令数量）
    std::unordered_map<FuncDefInstruction, int> funcSize;
    // 存储block_id对应的后续phi指令
    std::unordered_map<FuncDefInstruction,std::unordered_map<int,std::set<PhiInstruction*>>>phiGraph;
    // 内联阈值，超过此值的函数不会被内联
    const int INLINE_THRESHOLD = 30;


    // 构建函数调用图
    void buildCallGraph(FuncDefInstruction caller);
    // 判断函数是否适合内联
    bool shouldInline(FuncDefInstruction caller, FuncDefInstruction callee);
    // 执行函数内联
    void inlineFunction(int callerBlockId, LLVMBlock callerBlock,FuncDefInstruction caller, FuncDefInstruction callee, CallInstruction* callInst);
    // 重命名寄存器
    int renameRegister(FuncDefInstruction caller,int oldReg, std::unordered_map<int, int>& regMapping);
    // 重命名基本块标签
    int renameLabel(FuncDefInstruction caller,int oldLabel, std::unordered_map<int, int>& labelMapping);
    // 复制基本块
    LLVMBlock copyBasicBlock(FuncDefInstruction caller, LLVMBlock origBlock, std::unordered_map<int, int>& regMapping, std::unordered_map<int, int>& labelMapping);
    // 重组由于函数传参被截断的GEP指令，使得数组都是一次引用
    void recombineGEP();
    
public:
    std::unordered_set<FuncDefInstruction> inlined_function_names;
    FunctionInlinePass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
};

#endif