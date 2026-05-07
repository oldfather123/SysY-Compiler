#ifndef RISCV64_ASMPRINTER_H
#define RISCV64_ASMPRINTER_H

#include "RISCv64LLIR.h"
#include <iostream>

extern int DEBUG;
extern int DEEPDEBUG;

namespace sysy {

class RISCv64AsmPrinter {
public:
    RISCv64AsmPrinter(MachineFunction* mfunc);
    
    // 主入口
    void run(std::ostream& os, bool debug = false);
    void printInstruction(MachineInstr* instr, bool debug = false);
    // 辅助函数
    void setStream(std::ostream& os) { OS = &os; }
    // 辅助函数
    std::string regToString(PhysicalReg reg) const;
    std::string formatInstr(const MachineInstr *instr);

private:
    // 打印各个部分
    void printBasicBlock(MachineBasicBlock* mbb, bool debug = false);
    // 辅助函数
    void printOperand(MachineOperand* op);

    MachineFunction* MFunc;
    std::ostream* OS = nullptr;

};

} // namespace sysy

#endif // RISCV64_ASMPRINTER_H