#ifndef LLVM_IR_LICM_H
#define LLVM_IR_LICM_H

#include "llvm_ir.h"

namespace llvm_ir {

class Module;

namespace licm {

// Main entry point for the LICM pass
bool runOnModule(Module& M);

// 内部使用的LICM pass类
class LICMPass {
public:
    LICMPass(Module& M) : module(M) {}
    bool runOnFunction(Function& F);
    
    // 查找函数定义
    Function* findFunction(const std::string& name);
    
private:
    Module& module;
};

} // namespace licm
} // namespace llvm_ir

#endif // LLVM_IR_LICM_H 