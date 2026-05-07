#ifndef RISCV64_BACKEND_H
#define RISCV64_BACKEND_H

#include "IR.h"
#include <string>

extern int DEBUG;
extern int DEEPDEBUG;
extern int optLevel;

namespace sysy {

// RISCv64CodeGen 现在是一个高层驱动器
class RISCv64CodeGen {
public:
    RISCv64CodeGen(Module* mod) : module(mod) {}
    // 唯一的公共入口点
    std::string code_gen();

private:
    // 模块级代码生成
    std::string module_gen();
    // 函数级代码生成 (实现新的流水线)
    std::string function_gen(Function* func);

    // 私有辅助函数，用于根据类型计算其占用的字节数。
    unsigned getTypeSizeInBytes(Type* type);
    
    Module* module;
    bool irc_failed = false;
};

} // namespace sysy

#endif // RISCV64_BACKEND_H