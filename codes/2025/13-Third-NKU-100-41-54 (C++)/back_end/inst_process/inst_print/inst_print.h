#ifndef INST_PRINT_H
#define INST_PRINT_H
#include<iostream>
#include"../inst_select/inst_select.h"
class MachinePrinter {
    // 指令打印基类
protected:
    MachineUnit *printee;
    MachineFunction *current_func;
    MachineBlock *cur_block;
    std::ostream &s;
    bool output_physical_reg;

public:
    virtual void emit() = 0;
    MachinePrinter(std::ostream &s, MachineUnit *printee) : s(s), printee(printee), output_physical_reg(false) {}
    void SetOutputPhysicalReg(bool outputPhy) { output_physical_reg = outputPhy; }
    std::ostream &GetPrintStream() { return s; }
};

class RiscV64Printer : public MachinePrinter {
private:
public:
    void emit();
    void SyncFunction(MachineFunction *func);
    void SyncBlock(MachineBlock *block);
    RiscV64Printer(std::ostream &s, MachineUnit *printee) : MachinePrinter(s, printee) {}

    template <class INSPTR> void printAsm(INSPTR ins);
    template <class FIELDORPTR> void printRVfield(FIELDORPTR);
};
#endif