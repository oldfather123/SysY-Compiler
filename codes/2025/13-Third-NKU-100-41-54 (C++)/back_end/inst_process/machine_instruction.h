#ifndef MACHINE_INSTRUCTION_H
#define MACHINE_INSTRUCTION_H
#include<vector>
#include"../basic/register.h"
#include"inst_help.h"
#include"../../include/Instruction.h"

enum RelocType {
    NONE, 
    PCREL_HI, 
    PCREL_LO
};

class MachineBaseInstruction {
public:
    //enum { ARM = 0, RiscV, PHI};
    enum { ARM = 0, RiscV, PHI, COPY, LOCAL_LABEL };
    const int arch;

private:
    int ins_number; // 指令编号, 用于活跃区间计算
	bool no_schedule;

public:
    void setNumber(int ins_number) { this->ins_number = ins_number; }
    int getNumber() { return ins_number; }
    MachineBaseInstruction(int arch) : arch(arch) {}
    virtual std::vector<Register *> GetReadReg() = 0;     // 获得该指令所有读的寄存器
    virtual std::vector<Register *> GetWriteReg() = 0;    // 获得该指令所有写的寄存器
    virtual int GetLatency() = 0;    // 如果你不打算实现指令调度优化，可以忽略该函数

    void SetNoSchedule(bool no_schedule) { this->no_schedule = no_schedule; }
    bool CanSchedule() { return !no_schedule; }
};




extern Register RISCVregs[];
class RiscV64Instruction : public MachineBaseInstruction {
private:
    int op;
    Register rd, rs1, rs2, rs3;
    bool use_label;
    int imm ;
    RiscVLabel label;
    RelocType reloc_type;
    int local_label_id; // For local labels like 3:

    // 下面两个变量的具体作用见ConstructCall函数
    int callireg_num;
    int callfreg_num;

    int ret_type; // 用于GetI_typeReadreg(), 即确定函数返回时用的是a0寄存器还是fa0寄存器, 或者没有返回值

    std::vector<Register *> GetR_typeReadreg() { return {&rs1, &rs2}; }
    std::vector<Register *> GetR2_typeReadreg() { return {&rs1}; }
    std::vector<Register *> GetR4_typeReadreg() { return {&rs1, &rs2, &rs3}; }
    std::vector<Register *> GetI_typeReadreg() {
        std::vector<Register *> ret;
        ret.push_back(&rs1);
        if (op == RISCV_JALR) { 
            // 当ret_type为1或2时, 我们认为jalr只会用于函数返回, 所以jalr会读取a0或fa0寄存器(即函数返回值)
            // 如果函数没有返回值或者你在其他地方使用到了jalr指令，将ret_type设置为0即可
            if (ret_type == 1) {
                ret.push_back(&RISCVregs[RISCV_a0]);
            } else if (ret_type == 2) {
                ret.push_back(&RISCVregs[RISCV_fa0]);
            }
        }
        return ret;
    }
    std::vector<Register *> GetS_typeReadreg() { return {&rs1, &rs2}; }
    std::vector<Register *> GetB_typeReadreg() { return {&rs1, &rs2}; }
    std::vector<Register *> GetU_typeReadreg() { return {}; }
    std::vector<Register *> GetJ_typeReadreg() { return {}; }
    std::vector<Register *> GetCall_typeReadreg() {
        std::vector<Register *> ret;
        for (int i = 0; i < callireg_num; i++) {
            ret.push_back(&RISCVregs[RISCV_a0 + i]);
        }
        for (int i = 0; i < callfreg_num; i++) {
            ret.push_back(&RISCVregs[RISCV_fa0 + i]);
        }
        return ret;
    }

    std::vector<Register *> GetR_typeWritereg() { return {&rd}; }
    std::vector<Register *> GetR2_typeWritereg() { return {&rd}; }
    std::vector<Register *> GetR4_typeWritereg() { return {&rd}; }
    std::vector<Register *> GetI_typeWritereg() { return {&rd}; }
    std::vector<Register *> GetS_typeWritereg() { return {}; }
    std::vector<Register *> GetB_typeWritereg() { return {}; }
    std::vector<Register *> GetU_typeWritereg() { return {&rd}; }
    std::vector<Register *> GetJ_typeWritereg() { return {&rd}; }
    std::vector<Register *> GetCall_typeWritereg() {
        return {
        &RISCVregs[RISCV_ra],

        &RISCVregs[RISCV_t0],  &RISCVregs[RISCV_t1],  &RISCVregs[RISCV_t2],   &RISCVregs[RISCV_t3],
        &RISCVregs[RISCV_t4],  &RISCVregs[RISCV_t5],  &RISCVregs[RISCV_t6],

        &RISCVregs[RISCV_a0],  &RISCVregs[RISCV_a1],  &RISCVregs[RISCV_a2],   &RISCVregs[RISCV_a3],
        &RISCVregs[RISCV_a4],  &RISCVregs[RISCV_a5],  &RISCVregs[RISCV_a6],   &RISCVregs[RISCV_a7],

        &RISCVregs[RISCV_ft0], &RISCVregs[RISCV_ft1], &RISCVregs[RISCV_ft2],  &RISCVregs[RISCV_ft3],
        &RISCVregs[RISCV_ft4], &RISCVregs[RISCV_ft5], &RISCVregs[RISCV_ft6],  &RISCVregs[RISCV_ft7],
        &RISCVregs[RISCV_ft8], &RISCVregs[RISCV_ft9], &RISCVregs[RISCV_ft10], &RISCVregs[RISCV_ft11],

        &RISCVregs[RISCV_fa0], &RISCVregs[RISCV_fa1], &RISCVregs[RISCV_fa2],  &RISCVregs[RISCV_fa3],
        &RISCVregs[RISCV_fa4], &RISCVregs[RISCV_fa5], &RISCVregs[RISCV_fa6],  &RISCVregs[RISCV_fa7],
        };
    }

    friend class RiscV64InstructionConstructor;

public:
    RiscV64Instruction() : MachineBaseInstruction(MachineBaseInstruction::RiscV), imm(0), use_label(false), reloc_type(NONE), local_label_id(-1) {}

    RiscV64Instruction(int id) : MachineBaseInstruction(MachineBaseInstruction::LOCAL_LABEL), local_label_id(id) {} // For LOCAL_LABEL
    void setOpcode(int op, bool use_label) {
        this->op = op;
        this->use_label = use_label;
    }
    void setRd(Register rd) { this->rd = rd; }
    void setRs1(Register rs1) { this->rs1 = rs1; }
    void setRs2(Register rs2) { this->rs2 = rs2; }
    void setRs3(Register rs3) { this->rs3 = rs3; }
    void setImm(int imm) { this->imm = imm; }
    void setLabel(RiscVLabel label) { this->label = label; }
    void setCalliregNum(int n) { callireg_num = n; }
    void setCallfregNum(int n) { callfreg_num = n; }
    void setRetType(int use) { ret_type = use; }
    void setRelocType(RelocType type) { reloc_type = type; }
    void setLocalLabelId(int id) { local_label_id = id; }
    Register getRd() { return rd; }
    Register getRs1() { return rs1; }
    Register getRs2() { return rs2; }
    Register getRs3() { return rs3; }
    void setUseLabel(bool use_label) { this->use_label = use_label; }
    bool getUseLabel() { return use_label; }
    int getImm() { return imm; }
    RiscVLabel getLabel() { return label; }
    RelocType getRelocType() { return reloc_type; }
    int getLocalLabelId() { return local_label_id; }
    int getOpcode() { return op; }
    std::vector<Register *> GetReadReg() {
        if (arch == LOCAL_LABEL) {
            return {};
        }
        switch (OpTable.at(op).ins_formattype) {
        case RvOpInfo::R_type:
            return GetR_typeReadreg();
        case RvOpInfo::R2_type:
            return GetR2_typeReadreg();
        case RvOpInfo::R4_type:
            return GetR4_typeReadreg();
        case RvOpInfo::I_type:
            return GetI_typeReadreg();
        case RvOpInfo::S_type:
            return GetS_typeReadreg();
        case RvOpInfo::B_type:
            return GetB_typeReadreg();
        case RvOpInfo::U_type:
            return GetU_typeReadreg();
        case RvOpInfo::J_type:
            return GetJ_typeReadreg();
        case RvOpInfo::CALL_type:
            return GetCall_typeReadreg();
        }
        ERROR("Unexpected insformat");
    }
    
    std::vector<Register *> GetWriteReg() {
        if (arch == LOCAL_LABEL) {
            return {};
        }
        switch (OpTable.at(op).ins_formattype) {
        case RvOpInfo::R_type:
            return GetR_typeWritereg();
        case RvOpInfo::R2_type:
            return GetR2_typeWritereg();
        case RvOpInfo::R4_type:
            return GetR4_typeWritereg();
        case RvOpInfo::I_type:
            return GetI_typeWritereg();
        case RvOpInfo::S_type:
            return GetS_typeWritereg();
        case RvOpInfo::B_type:
            return GetB_typeWritereg();
        case RvOpInfo::U_type:
            return GetU_typeWritereg();
        case RvOpInfo::J_type:
            return GetJ_typeWritereg();
        case RvOpInfo::CALL_type:
            return GetCall_typeWritereg();
        }
        ERROR("Unexpected insformat");
    }
   
    int GetLatency() {
        if (arch == LOCAL_LABEL) {
            return 0;
        }
        return OpTable.at(op).latency;
    }
};

class RiscV64InstructionConstructor {
    static RiscV64InstructionConstructor instance;

    RiscV64InstructionConstructor() {}

public:
    static RiscV64InstructionConstructor *GetConstructor() { return &instance; }
    // 函数命名方法大部分与RISC-V指令格式一致

    // example: addw Rd, Rs1, Rs2 
    RiscV64Instruction *ConstructR(int op, Register Rd, Register Rs1, Register Rs2) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, false);
        Assert(OpTable[op].ins_formattype == RvOpInfo::R_type);
        ret->setRd(Rd);
        ret->setRs1(Rs1);
        ret->setRs2(Rs2);
        return ret;
    }
    // example: fmv.x.w Rd, Rs1
    RiscV64Instruction *ConstructR2(int op, Register Rd, Register Rs1) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, false);
        Assert(OpTable[op].ins_formattype == RvOpInfo::R2_type);
        ret->setRd(Rd);
        ret->setRs1(Rs1);
        return ret;
    }
    // example: fmadd.s Rd, Rs1, Rs2, Rs3
    RiscV64Instruction *ConstructR4(int op, Register Rd, Register Rs1, Register Rs2, Register Rs3) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, false);
        Assert(OpTable[op].ins_formattype == RvOpInfo::R4_type);
        ret->setRd(Rd);
        ret->setRs1(Rs1);
        ret->setRs2(Rs2);
        ret->setRs3(Rs3);
        return ret;
    }
    // example: lw Rd, imm(Rs1) 
    // example: addi Rd, Rs1, imm
    RiscV64Instruction *ConstructIImm(int op, Register Rd, Register Rs1, int imm) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, false);
		Assert(imm <= 2047 && imm >= -2048);
        Assert(OpTable[op].ins_formattype == RvOpInfo::I_type);
        ret->setRd(Rd);
        ret->setRs1(Rs1);
        ret->setImm(imm);
        return ret;
    }
    // example: lw Rd label(Rs1)   =>  lw Rd %lo(label_name)(Rs1)
    // example: addi Rd, Rs1, label  =>  addi Rd, Rs1, %lo(label_name)
    RiscV64Instruction *ConstructILabel(int op, Register Rd, Register Rs1, RiscVLabel label) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, true);
        Assert(OpTable[op].ins_formattype == RvOpInfo::I_type);
        ret->setRd(Rd);
        ret->setRs1(Rs1);
        ret->setLabel(label);
        return ret;
    }

    RiscV64Instruction *ConstructIImm_pcrel_lo(int op, Register Rd, Register Rs1, RiscVLabel label) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, true);
        Assert(OpTable[op].ins_formattype == RvOpInfo::I_type);
        ret->setRd(Rd);
        ret->setRs1(Rs1);
        ret->setLabel(label);
        ret->setRelocType(PCREL_LO);
        ret->setLocalLabelId(label.get_local_label_id());
        return ret;
    }

    // example: sw value imm(ptr)
    RiscV64Instruction *ConstructSImm(int op, Register value, Register ptr, int imm) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, false);
		Assert(imm <= 2047 && imm >= -2048);
        Assert(OpTable[op].ins_formattype == RvOpInfo::S_type);
        ret->setRs1(value);
        ret->setRs2(ptr);
        ret->setImm(imm);
        return ret;
    }
    // example: sw value label(ptr)  =>  sw value %lo(label_name)(ptr)
    RiscV64Instruction *ConstructSLabel(int op, Register value, Register ptr, RiscVLabel label) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, true);
        Assert(OpTable[op].ins_formattype == RvOpInfo::S_type);
        ret->setRs1(value);
        ret->setRs2(ptr);
        ret->setLabel(label);
        ret->setRelocType(PCREL_LO);
        ret->setLocalLabelId(label.get_local_label_id());
        return ret;
    }
    // example: b(cond) Rs1, Rs2,label  =>  bne Rs1, Rs2, .L3(标签具体如何输出见riscv64_printasm.cc)
    RiscV64Instruction *ConstructBLabel(int op, Register Rs1, Register Rs2, RiscVLabel label) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, true);
        Assert(OpTable[op].ins_formattype == RvOpInfo::B_type);
        ret->setRs1(Rs1);
        ret->setRs2(Rs2);
        ret->setLabel(label);
        ret->setRelocType(PCREL_HI);
        return ret;
    }
    // example: lui Rd, imm
    RiscV64Instruction *ConstructUImm(int op, Register Rd, int imm) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, false);
        Assert(OpTable[op].ins_formattype == RvOpInfo::U_type);
        ret->setRd(Rd);
        ret->setImm(imm);
        return ret;
    }
    // example: lui Rd, %hi(label_name)
    RiscV64Instruction *ConstructULabel(int op, Register Rd, RiscVLabel label) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, true);
        Assert(OpTable[op].ins_formattype == RvOpInfo::U_type);
        ret->setRd(Rd);
        ret->setLabel(label);
        return ret;
    }

    RiscV64Instruction *ConstructUImm_pcrel_hi(int op, Register Rd, RiscVLabel label) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, true);
        Assert(OpTable[op].ins_formattype == RvOpInfo::U_type);
        ret->setRd(Rd);
        ret->setLabel(label);
        ret->setRelocType(PCREL_HI);
        return ret;
    }

    // example: jal rd, label  =>  jal a0, .L4
    RiscV64Instruction *ConstructJLabel(int op, Register rd, RiscVLabel label) {
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, true);
        Assert(OpTable[op].ins_formattype == RvOpInfo::J_type);
        ret->setRd(rd);
        ret->setLabel(label);
        return ret;
    }
    // example: call funcname  
    // iregnum 和 fregnum 表示该函数调用会分别用几个物理寄存器和浮点寄存器传参
    // iregnum 和 fregnum 的作用为精确确定call会读取哪些寄存器 (具体见GetCall_typeWritereg()函数)
    // 可以进行更精确的寄存器分配
    // 对于函数调用，我们单独处理这一条指令，而不是用真指令替代，原因是函数调用涉及到部分寄存器的读写
    RiscV64Instruction *ConstructCall(int op, std::string funcname, int iregnum, int fregnum) {
        Assert(OpTable[op].ins_formattype == RvOpInfo::CALL_type);
        RiscV64Instruction *ret = new RiscV64Instruction();
        ret->setOpcode(op, true);
        // ret->setRd(GetPhysicalReg(phy_rd));
        ret->setCalliregNum(iregnum);
        ret->setCallfregNum(fregnum);
        ret->setLabel(RiscVLabel(funcname, false));
        return ret;
    }
    
    RiscV64Instruction *ConstructLocalLabel(int id) {
        return new RiscV64Instruction(id);
    }
};
extern RiscV64InstructionConstructor *rvconstructor;


#endif