#pragma once
#include "../ir/llvm_ir.h"
#include "function_reg_alloc.h"
#include "../riscv/riscv.h"
#include <vector>
#include <string>

namespace llvm_ir {

// 溢出/重载指令类型
enum class SpillReloadType {
    SPILL,    // 寄存器 -> 栈
    RELOAD    // 栈 -> 寄存器
};

// 溢出/重载指令信息
struct SpillReloadInst {
    SpillReloadType type;
    Value* vreg;           // 虚拟寄存器
    riscv::reg reg;        // 物理寄存器
    int stack_offset;      // 栈偏移
    Type data_type;        // 数据类型
    int instruction_pos;   // 指令位置（用于插入）
    
    SpillReloadInst(SpillReloadType t, Value* v, riscv::reg r, int offset, Type dt, int pos)
        : type(t), vreg(v), reg(r), stack_offset(offset), data_type(dt), instruction_pos(pos) {}
};

// 溢出/重载指令生成器
class SpillReloadGenerator {
private:
    Function* func;
    const FunctionRegAllocResult& alloc_result;
    
    // 收集的溢出/重载指令
    std::vector<SpillReloadInst> spill_reload_insts;

public:
    SpillReloadGenerator(Function* f, const FunctionRegAllocResult& result)
        : func(f), alloc_result(result) {}
    
    // 分析并生成溢出/重载指令
    void generateSpillReloadInstructions();
    
    // 获取生成的指令列表
    const std::vector<SpillReloadInst>& getInstructions() const { return spill_reload_insts; }
    
    // 生成RISC-V汇编代码
    std::string generateRiscvCode() const;
    
    // 打印指令信息（调试用）
    void printInstructions() const;

private:
    // 分析指令的寄存器使用
    void analyzeInstruction(Instruction* inst, int pos);
    
    // 检查是否需要溢出
    bool needsSpill(Value* vreg, int pos) const;
    
    // 检查是否需要重载
    bool needsReload(Value* vreg, int pos) const;
    
    // 生成单个溢出指令的RISC-V代码
    std::string generateSpillCode(const SpillReloadInst& inst) const;
    
    // 生成单个重载指令的RISC-V代码
    std::string generateReloadCode(const SpillReloadInst& inst) const;
    
    // 获取RISC-V寄存器名
    std::string getRegName(riscv::reg reg) const;
    
    // 获取数据类型对应的RISC-V指令
    std::string getLoadStoreOp(Type type) const;
};

// 全局函数：生成溢出/重载指令
std::vector<SpillReloadInst> generateSpillReloadInstructions(Function* func, const FunctionRegAllocResult& alloc_result);

} // namespace llvm_ir 