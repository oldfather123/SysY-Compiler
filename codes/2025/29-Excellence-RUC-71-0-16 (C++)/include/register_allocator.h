#pragma once

#include "irtranslater.h"
#include "riscv_instruction.h"
#include <algorithm>
#include <atomic>
#include <map>
#include <set>
#include <vector>

// 虚拟寄存器的生存期信息
struct LiveRange {
  int virtual_reg;
  int start_pos;   // 第一次定义的位置
  int end_pos;     // 最后一次使用的位置
  bool is_spilled; // 是否被溢出到内存
  int spill_slot;  // 溢出槽位置（如果被溢出）

  LiveRange(int reg, int start, int end)
      : virtual_reg(reg), start_pos(start), end_pos(end), is_spilled(false),
        spill_slot(-1) {}
};

// RISC-V寄存器信息
struct PhysicalRegister {
  int reg_no;           // 寄存器编号
  std::string name;     // 寄存器名称
  bool is_callee_saved; // 是否为被调用方保存寄存器
  bool is_available;    // 是否可用于分配

  PhysicalRegister(int no, const std::string &n, bool callee_saved)
      : reg_no(no), name(n), is_callee_saved(callee_saved), is_available(true) {
  }
};

class RegisterAllocator {
private:
  // RISC-V物理寄存器定义
  std::vector<PhysicalRegister> available_registers;

  // 寄存器分配映射
  std::map<int, int> virtual_to_physical; // 虚拟寄存器 -> 物理寄存器
  std::map<int, int>
      physical_to_virtual; // 物理寄存器 -> 虚拟寄存器（当前占用）

  // 虚拟寄存器类型跟踪 (新增)
  std::map<int, bool> virtual_reg_is_float; // 虚拟寄存器 -> 是否为浮点类型

  // 溢出管理
  std::set<int> spilled_virtuals; // 被溢出的虚拟寄存器
  std::map<int, int> spill_slots; // 虚拟寄存器 -> 溢出槽偏移
  int next_spill_slot;            // 下一个可用的溢出槽

  // 生存期分析
  std::vector<LiveRange> live_ranges;
  std::map<int, LiveRange *> virtual_to_range;

  // 当前函数的栈帧信息
  StackFrameInfo *current_frame;

  // 取消标志
  const std::atomic<bool> *cancel_flag;

public:
  RegisterAllocator();

  // 主要接口
  void allocateRegistersForFunction(const std::string &func_name,
                                    std::map<int, RiscvBlock *> &blocks,
                                    StackFrameInfo *frame_info);
  void allocateRegistersForFunction(const std::string &func_name,
                                    std::map<int, RiscvBlock *> &blocks,
                                    StackFrameInfo *frame_info,
                                    const std::atomic<bool> &should_cancel);

  // 寄存器查询
  int getPhysicalRegister(int virtual_reg);
  bool isSpilled(int virtual_reg);
  int getSpillOffset(int virtual_reg);

  // 生存期分析
  void computeLiveRanges(const std::map<int, RiscvBlock *> &blocks);

  // 寄存器分配算法
  void preAllocateSpecialRegisters(const std::map<int, RiscvBlock *> &blocks);
  void preAllocateFunctionCallArguments(
      const std::map<int, RiscvBlock *> &blocks); // 新增：函数调用参数预分配
  void performLinearScanAllocation();
  void allocateByCategory();             // 新增：按类型分配
  bool isLocalVariable(int virtual_reg); // 新增：判断是否为局部变量
  void allocateRangesWithRegisters(
      std::vector<LiveRange *> &ranges,
      std::set<int> &reg_set); // 新增：为特定范围分配寄存器
  void insertSpillCode(std::map<int, RiscvBlock *> &blocks);

  // 指令重写
  void rewriteInstructions(std::map<int, RiscvBlock *> &blocks);

  // 工具方法
  void initializePhysicalRegisters();
  int findFreeRegister();
  int selectVictimRegister(int current_pos);
  void spillRegister(int physical_reg, int current_pos);
  std::vector<int> extractVirtualRegisters(RiscvInstruction *inst);
  void rewriteOperand(RiscvOperand *&operand);
  void rewriteOperandSafe(RiscvOperand *&operand,
                          std::set<RiscvOperand *> &rewritten_operands);
  void rewriteOperandNew(RiscvOperand *&operand,
                         const std::map<int, int> &vreg_to_temp_phys_reg);
  bool usesVirtualRegister(RiscvInstruction *inst, int virtual_reg);
  bool definesVirtualRegister(RiscvInstruction *inst, int virtual_reg);

  // 新增：虚拟寄存器类型相关方法
  void analyzeVirtualRegisterTypes(const std::map<int, RiscvBlock *> &blocks);
  bool isFloatVirtualRegister(int virtual_reg);
  bool isFloatPhysicalRegister(int physical_reg);
  int findFreeFloatRegister();
  int findFreeIntegerRegister();
  void markFloatRegister(RiscvOperand *operand);
  void markIntegerRegister(RiscvOperand *operand);

  // 调试和统计
  void printAllocationResult();
  void printLiveRanges();

  // 调试和状态查询方法
  int getAvailableRegistersCount() const { return available_registers.size(); }
  int getVirtualToPhysicalMappingCount() const {
    return virtual_to_physical.size();
  }
  int getSpilledVirtualsCount() const { return spilled_virtuals.size(); }
  int getLiveRangesCount() const { return live_ranges.size(); }
};

// 寄存器分配器集成到翻译器的接口
class RegisterAllocationPass {
public:
  static void applyToTranslator(Translator &translator);
  static void applyToTranslator(Translator &translator,
                                const std::atomic<bool> &should_cancel);
};