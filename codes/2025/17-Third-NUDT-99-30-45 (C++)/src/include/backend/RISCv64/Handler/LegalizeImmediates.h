#ifndef SYSY_LEGALIZE_IMMEDIATES_H
#define SYSY_LEGALIZE_IMMEDIATES_H

#include "RISCv64LLIR.h"
#include "Pass.h"

namespace sysy {

// MachineFunction 的前向声明在这里是可选的，因为 RISCv64LLIR.h 已经定义了它
// class MachineFunction;

/**
 * @class LegalizeImmediatesPass
 * @brief 一个用于“合法化”机器指令的Pass。
 *
 * 这个Pass的主要职责是遍历所有机器指令，查找那些包含了超出
 * 目标架构（RISC-V）编码范围的大立即数（immediate）的指令，
 * 并将它们展开成一个等价的、只包含合法立即数的指令序列。
 *
 * 它在指令选择之后、寄存器分配之前运行，确保进入后续阶段的
 * 所有指令都符合硬件约束。
 */
class LegalizeImmediatesPass : public Pass {
public:
    static char ID;
    
    LegalizeImmediatesPass() : Pass("legalize-immediates", Granularity::Function, PassKind::Optimization) {}
    
    void *getPassID() const override { return &ID; }
    
    void runOnMachineFunction(MachineFunction* mfunc);
};

} // namespace sysy

#endif // SYSY_LEGALIZE_IMMEDIATES_H