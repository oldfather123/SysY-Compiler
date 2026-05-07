#ifndef __MACHINE_OPERAND_H_
#define __MACHINE_OPERAND_H_

#include <memory>
#include <ostream>

class MachineInstruction;

class MachineOperand {
public:
    enum MachineOperandType{ IMM, VREG, REG, LABEL, FVREG };
    
    MachineInstruction *parent;
    MachineOperandType type;
    int val;
    int reg_no;
    std::string label;
    std::string vreg_name;
    bool param{false};
    int paramNo;
    bool fpu{false};
    float fval;

    
    MachineOperand() = delete;
    MachineOperand(MachineOperandType type, int val, bool fpu = false); // IMM_I, I_REG, F_REG
    MachineOperand(MachineOperandType type, std::string label_p); // VREG, LABEL
    // MachineOperand(int type_p, float fval_p);

    void print_reg(std::ostream &os) const;
    void output(std::ostream &os) const;
};

enum IREGISTER {
    ZERO = 0,
    RA,
    SP,
    GP,
    TP,
    T0, T1, T2,
    FP,
    S1,
    A0, A1, A2, A3, A4, A5, A6, A7,
    S2, S3, S4, S5, S6, S7, S8, S9, S10, S11,
    T3, T4, T5, T6,
};

enum Fregister{
    // f0, f1, f2, f3, f4, f5, f6, f7, //temp
    // f8, f9, //save
    // f10, f11, f12, f13, f14, f15, f16, f17, //func
    // f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, //saved
    // f28,
    // f29,
    // f30,
    // f31,
    ft0 = 32, ft1, ft2, ft3, ft4, ft5, ft6, ft7,                 // temp
    fs0, fs1,                                               // saved
    fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7,                 // func args
    fs2, fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11,     // saved
    ft8, ft9, ft10, ft11,                                   // temp
};

#endif