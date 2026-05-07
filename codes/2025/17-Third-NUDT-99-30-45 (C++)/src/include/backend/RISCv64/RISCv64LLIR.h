#ifndef RISCV64_LLIR_H
#define RISCV64_LLIR_H

#include "IR.h" // 确保包含了您自己的IR头文件
#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <cstdint>
#include <map>

// 前向声明，避免循环引用
namespace sysy {
class Function;
class RISCv64ISel;
}

namespace sysy {

// 物理寄存器定义
enum class PhysicalReg {
    // --- 特殊功能寄存器 ---
    ZERO, RA, SP, GP, TP,

    // --- 整数寄存器 (按调用约定分组) ---
    // 临时寄存器 (调用者保存)
    T0, T1, T2, T3, T4, T5, T6,
    
    // 保存寄存器 (被调用者保存)
    S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11,
    
    // 参数/返回值寄存器 (调用者保存)
    A0, A1, A2, A3, A4, A5, A6, A7,

    // --- 浮点寄存器 ---
    F0, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, 
    F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, 
    F22, F23, F24, F25, F26, F27, F28, F29, F30, F31,

    // 用于内部表示物理寄存器在干扰图中的节点ID（一个简单的特殊ID，确保不与vreg_counter冲突）
    // 假设 vreg_counter 不会达到这么大的值
    PHYS_REG_START_ID = 1000000, 
    PHYS_REG_END_ID = PHYS_REG_START_ID + 320, // 预留足够的空间

    INVALID, ///< 无效寄存器标记
};

// RISC-V 指令操作码枚举
enum class RVOpcodes {
    // 算术指令
    ADD, ADDI, ADDW, ADDIW, SUB, SUBW, MUL, MULW, MULH, DIV, DIVW, REM, REMW,
    // 逻辑指令
    XOR, XORI, OR, ORI, AND, ANDI,
    // 移位指令
    SLL, SLLI, SLLW, SLLIW, SRL, SRLI, SRLW, SRLIW, SRA, SRAI, SRAW, SRAIW,
    // 比较指令
    SLT, SLTI, SLTU, SLTIU,
    // 内存访问指令
    LW, LH, LB, LWU, LHU, LBU, SW, SH, SB, LD, SD,
    // 控制流指令
    J, JAL, JALR, RET,
    BEQ, BNE, BLT, BGE, BLTU, BGEU,
    // 伪指令
    LI, LA, MV, NEG, NEGW, SEQZ, SNEZ,
    // 函数调用
    CALL,
    // 特殊标记，非指令
    LABEL,

    // 浮点指令 (RISC-V 'F' 扩展)
    // 浮点加载与存储
    FLW,    // flw rd, offset(rs1)
    FSW,    // fsw rs2, offset(rs1)
    FLD,    // fld rd, offset(rs1)
    FSD,    // fsd rs2, offset(rs1)

    // 浮点算术运算 (单精度)
    FADD_S, // fadd.s rd, rs1, rs2
    FSUB_S, // fsub.s rd, rs1, rs2
    FMUL_S, // fmul.s rd, rs1, rs2
    FDIV_S, // fdiv.s rd, rs1, rs2
    FMADD_S, // fmadd.s rd, rs1, rs2, rs3
    
    // 浮点比较 (单精度)
    FEQ_S,  // feq.s rd, rs1, rs2 (结果写入整数寄存器rd)
    FLT_S,  // flt.s rd, rs1, rs2 (less than)
    FLE_S,  // fle.s rd, rs1, rs2 (less than or equal)
    
    // 浮点转换
    FCVT_S_W, // fcvt.s.w rd, rs1 (有符号整数 -> 单精度浮点)
    FCVT_W_S, // fcvt.w.s rd, rs1 (单精度浮点 -> 有符号整数)
    FCVT_W_S_RTZ, // fcvt.w.s rd, rs1, rtz (使用向零截断模式)

    // 浮点传送/移动
    FMV_S,    // fmv.s rd, rs1 (浮点寄存器之间)
    FMV_W_X,  // fmv.w.x rd, rs1 (整数寄存器位模式 -> 浮点寄存器)
    FMV_X_W,  // fmv.x.w rd, rs1 (浮点寄存器位模式 -> 整数寄存器)
    FNEG_S,   // fneg.s rd, rs (浮点取负)

    // 浮点控制状态寄存器 (CSR)
    FSRMI,    // fsrmi rd, imm (设置舍入模式立即数)

    // 伪指令
    FRAME_LOAD_W,  // 从栈帧加载 32位 Word (对应 lw)
    FRAME_LOAD_D,  // 从栈帧加载 64位 Doubleword (对应 ld)
    FRAME_STORE_W, // 保存 32位 Word 到栈帧 (对应 sw)
    FRAME_STORE_D, // 保存 64位 Doubleword 到栈帧 (对应 sd)
    FRAME_LOAD_F,   // 从栈帧加载单精度浮点数
    FRAME_STORE_F,  // 将单精度浮点数存入栈帧
    FRAME_ADDR,  // 获取栈帧变量的地址
    PSEUDO_KEEPALIVE, // 保持寄存器活跃，防止优化器删除
};

inline bool isGPR(PhysicalReg reg) {
    return reg >= PhysicalReg::ZERO && reg <= PhysicalReg::T6;
}

// 判断一个物理寄存器是否是浮点寄存器 (FPR)
inline bool isFPR(PhysicalReg reg) {
    return reg >= PhysicalReg::F0 && reg <= PhysicalReg::F31;
}

// 获取所有调用者保存的整数寄存器 (t0-t6, a0-a7)
inline const std::vector<PhysicalReg>& getCallerSavedIntRegs() {
    static const std::vector<PhysicalReg> regs = {
        PhysicalReg::T0, PhysicalReg::T1, PhysicalReg::T2, PhysicalReg::T3,
        PhysicalReg::T4, PhysicalReg::T5, PhysicalReg::T6,
        PhysicalReg::A0, PhysicalReg::A1, PhysicalReg::A2, PhysicalReg::A3,
        PhysicalReg::A4, PhysicalReg::A5, PhysicalReg::A6, PhysicalReg::A7
    };
    return regs;
}

// 获取所有被调用者保存的整数寄存器 (s0-s11)
inline const std::vector<PhysicalReg>& getCalleeSavedIntRegs() {
    static const std::vector<PhysicalReg> regs = {
        PhysicalReg::S0, PhysicalReg::S1, PhysicalReg::S2, PhysicalReg::S3,
        PhysicalReg::S4, PhysicalReg::S5, PhysicalReg::S6, PhysicalReg::S7,
        PhysicalReg::S8, PhysicalReg::S9, PhysicalReg::S10, PhysicalReg::S11
    };
    return regs;
}

// 获取所有调用者保存的浮点寄存器 (ft0-ft11, fa0-fa7)
inline const std::vector<PhysicalReg>& getCallerSavedFpRegs() {
    static const std::vector<PhysicalReg> regs = {
        PhysicalReg::F0, PhysicalReg::F1, PhysicalReg::F2, PhysicalReg::F3,
        PhysicalReg::F4, PhysicalReg::F5, PhysicalReg::F6, PhysicalReg::F7,
        PhysicalReg::F8, PhysicalReg::F9, PhysicalReg::F10, PhysicalReg::F11, // ft0-ft11 和 fa0-fa7 在标准ABI中重叠
        PhysicalReg::F12, PhysicalReg::F13, PhysicalReg::F14, PhysicalReg::F15,
        PhysicalReg::F16, PhysicalReg::F17
    };
    return regs;
}

// 获取所有被调用者保存的浮点寄存器 (fs0-fs11)
inline const std::vector<PhysicalReg>& getCalleeSavedFpRegs() {
    static const std::vector<PhysicalReg> regs = {
        PhysicalReg::F18, PhysicalReg::F19, PhysicalReg::F20, PhysicalReg::F21,
        PhysicalReg::F22, PhysicalReg::F23, PhysicalReg::F24, PhysicalReg::F25,
        PhysicalReg::F26, PhysicalReg::F27, PhysicalReg::F28, PhysicalReg::F29,
        PhysicalReg::F30, PhysicalReg::F31
    };
    return regs;
}

class MachineOperand;
class RegOperand;
class ImmOperand;
class LabelOperand;
class MemOperand;
class MachineInstr;
class MachineBasicBlock;
class MachineFunction;

// 操作数基类
class MachineOperand {
public:
    enum OperandKind { KIND_REG, KIND_IMM, KIND_LABEL, KIND_MEM };
    MachineOperand(OperandKind kind) : kind(kind) {}
    virtual ~MachineOperand() = default;
    OperandKind getKind() const { return kind; }
private:
    OperandKind kind;
};

// 寄存器操作数
class RegOperand : public MachineOperand {
public:
    // 构造虚拟寄存器
    RegOperand(unsigned vreg_num) 
        : MachineOperand(KIND_REG), vreg_num(vreg_num), is_virtual(true) {}

    // 构造物理寄存器
    RegOperand(PhysicalReg preg)
        : MachineOperand(KIND_REG), preg(preg), is_virtual(false) {}

    bool isVirtual() const { return is_virtual; }
    unsigned getVRegNum() const { return vreg_num; }
    PhysicalReg getPReg() const { return preg; }

    void setPReg(PhysicalReg new_preg) {
        preg = new_preg;
        is_virtual = false;
    }
    
    void setVRegNum(unsigned new_vreg_num) {
        vreg_num = new_vreg_num;
        is_virtual = true; // 确保设置vreg时，操作数状态正确
    }
private:
    unsigned vreg_num = 0;
    PhysicalReg preg = PhysicalReg::ZERO;
    bool is_virtual;
};

// 立即数操作数
class ImmOperand : public MachineOperand {
public:
    ImmOperand(int64_t value) : MachineOperand(KIND_IMM), value(value) {}
    int64_t getValue() const { return value; }
private:
    int64_t value;
};

// 标签操作数
class LabelOperand : public MachineOperand {
public:
    LabelOperand(const std::string& name) : MachineOperand(KIND_LABEL), name(name) {}
    const std::string& getName() const { return name; }
private:
    std::string name;
};

// 内存操作数, 表示 offset(base_reg)
class MemOperand : public MachineOperand {
public:
    MemOperand(std::unique_ptr<RegOperand> base, std::unique_ptr<ImmOperand> offset)
        : MachineOperand(KIND_MEM), base(std::move(base)), offset(std::move(offset)) {}
    RegOperand* getBase() const { return base.get(); }
    ImmOperand* getOffset() const { return offset.get(); }
private:
    std::unique_ptr<RegOperand> base;
    std::unique_ptr<ImmOperand> offset;
};

// 机器指令
class MachineInstr {
public:
    MachineInstr(RVOpcodes opcode) : opcode(opcode) {}

    RVOpcodes getOpcode() const { return opcode; }
    const std::vector<std::unique_ptr<MachineOperand>>& getOperands() const { return operands; }
    std::vector<std::unique_ptr<MachineOperand>>& getOperands() { return operands; }

    void addOperand(std::unique_ptr<MachineOperand> operand) {
        operands.push_back(std::move(operand));
    }
    /**
     * @brief （为紧急溢出模式添加）将指令中所有对特定虚拟寄存器的引用替换为指定的物理寄存器。
     * * @param old_vreg 需要被替换的虚拟寄存器号。
     * @param preg 用于替换的物理寄存器。
     */
    void replaceVRegWithPReg(unsigned old_vreg, PhysicalReg preg);

    /**
     * @brief （为常规溢出模式添加）根据提供的映射表，重映射指令中的虚拟寄存器。
     * * @param use_remap 一个从旧vreg到新vreg的映射，用于指令的use操作数。
     * @param def_remap 一个从旧vreg到新vreg的映射，用于指令的def操作数。
     */
    void remapVRegs(const std::map<unsigned, unsigned>& use_remap, const std::map<unsigned, unsigned>& def_remap);
private:
    RVOpcodes opcode;
    std::vector<std::unique_ptr<MachineOperand>> operands;
};

// 机器基本块
class MachineBasicBlock {
public:
    MachineBasicBlock(const std::string& name, MachineFunction* parent) 
        : name(name), parent(parent) {}

    const std::string& getName() const { return name; }
    MachineFunction* getParent() const { return parent; }
    const std::vector<std::unique_ptr<MachineInstr>>& getInstructions() const { return instructions; }
    std::vector<std::unique_ptr<MachineInstr>>& getInstructions() { return instructions; }
    
    void addInstruction(std::unique_ptr<MachineInstr> instr) {
        instructions.push_back(std::move(instr));
    }
    
    std::vector<MachineBasicBlock*> successors;
    std::vector<MachineBasicBlock*> predecessors;
private:
    std::string name;
    std::vector<std::unique_ptr<MachineInstr>> instructions;
    MachineFunction* parent;
};

// 栈帧信息
struct StackFrameInfo {
    int locals_size = 0; // 仅为AllocaInst分配的大小
    int locals_end_offset = 0; // 记录局部变量分配结束后的偏移量(相对于s0，为负)
    int spill_size = 0; // 仅为溢出分配的大小
    int total_size = 0; // 总大小
    int callee_saved_size = 0; // 保存寄存器的大小
    std::map<unsigned, int> alloca_offsets; // <AllocaInst的vreg, 栈偏移>
    std::map<unsigned, int> spill_offsets;  // <溢出vreg, 栈偏移>
    std::set<PhysicalReg> used_callee_saved_regs; // 使用的保存寄存器
    std::map<unsigned, PhysicalReg> vreg_to_preg_map; // RegAlloc最终的分配结果
    std::vector<PhysicalReg> callee_saved_regs_to_store; // 已排序的、需要存取的被调用者保存寄存器
};

// 机器函数
class MachineFunction {
public:
    MachineFunction(Function* func, RISCv64ISel* isel) : F(func), name(func->getName()), isel(isel) {}

    Function* getFunc() const { return F; }
    RISCv64ISel* getISel() const { return isel; }
    const std::string& getName() const { return name; }
    StackFrameInfo& getFrameInfo() { return frame_info; }
    const std::vector<std::unique_ptr<MachineBasicBlock>>& getBlocks() const { return blocks; }
    std::vector<std::unique_ptr<MachineBasicBlock>>& getBlocks() { return blocks; }
    void dumpStackFrameInfo(std::ostream& os = std::cerr) const;
    void addBlock(std::unique_ptr<MachineBasicBlock> block) {
        blocks.push_back(std::move(block));
    }
    void addProtectedArgumentVReg(unsigned vreg) {
        protected_argument_vregs.insert(vreg);
    }
    const std::set<unsigned>& getProtectedArgumentVRegs() const {
        return protected_argument_vregs;
    }
private:
    Function* F;
    RISCv64ISel* isel; // 指向创建它的ISel，用于获取vreg映射等信息
    std::string name;
    std::vector<std::unique_ptr<MachineBasicBlock>> blocks;
    StackFrameInfo frame_info;
    std::set<unsigned> protected_argument_vregs;
};
inline bool isMemoryOp(RVOpcodes opcode) {
    switch (opcode) {
        case RVOpcodes::LB: case RVOpcodes::LH: case RVOpcodes::LW: case RVOpcodes::LD:
        case RVOpcodes::LBU: case RVOpcodes::LHU: case RVOpcodes::LWU:
        case RVOpcodes::SB: case RVOpcodes::SH: case RVOpcodes::SW: case RVOpcodes::SD:
        case RVOpcodes::FLW:
        case RVOpcodes::FSW:
        case RVOpcodes::FLD:
        case RVOpcodes::FSD:
            return true;
        default:
            return false;
    }
}

void getInstrUseDef(const MachineInstr* instr, std::set<unsigned>& use, std::set<unsigned>& def);

} // namespace sysy

#endif // RISCV64_LLIR_H