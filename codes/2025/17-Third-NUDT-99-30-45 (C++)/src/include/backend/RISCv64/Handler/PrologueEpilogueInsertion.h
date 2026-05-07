#ifndef SYSY_PROLOGUE_EPILOGUE_INSERTION_H
#define SYSY_PROLOGUE_EPILOGUE_INSERTION_H

#include "RISCv64LLIR.h"
#include "Pass.h"

namespace sysy {

class MachineFunction;

/**
 * @class PrologueEpilogueInsertionPass
 * @brief 在函数中插入序言和尾声的机器指令。
 *
 * 这个Pass在所有栈帧大小计算完毕后（包括局部变量、溢出槽、被调用者保存寄存器），
 * 在寄存器分配之后运行。它的职责是：
 * 1. 根据 StackFrameInfo 中的最终栈大小，生成用于分配和释放栈帧的指令 (addi sp, sp, +/-size)。
 * 2. 生成用于保存和恢复返回地址(ra)和旧帧指针(s0)的指令。
 * 3. 将这些指令作为 MachineInstr 对象插入到 MachineFunction 的入口块和所有返回块中。
 * 4. 这个Pass可能会生成带有大立即数的指令，需要后续的 LegalizeImmediatesPass 来处理。
 */
class PrologueEpilogueInsertionPass : public Pass {
public:
    static char ID;
    
    PrologueEpilogueInsertionPass() : Pass("prologue-epilogue-insertion", Granularity::Function, PassKind::Optimization) {}
    
    void *getPassID() const override { return &ID; }
    
    void runOnMachineFunction(MachineFunction* mfunc);
};

} // namespace sysy

#endif // SYSY_PROLOGUE_EPILOGUE_INSERTION_H