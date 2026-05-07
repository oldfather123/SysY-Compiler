#ifndef __MACHINE_INSTRUCTION_H_
#define __MACHINE_INSTRUCTION_H_

#include "codegen/MachineOperand.hpp"
#include "codegen/utils/NotImplemented.hpp"
#include <list>
#include <memory>
#include <ostream>
#include <vector>

class MachineBlock;

class MachineInstruction {
public:
    MachineBlock *parent;

    /**
     * @brief Instruction operand list, sorted by appearance order in assembly
     * 
     * e.g.
     * addw dst, src1, src2
     * def_list{ dst }, use_list{ src1, src2 }
     */
    std::vector<MachineOperand *> def_list;
    std::vector<MachineOperand *> use_list;

    static int counter;
    int no{-1};         // 指令编号

    enum InstructionType {
        BINARY,     // 二元运算
        STORE,
        LOAD,
        BRANCH,
        CMP,
        JUMP,
        MOVE,
        CONVERT
    };
    InstructionType type;
    int optype;

    std::list<MachineInstruction *>::iterator current_it;

    MachineInstruction(MachineBlock *parent_p, InstructionType type_p, int opcode_p)
        : parent(parent_p), type(type_p), optype(opcode_p) {}
    
    virtual ~MachineInstruction() {
        for (auto &def : def_list) {
            delete def;
            def = nullptr;
        }
        for (auto &use : use_list) {
            delete use;
            use = nullptr;
        }
    }

    void insertBefore(MachineInstruction *inst);
    void insertAfter(MachineInstruction *inst);
    void replaceDef(MachineOperand *old, MachineOperand *new_);
    void replaceUse(MachineOperand *old, MachineOperand *new_);
    
    virtual void output(std::ostream &os) const = 0;
};

class BinaryMInstruction : public MachineInstruction {
public:
    enum OpType {
        /**
         * 带W(word), H(high, 高32位)的都是32位版本
         * U(unsigned)
         * 
         * Ref: 
         * RV64I 指令图示: https://cttts-oss.oss-cn-shanghai.aliyuncs.com/img/20230717165610.png
         * RV64M 指令图示: https://cttts-oss.oss-cn-shanghai.aliyuncs.com/img/20230717165642.png
         * 
         * More to Read:
         * RISC-V指南: https://box.nju.edu.cn/f/1a0088e7057849839657/
         * 第2章: RV32I     基本指令
         * 第3章: RISC-V    汇编语言, 介绍了函数调用规范、汇编指示符、伪指令等
         * 第4章: RV32M     乘除指令
         * 第9章: RV64      64位地址指令
         */
        ADD, ADDI,
        ADDW, ADDIW,

        SUB, SUBW,

        AND, ANDI,
        OR, ORI,
        XOR, XORI,
        /* TODO: 移位指令, 但我看IR里好像没有移位指令，不考虑用位运算优化计算前可先不考虑这些 */

        MULW, MULHU, MULHSU,
        DIVW, DIVUW,
        REMW, REMUW,

        //float (same with int)
        FADD ,FSUB, FMUL, FDIV,
        FMIN, FMAX,

        // right/left arithmetic/logic
        sraiw, srliw, slliw
    };

    BinaryMInstruction(MachineBlock *mBlock,
                       int type,
                       MachineOperand *dst,
                       MachineOperand *src1,
                       MachineOperand *src2);
    
    void output(std::ostream &os) const override;
};

class FnegMInstruction : public MachineInstruction {
public:
    enum OpType {
        FNEG = 25
    };

    FnegMInstruction(MachineBlock *mBlock,
                       int type,
                       MachineOperand *dst,
                       MachineOperand *src);

    void output(std::ostream &os) const override;
};


class LoadMInstruction : public MachineInstruction {
public:
    enum OpType {
        LB,     // load byte
        LH,     // load halfword
        LW,     // load word
        LD,     // load doubleword
        LBU,    // load byte        unsigned
        LHU,    // load halfword    unsigned
        LWU,    // load word        unsigned

        //float
        FLW
    };
    LoadMInstruction(MachineBlock *mBlock,
                     int opcode,
                     MachineOperand *dst,
                     MachineOperand *src1,
                     MachineOperand *src2);
    void output(std::ostream &os) const override;
};

class LoadImmInstruction : public MachineInstruction{
public:
//    li lui la暂时放在一起，不知道有没有问题
    enum OpType {LI = 7, LUI, LA};

    void output(std::ostream &os) const override;

    LoadImmInstruction(MachineBlock *mBlock,
                       int opcode,
                       MachineOperand *dst,
                       MachineOperand *src1);
};

class StoreMInstruction : public MachineInstruction {
public:
    enum OpType {
        SB,     // store byte
        SH,     // store halfword
        SW,     // store word
        SD,     // store doubleword

        //float
        FSW
    };
    StoreMInstruction(MachineBlock *mBlock,
                      int opcode,
                      MachineOperand *src,
                      MachineOperand *dst1,
                      MachineOperand *dst2);
    void output(std::ostream &os) const override;
};


class BranchMInstruction : public MachineInstruction {
public:
    enum OpType {
        BEQ, BNE,
        BGE, BGEU,
        BLT, BLTU,

        // 伪指令
        BEQZ, BNEZ, BLEZ, BGEZ, BLTZ, BGTZ,
    };
    // TODO
    BranchMInstruction(MachineBlock *mBlock,
                       int opcode,
                       MachineOperand *reg1,
                       MachineOperand *reg2,
                       MachineOperand *label);
    void output(std::ostream &os) const override;
};

class CmpMInstruction: public MachineInstruction {
public:
    enum OpType {
        SLT, SLTI,
        SLTU, SLTIU,

        // 伪指令
        SNEZ, SLTZ, SGTZ,

//        float
        FEQ,FLT,FLE,

//        maybe not used
        FGT,FGE
    };
    // TODO
    CmpMInstruction(MachineBlock *mBlock,
                    int opcode,
                    MachineOperand *dst,
                    MachineOperand *src1,
                    MachineOperand *src2);

    void output(std::ostream &os) const override;
};

class JumpMInstruction: public MachineInstruction {
public:
    enum OpType {
        // JAL, JALR,

        // 下面是伪指令，不是很确定是否该引入, 本质上都可用jal, jalr实现. 引入后operand的数量不是很好处理
        // 我倾向于优先使用JAL和JALR
        J = 0,
        // RET,
    };
    // TODO
    JumpMInstruction(MachineBlock *mBlock,
                     int opcode,
                     MachineOperand *operand);
    void output(std::ostream &os) const override;
};

class JrMInstruction: public MachineInstruction{
public:
    enum OpType{JR = 1};
    JrMInstruction(MachineBlock *mBlock,int opcode,MachineOperand *ra);
    void output(std::ostream &os) const override;
};

class JalMInstruction: public MachineInstruction{
public:
    enum OpType{JAL = 2};
    JalMInstruction(MachineBlock *mBlock, int opcode, MachineOperand *ra, MachineOperand *label);
    void output(std::ostream &os) const override;
};

class MoveMInstruction: public MachineInstruction{
public:
    enum OpType{
        FMV, MV
    };
    MoveMInstruction(MachineBlock *mBlock, int opcode, MachineOperand *dst, MachineOperand *src);
    void output(std::ostream &os) const override;
};

class ConvertMInstruction: public MachineInstruction{
public:
    enum OpType{
        F2I, I2F
    };
    ConvertMInstruction(MachineBlock *mBlock, int opcode, MachineOperand *dst, MachineOperand *src);
    void output(std::ostream &os) const override;
};

class CallFuncMInstruction: public MachineInstruction{
public:
    enum OpType{
        CALL = 3
    };
    CallFuncMInstruction(MachineBlock *mBlock, int opcode, MachineOperand *dst);
    void output(std::ostream &os) const override;
};

#endif