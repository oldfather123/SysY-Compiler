#pragma once

#include <string>
#include "IR.h"

namespace sysy {

class SysYPrinter {
private:
    Module *pModule;

public:
    explicit SysYPrinter(Module *pModule) : pModule(pModule) {}

public:
    void printIR();
    void printGlobalVariable();
    void printGlobalConstant();
    

public:
    static void printFunction(Function *function);
    static void printInst(Instruction *pInst);
    static void printType(Type *type);
    static void printValue(Value *value);
    static void printBlock(BasicBlock *block);
    static std::string getBlockName(BasicBlock *block);
    static std::string getOperandName(Value *operand);
    static std::string getTypeString(Type *type);
    static std::string getValueName(Value *value);
};

}  // namespace sysy
