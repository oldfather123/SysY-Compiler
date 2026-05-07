#ifndef LLVM_IR_GENERATOR_H
#define LLVM_IR_GENERATOR_H

#include "../ir/tca_ir.h"
#include "../ir/llvm_ir.h"
#include <map>
#include <memory>

namespace llvm_ir {

class LLVMIRGenerator {
private:
    std::unique_ptr<Module> module;
    IRBuilder builder;
    std::map<std::string, Value*> valueMap; // 映射变量名到LLVM Value
    std::map<std::string, llvm_ir::Type> typeMap; // 映射类型
    std::map<int, BasicBlock*> blockMap; // 映射指令位置到基本块
    Function* currentFunction;
    BasicBlock* currentBlock;
    const ir::Function* currentIRFunction; // 当前正在处理的IR函数
    size_t currentInstructionIndex; // 当前指令索引
    int labelCounter;
    int tempCounter;

public:
    LLVMIRGenerator(const std::string& moduleName);
    
    // 主要转换函数
    std::unique_ptr<Module> generateModule(const ir::Program& program);
    
    // 类型转换
    llvm_ir::Type convertType(ir::Type irType);
    llvm_ir::Type convertTypeForArrayElement(ir::Type irType);
    
    // 函数转换
    void generateFunction(const ir::Function& irFunc);
    
    // 指令转换
    void generateInstruction(const ir::Instruction* inst);
    
    // 特殊指令转换
    void generateCallInstruction(const ir::CallInst* callInst);
    void generateBinaryOperation(const ir::Instruction* inst);
    void generateUnaryOperation(const ir::Instruction* inst);
    void generateTypeConversion(const ir::Instruction* inst);
    void generateCompareOperation(const ir::Instruction* inst);
    void generateMemoryOperation(const ir::Instruction* inst);
    void generateControlFlow(const ir::Instruction* inst);
    
    // 操作数转换
    Value* convertOperand(const ir::Operand& operand);
    Value* convertOperandForCall(const ir::Operand& operand);
    
    // 获取数组指针（不进行load操作）
    Value* getArrayPointer(const ir::Operand& operand);
    
    // 辅助函数
    std::string getNextLabel();
    std::string getNextTemp();
    Value* getOrCreateValue(const std::string& name, llvm_ir::Type type);
    
    // 基本块管理
    void createBasicBlocks(const ir::Function& irFunc);
    int parseJumpTarget(const std::string& target, int currentPC);
    
    // 生成内置函数声明
    void generateBuiltinFunctions();
    
    // 预分配变量（解决支配关系问题）
    void preallocateVariables(const ir::Function& irFunc);
    
    // 推断变量类型
    llvm_ir::Type inferVariableType(const ir::Function& irFunc, const std::string& varName);
    
private:
    // 创建常量
    Value* createConstant(const std::string& value, llvm_ir::Type type);
    
    // 处理全局变量
    void generateGlobalVariables(const ir::Program& program);
};

} // namespace llvm_ir

#endif
