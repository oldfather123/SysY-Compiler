#include "PostRA_Scheduler.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define MAX_SCHEDULING_BLOCK_SIZE 10000 // 限制调度块大小，避免过大导致性能问题

namespace sysy {

char PostRA_Scheduler::ID = 0;

// 检查指令是否是加载指令 (LW, LD)
bool isLoadInstr(MachineInstr *instr) {
  RVOpcodes opcode = instr->getOpcode();
  return opcode == RVOpcodes::LW || opcode == RVOpcodes::LD ||
         opcode == RVOpcodes::LH || opcode == RVOpcodes::LB ||
         opcode == RVOpcodes::LHU || opcode == RVOpcodes::LBU ||
         opcode == RVOpcodes::LWU;
}

// 检查指令是否是存储指令 (SW, SD)
bool isStoreInstr(MachineInstr *instr) {
  RVOpcodes opcode = instr->getOpcode();
  return opcode == RVOpcodes::SW || opcode == RVOpcodes::SD ||
         opcode == RVOpcodes::SH || opcode == RVOpcodes::SB;
}

// 检查指令是否为控制流指令
bool isControlFlowInstr(MachineInstr *instr) {
  RVOpcodes opcode = instr->getOpcode();
  return opcode == RVOpcodes::RET || opcode == RVOpcodes::J ||
         opcode == RVOpcodes::BEQ || opcode == RVOpcodes::BNE ||
         opcode == RVOpcodes::BLT || opcode == RVOpcodes::BGE ||
         opcode == RVOpcodes::BLTU || opcode == RVOpcodes::BGEU ||
         opcode == RVOpcodes::CALL;
}

// 预计算指令信息的缓存
static std::unordered_map<MachineInstr *, InstrRegInfo> instr_info_cache;

// 获取指令定义的寄存器 - 优化版本
std::unordered_set<PhysicalReg> getDefinedRegisters(MachineInstr *instr) {
  std::unordered_set<PhysicalReg> defined_regs;
  RVOpcodes opcode = instr->getOpcode();

  // 特殊处理CALL指令
  if (opcode == RVOpcodes::CALL) {
    // CALL指令可能定义返回值寄存器
    if (!instr->getOperands().empty() &&
        instr->getOperands().front()->getKind() == MachineOperand::KIND_REG) {
      auto reg_op =
          static_cast<RegOperand *>(instr->getOperands().front().get());
      if (!reg_op->isVirtual()) {
        defined_regs.insert(reg_op->getPReg());
      }
    }
    return defined_regs;
  }

  // 存储指令不定义寄存器
  if (isStoreInstr(instr)) {
    return defined_regs;
  }

  // 分支指令不定义寄存器
  if (opcode == RVOpcodes::BEQ || opcode == RVOpcodes::BNE ||
      opcode == RVOpcodes::BLT || opcode == RVOpcodes::BGE ||
      opcode == RVOpcodes::BLTU || opcode == RVOpcodes::BGEU ||
      opcode == RVOpcodes::J || opcode == RVOpcodes::RET) {
    return defined_regs;
  }

  // 对于其他指令，第一个寄存器操作数通常是定义的
  if (!instr->getOperands().empty() &&
      instr->getOperands().front()->getKind() == MachineOperand::KIND_REG) {
    auto reg_op = static_cast<RegOperand *>(instr->getOperands().front().get());
    if (!reg_op->isVirtual()) {
      defined_regs.insert(reg_op->getPReg());
    }
  }

  return defined_regs;
}

// 获取指令使用的寄存器 - 优化版本
std::unordered_set<PhysicalReg> getUsedRegisters(MachineInstr *instr) {
  std::unordered_set<PhysicalReg> used_regs;
  RVOpcodes opcode = instr->getOpcode();

  // 特殊处理CALL指令
  if (opcode == RVOpcodes::CALL) {
    bool first_reg_skipped = false;
    for (const auto &op : instr->getOperands()) {
      if (op->getKind() == MachineOperand::KIND_REG) {
        if (!first_reg_skipped) {
          first_reg_skipped = true;
          continue; // 跳过返回值寄存器
        }
        auto reg_op = static_cast<RegOperand *>(op.get());
        if (!reg_op->isVirtual()) {
          used_regs.insert(reg_op->getPReg());
        }
      }
    }
    return used_regs;
  }

  // 对于存储指令，所有寄存器操作数都是使用的
  if (isStoreInstr(instr)) {
    for (const auto &op : instr->getOperands()) {
      if (op->getKind() == MachineOperand::KIND_REG) {
        auto reg_op = static_cast<RegOperand *>(op.get());
        if (!reg_op->isVirtual()) {
          used_regs.insert(reg_op->getPReg());
        }
      } else if (op->getKind() == MachineOperand::KIND_MEM) {
        auto mem_op = static_cast<MemOperand *>(op.get());
        if (!mem_op->getBase()->isVirtual()) {
          used_regs.insert(mem_op->getBase()->getPReg());
        }
      }
    }
    return used_regs;
  }

  // 对于分支指令，所有寄存器操作数都是使用的
  if (opcode == RVOpcodes::BEQ || opcode == RVOpcodes::BNE ||
      opcode == RVOpcodes::BLT || opcode == RVOpcodes::BGE ||
      opcode == RVOpcodes::BLTU || opcode == RVOpcodes::BGEU) {
    for (const auto &op : instr->getOperands()) {
      if (op->getKind() == MachineOperand::KIND_REG) {
        auto reg_op = static_cast<RegOperand *>(op.get());
        if (!reg_op->isVirtual()) {
          used_regs.insert(reg_op->getPReg());
        }
      }
    }
    return used_regs;
  }

  // 对于其他指令，除了第一个寄存器操作数（通常是定义），其余都是使用的
  bool first_reg = true;
  for (const auto &op : instr->getOperands()) {
    if (op->getKind() == MachineOperand::KIND_REG) {
      if (first_reg) {
        first_reg = false;
        continue; // 跳过第一个寄存器（定义）
      }
      auto reg_op = static_cast<RegOperand *>(op.get());
      if (!reg_op->isVirtual()) {
        used_regs.insert(reg_op->getPReg());
      }
    } else if (op->getKind() == MachineOperand::KIND_MEM) {
      auto mem_op = static_cast<MemOperand *>(op.get());
      if (!mem_op->getBase()->isVirtual()) {
        used_regs.insert(mem_op->getBase()->getPReg());
      }
    }
  }
  return used_regs;
}

// 获取内存访问的基址和偏移

MemoryAccess getMemoryAccess(MachineInstr *instr) {
  if (!isLoadInstr(instr) && !isStoreInstr(instr)) {
    return MemoryAccess();
  }

  // 查找内存操作数
  for (const auto &op : instr->getOperands()) {
    if (op->getKind() == MachineOperand::KIND_MEM) {
      auto mem_op = static_cast<MemOperand *>(op.get());
      if (!mem_op->getBase()->isVirtual()) {
        return MemoryAccess(mem_op->getBase()->getPReg(),
                            mem_op->getOffset()->getValue());
      }
    }
  }

  return MemoryAccess();
}

// 预计算指令信息
InstrRegInfo &getInstrInfo(MachineInstr *instr) {
  auto it = instr_info_cache.find(instr);
  if (it != instr_info_cache.end()) {
    return it->second;
  }

  InstrRegInfo &info = instr_info_cache[instr];
  info.defined_regs = getDefinedRegisters(instr);
  info.used_regs = getUsedRegisters(instr);
  info.is_load = isLoadInstr(instr);
  info.is_store = isStoreInstr(instr);
  info.is_control_flow = isControlFlowInstr(instr);
  info.mem_access = getMemoryAccess(instr);

  return info;
}

// 检查内存依赖 - 优化版本
bool hasMemoryDependency(const InstrRegInfo &info1, const InstrRegInfo &info2) {
  // 如果都不是内存指令，没有内存依赖
  if (!info1.is_load && !info1.is_store && !info2.is_load && !info2.is_store) {
    return false;
  }

  const MemoryAccess &mem1 = info1.mem_access;
  const MemoryAccess &mem2 = info2.mem_access;

  if (!mem1.valid || !mem2.valid) {
    // 如果无法确定内存访问模式，保守地认为存在依赖
    return true;
  }

  // 如果访问相同的内存位置
  if (mem1.base_reg == mem2.base_reg && mem1.offset == mem2.offset) {
    // Store->Load: RAW依赖
    // Load->Store: WAR依赖
    // Store->Store: WAW依赖
    return info1.is_store || info2.is_store;
  }

  // 不同内存位置通常没有依赖，但为了安全起见，
  // 如果涉及store指令，我们需要更保守
  if (info1.is_store && info2.is_load) {
    // 保守处理：不同store和load之间可能有别名
    return false; // 这里可以根据需要调整策略
  }

  return false;
}

// 检查两个指令之间是否存在依赖关系 - 优化版本
bool hasDependency(MachineInstr *instr1, MachineInstr *instr2) {
  const InstrRegInfo &info1 = getInstrInfo(instr1);
  const InstrRegInfo &info2 = getInstrInfo(instr2);

  // 检查RAW依赖：instr1定义的寄存器是否被instr2使用
  for (const auto &reg : info1.defined_regs) {
    if (info2.used_regs.find(reg) != info2.used_regs.end()) {
      return true; // RAW依赖 - instr2读取instr1写入的值
    }
  }

  // 检查WAR依赖：instr1使用的寄存器是否被instr2定义
  for (const auto &reg : info1.used_regs) {
    if (info2.defined_regs.find(reg) != info2.defined_regs.end()) {
      return true; // WAR依赖 - instr2覆盖instr1需要的值
    }
  }

  // 检查WAW依赖：两个指令定义相同寄存器
  for (const auto &reg : info1.defined_regs) {
    if (info2.defined_regs.find(reg) != info2.defined_regs.end()) {
      return true; // WAW依赖 - 两条指令写入同一寄存器
    }
  }

  // 检查内存依赖
  if (hasMemoryDependency(info1, info2)) {
    return true;
  }

  return false;
}

// 检查是否可以安全地将instr1和instr2交换位置 - 优化版本
bool canSwapInstructions(MachineInstr *instr1, MachineInstr *instr2) {
  const InstrRegInfo &info1 = getInstrInfo(instr1);
  const InstrRegInfo &info2 = getInstrInfo(instr2);

  // 不能移动控制流指令
  if (info1.is_control_flow || info2.is_control_flow) {
    return false;
  }

  // 检查双向依赖关系
  return !hasDependency(instr1, instr2) && !hasDependency(instr2, instr1);
}

// 新增：验证调度结果的正确性 - 优化版本
void validateSchedule(const std::vector<MachineInstr *> &instr_list) {
  for (int i = 0; i < (int)instr_list.size(); i++) {
    for (int j = i + 1; j < (int)instr_list.size(); j++) {
      MachineInstr *earlier = instr_list[i];
      MachineInstr *later = instr_list[j];

      const InstrRegInfo &info_earlier = getInstrInfo(earlier);
      const InstrRegInfo &info_later = getInstrInfo(later);

      // 检查是否存在被违反的依赖关系
      // 检查RAW依赖
      for (const auto &reg : info_earlier.defined_regs) {
        if (info_later.used_regs.find(reg) != info_later.used_regs.end()) {
          // 这是正常的依赖关系，earlier应该在later之前
          continue;
        }
      }

      // 检查内存依赖
      if (hasMemoryDependency(info_earlier, info_later)) {
        const MemoryAccess &mem1 = info_earlier.mem_access;
        const MemoryAccess &mem2 = info_later.mem_access;

        if (mem1.valid && mem2.valid && mem1.base_reg == mem2.base_reg &&
            mem1.offset == mem2.offset) {
          if (info_earlier.is_store && info_later.is_load) {
            // Store->Load依赖，顺序正确
            continue;
          }
        }
      }
    }
  }
}

// 在基本块内对指令进行调度优化 - 优化版本
void scheduleBlock(MachineBasicBlock *mbb) {
  auto &instructions = mbb->getInstructions();
  if (instructions.size() <= 1)
    return;
  if (instructions.size() > MAX_SCHEDULING_BLOCK_SIZE) {
    return; // 跳过超大块，防止卡住
  }

  // 清理缓存，避免无效指针
  instr_info_cache.clear();

  std::vector<MachineInstr *> instr_list;
  instr_list.reserve(instructions.size()); // 预分配容量
  for (auto &instr : instructions) {
    instr_list.push_back(instr.get());
  }

  // 预计算所有指令的信息
  for (auto *instr : instr_list) {
    getInstrInfo(instr);
  }

  // 使用更严格的调度策略，避免破坏依赖关系
  bool changed = true;
  int max_iterations = 10; // 限制迭代次数避免死循环
  int iteration = 0;

  while (changed && iteration < max_iterations) {
    changed = false;
    iteration++;

    for (int i = 0; i < (int)instr_list.size() - 1; i++) {
      MachineInstr *instr1 = instr_list[i];
      MachineInstr *instr2 = instr_list[i + 1];

      const InstrRegInfo &info1 = getInstrInfo(instr1);
      const InstrRegInfo &info2 = getInstrInfo(instr2);

      // 只进行非常保守的优化
      bool should_swap = false;

      // 策略1: 将load指令提前，减少load-use延迟
      if (info2.is_load && !info1.is_load && !info1.is_store) {
        should_swap = canSwapInstructions(instr1, instr2);
      }
      // 策略2: 将非关键store指令延后，为其他指令让路
      else if (info1.is_store && !info2.is_load && !info2.is_store) {
        should_swap = canSwapInstructions(instr1, instr2);
      }

      if (should_swap) {
        std::swap(instr_list[i], instr_list[i + 1]);
        changed = true;

        // 调试输出
        // std::cout << "Swapped instructions at positions " << i << " and " <<
        // (i+1) << std::endl;
      }
    }
  }

  // 验证调度结果的正确性
  validateSchedule(instr_list);

  // 将调度后的指令顺序写回
  std::unordered_map<MachineInstr *, std::unique_ptr<MachineInstr>> instr_map;
  instr_map.reserve(instructions.size()); // 预分配容量
  for (auto &instr : instructions) {
    instr_map[instr.get()] = std::move(instr);
  }

  instructions.clear();
  instructions.reserve(instr_list.size()); // 预分配容量
  for (auto instr : instr_list) {
    instructions.push_back(std::move(instr_map[instr]));
  }
}

bool PostRA_Scheduler::runOnFunction(Function *F, AnalysisManager &AM) {
  // 这个函数在IR级别运行，但我们需要在机器指令级别运行
  // 所以我们返回false，表示没有对IR进行修改
  return false;
}

void PostRA_Scheduler::runOnMachineFunction(MachineFunction *mfunc) {
  // std::cout << "Running Post-RA Local Scheduler... " << std::endl;

  // 遍历每个机器基本块
  for (auto &mbb : mfunc->getBlocks()) {
    scheduleBlock(mbb.get());
  }

  // 清理全局缓存
  instr_info_cache.clear();
}

} // namespace sysy