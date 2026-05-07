#ifndef CALLEE_SAVED_HANDLER_H
#define CALLEE_SAVED_HANDLER_H

#include "RISCv64LLIR.h"
#include "Pass.h"

namespace sysy {

/**
 * @class CalleeSavedHandler
 * @brief 处理被调用者保存寄存器(Callee-Saved Registers)的Pass。
 * * 这个Pass在寄存器分配之后运行。它的主要职责是：
 * 1. 扫描整个函数，找出所有被使用的 `s` 系列寄存器。
 * 2. 在函数序言中插入 `sd` 指令来保存这些寄存器。
 * 3. 在函数结尾（ret指令前）插入 `ld` 指令来恢复这些寄存器。
 * 4. 正确计算因保存这些寄存器而需要的额外栈空间，并更新StackFrameInfo。
 */
class CalleeSavedHandler : public Pass {
public:
    static char ID;
    
    CalleeSavedHandler() : Pass("callee-saved-handler", Granularity::Function, PassKind::Optimization) {}
    
    void *getPassID() const override { return &ID; }
    
    bool runOnFunction(Function *F, AnalysisManager& AM) override;
    
    void runOnMachineFunction(MachineFunction* mfunc);
};

} // namespace sysy

#endif // CALLEE_SAVED_HANDLER_H