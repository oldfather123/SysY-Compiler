#include "PreRA_Scheduler.h"
#include "RISCv64LLIR.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define MAX_SCHEDULING_BLOCK_SIZE 1000 // 严格限制调度块大小

namespace sysy {

char PreRA_Scheduler::ID = 0;

// 检查指令是否是加载指令 (LW, LD)
static bool isLoadInstr(MachineInstr *instr) {
  RVOpcodes opcode = instr->getOpcode();
  return opcode == RVOpcodes::LW || opcode == RVOpcodes::LD ||
         opcode == RVOpcodes::LH || opcode == RVOpcodes::LB ||
         opcode == RVOpcodes::LHU || opcode == RVOpcodes::LBU ||
         opcode == RVOpcodes::LWU;
}

// 检查指令是否是存储指令 (SW, SD)
static bool isStoreInstr(MachineInstr *instr) {
  RVOpcodes opcode = instr->getOpcode();
  return opcode == RVOpcodes::SW || opcode == RVOpcodes::SD ||
         opcode == RVOpcodes::SH || opcode == RVOpcodes::SB;
}

// 检查指令是否为分支指令
static bool isBranchInstr(MachineInstr *instr) {
  RVOpcodes opcode = instr->getOpcode();
  return opcode == RVOpcodes::BEQ || opcode == RVOpcodes::BNE ||
         opcode == RVOpcodes::BLT || opcode == RVOpcodes::BGE ||
         opcode == RVOpcodes::BLTU || opcode == RVOpcodes::BGEU;
}

// 检查指令是否为跳转指令
static bool isJumpInstr(MachineInstr *instr) {
  RVOpcodes opcode = instr->getOpcode();
  return opcode == RVOpcodes::J;
}

// 检查指令是否为返回指令
static bool isReturnInstr(MachineInstr *instr) {
  return instr->getOpcode() == RVOpcodes::RET;
}

// 检查指令是否为调用指令
static bool isCallInstr(MachineInstr *instr) {
  return instr->getOpcode() == RVOpcodes::CALL;
}

// 检查指令是否为块终结指令（必须保持在块尾）
static bool isTerminatorInstr(MachineInstr *instr) {
  return isBranchInstr(instr) || isJumpInstr(instr) || isReturnInstr(instr);
}

// 检查指令是否有副作用（需要谨慎处理）
static bool hasSideEffect(MachineInstr *instr) {
  return isStoreInstr(instr) || isCallInstr(instr) || isTerminatorInstr(instr);
}

// 检查指令是否涉及内存操作
static bool hasMemoryAccess(MachineInstr *instr) {
  return isLoadInstr(instr) || isStoreInstr(instr);
}

// 获取内存访问位置信息
struct MemoryLocation {
  unsigned base_reg;
  int64_t offset;
  bool is_valid;

  MemoryLocation() : base_reg(0), offset(0), is_valid(false) {}
  MemoryLocation(unsigned base, int64_t off)
      : base_reg(base), offset(off), is_valid(true) {}

  bool operator==(const MemoryLocation &other) const {
    return is_valid && other.is_valid && base_reg == other.base_reg &&
           offset == other.offset;
  }
};

// 缓存指令分析信息
struct InstrInfo {
  std::unordered_set<unsigned> defined_regs;
  std::unordered_set<unsigned> used_regs;
  MemoryLocation mem_location;
  bool is_load;
  bool is_store;
  bool is_terminator;
  bool is_call;
  bool has_side_effect;
  bool has_memory_access;
  
  InstrInfo() : is_load(false), is_store(false), is_terminator(false), 
                is_call(false), has_side_effect(false), has_memory_access(false) {}
};

// 指令信息缓存
static std::unordered_map<MachineInstr*, InstrInfo> instr_info_cache;

// 获取指令定义的虚拟寄存器 - 优化版本
static std::unordered_set<unsigned> getDefinedVirtualRegisters(MachineInstr *instr) {
  std::unordered_set<unsigned> defined_regs;
  RVOpcodes opcode = instr->getOpcode();

  // CALL指令可能定义返回值寄存器
  if (opcode == RVOpcodes::CALL) {
    if (!instr->getOperands().empty() &&
        instr->getOperands().front()->getKind() == MachineOperand::KIND_REG) {
      auto reg_op =
          static_cast<RegOperand *>(instr->getOperands().front().get());
      if (reg_op->isVirtual()) {
        defined_regs.insert(reg_op->getVRegNum());
      }
    }
    return defined_regs;
  }

  // 存储指令和终结指令不定义寄存器
  if (isStoreInstr(instr) || isTerminatorInstr(instr)) {
    return defined_regs;
  }

  // 其他指令的第一个操作数通常是目标寄存器
  if (!instr->getOperands().empty() &&
      instr->getOperands().front()->getKind() == MachineOperand::KIND_REG) {
    auto reg_op = static_cast<RegOperand *>(instr->getOperands().front().get());
    if (reg_op->isVirtual()) {
      defined_regs.insert(reg_op->getVRegNum());
    }
  }

  return defined_regs;
}

// 获取指令使用的虚拟寄存器 - 优化版本
static std::unordered_set<unsigned> getUsedVirtualRegisters(MachineInstr *instr) {
  std::unordered_set<unsigned> used_regs;
  RVOpcodes opcode = instr->getOpcode();

  // CALL指令：跳过第一个操作数（返回值），其余为参数
  if (opcode == RVOpcodes::CALL) {
    bool first_reg_skipped = false;
    for (const auto &op : instr->getOperands()) {
      if (op->getKind() == MachineOperand::KIND_REG) {
        if (!first_reg_skipped) {
          first_reg_skipped = true;
          continue;
        }
        auto reg_op = static_cast<RegOperand *>(op.get());
        if (reg_op->isVirtual()) {
          used_regs.insert(reg_op->getVRegNum());
        }
      }
    }
    return used_regs;
  }

  // 存储指令和终结指令：所有操作数都是使用的
  if (isStoreInstr(instr) || isTerminatorInstr(instr)) {
    for (const auto &op : instr->getOperands()) {
      if (op->getKind() == MachineOperand::KIND_REG) {
        auto reg_op = static_cast<RegOperand *>(op.get());
        if (reg_op->isVirtual()) {
          used_regs.insert(reg_op->getVRegNum());
        }
      } else if (op->getKind() == MachineOperand::KIND_MEM) {
        auto mem_op = static_cast<MemOperand *>(op.get());
        if (mem_op->getBase()->isVirtual()) {
          used_regs.insert(mem_op->getBase()->getVRegNum());
        }
      }
    }
    return used_regs;
  }

  // 其他指令：跳过第一个操作数（目标寄存器），其余为源操作数
  bool first_reg = true;
  for (const auto &op : instr->getOperands()) {
    if (op->getKind() == MachineOperand::KIND_REG) {
      if (first_reg) {
        first_reg = false;
        continue;
      }
      auto reg_op = static_cast<RegOperand *>(op.get());
      if (reg_op->isVirtual()) {
        used_regs.insert(reg_op->getVRegNum());
      }
    } else if (op->getKind() == MachineOperand::KIND_MEM) {
      auto mem_op = static_cast<MemOperand *>(op.get());
      if (mem_op->getBase()->isVirtual()) {
        used_regs.insert(mem_op->getBase()->getVRegNum());
      }
    }
  }
  return used_regs;
}

// 获取内存访问位置
static MemoryLocation getMemoryLocation(MachineInstr *instr) {
  if (!isLoadInstr(instr) && !isStoreInstr(instr)) {
    return MemoryLocation();
  }

  for (const auto &op : instr->getOperands()) {
    if (op->getKind() == MachineOperand::KIND_MEM) {
      auto mem_op = static_cast<MemOperand *>(op.get());
      if (mem_op->getBase()->isVirtual()) {
        return MemoryLocation(mem_op->getBase()->getVRegNum(),
                              mem_op->getOffset()->getValue());
      }
    }
  }

  return MemoryLocation();
}

// 预计算并缓存指令信息
static const InstrInfo& getInstrInfo(MachineInstr *instr) {
  auto it = instr_info_cache.find(instr);
  if (it != instr_info_cache.end()) {
    return it->second;
  }
  
  InstrInfo& info = instr_info_cache[instr];
  info.defined_regs = getDefinedVirtualRegisters(instr);
  info.used_regs = getUsedVirtualRegisters(instr);
  info.mem_location = getMemoryLocation(instr);
  info.is_load = isLoadInstr(instr);
  info.is_store = isStoreInstr(instr);
  info.is_terminator = isTerminatorInstr(instr);
  info.is_call = isCallInstr(instr);
  info.has_side_effect = hasSideEffect(instr);
  info.has_memory_access = hasMemoryAccess(instr);
  
  return info;
}

// 检查两个内存位置是否可能别名
static bool mayAlias(const MemoryLocation &loc1, const MemoryLocation &loc2) {
  if (!loc1.is_valid || !loc2.is_valid) {
    return true; // 保守处理：未知位置可能别名
  }

  // 不同基址寄存器，保守假设可能别名
  if (loc1.base_reg != loc2.base_reg) {
    return true;
  }

  // 相同基址寄存器，检查偏移
  return loc1.offset == loc2.offset;
}

// 检查两个指令之间是否存在数据依赖 - 优化版本
static bool hasDataDependency(MachineInstr *first, MachineInstr *second) {
  const InstrInfo& info_first = getInstrInfo(first);
  const InstrInfo& info_second = getInstrInfo(second);

  // RAW依赖: second读取first写入的寄存器
  for (const auto &reg : info_first.defined_regs) {
    if (info_second.used_regs.find(reg) != info_second.used_regs.end()) {
      return true;
    }
  }

  // WAR依赖: second写入first读取的寄存器
  for (const auto &reg : info_first.used_regs) {
    if (info_second.defined_regs.find(reg) != info_second.defined_regs.end()) {
      return true;
    }
  }

  // WAW依赖: 两个指令写入同一寄存器
  for (const auto &reg : info_first.defined_regs) {
    if (info_second.defined_regs.find(reg) != info_second.defined_regs.end()) {
      return true;
    }
  }

  return false;
}

// 检查两个指令之间是否存在内存依赖 - 优化版本
static bool hasMemoryDependency(MachineInstr *first, MachineInstr *second) {
  const InstrInfo& info_first = getInstrInfo(first);
  const InstrInfo& info_second = getInstrInfo(second);

  if (!info_first.has_memory_access || !info_second.has_memory_access) {
    return false;
  }

  // 如果至少有一个是存储指令，需要检查别名
  if (info_first.is_store || info_second.is_store) {
    return mayAlias(info_first.mem_location, info_second.mem_location);
  }

  return false; // 两个加载指令之间没有依赖
}

// 检查两个指令之间是否存在控制依赖 - 优化版本
static bool hasControlDependency(MachineInstr *first, MachineInstr *second) {
  const InstrInfo& info_first = getInstrInfo(first);
  const InstrInfo& info_second = getInstrInfo(second);

  // 终结指令与任何其他指令都有控制依赖
  if (info_first.is_terminator) {
    return true; // first是终结指令，second不能移动到first之前
  }

  if (info_second.is_terminator) {
    return false; // second是终结指令，可以保持在后面
  }

  // CALL指令具有控制副作用，但可以参与有限的调度
  if (info_first.is_call || info_second.is_call) {
    // CALL指令之间保持顺序
    if (info_first.is_call && info_second.is_call) {
      return true;
    }
    // 其他情况允许调度（通过数据依赖控制）
  }

  return false;
}

// 综合检查两个指令是否可以交换 - 优化版本
static bool canSwapInstructions(MachineInstr *first, MachineInstr *second) {
  // 检查所有类型的依赖
  if (hasDataDependency(first, second) || hasDataDependency(second, first)) {
    return false;
  }

  if (hasMemoryDependency(first, second)) {
    return false;
  }

  if (hasControlDependency(first, second) ||
      hasControlDependency(second, first)) {
    return false;
  }

  return true;
}

// 找到基本块中的调度边界 - 优化版本
static std::vector<size_t>
findSchedulingBoundaries(const std::vector<MachineInstr *> &instrs) {
  std::vector<size_t> boundaries;
  boundaries.reserve(instrs.size() / 10); // 预估边界数量
  boundaries.push_back(0); // 起始边界

  for (size_t i = 0; i < instrs.size(); i++) {
    const InstrInfo& info = getInstrInfo(instrs[i]);
    // 终结指令前后都是边界
    if (info.is_terminator) {
      if (i > 0)
        boundaries.push_back(i);
      if (i + 1 < instrs.size())
        boundaries.push_back(i + 1);
    }
    // 跳转目标标签也可能是边界（这里简化处理）
  }

  boundaries.push_back(instrs.size()); // 结束边界

  // 去重并排序
  std::sort(boundaries.begin(), boundaries.end());
  boundaries.erase(std::unique(boundaries.begin(), boundaries.end()),
                   boundaries.end());

  return boundaries;
}

// 在单个调度区域内进行指令调度 - 优化版本
static void scheduleRegion(std::vector<MachineInstr *> &instrs, size_t start,
                           size_t end) {
  if (end - start <= 1) {
    return; // 区域太小，无需调度
  }

  // 保守的调度策略：
  // 1. 只对小规模区域进行调度
  // 2. 优先将加载指令向前调度，以隐藏内存延迟
  // 3. 确保不破坏数据依赖和内存依赖

  // 简单的调度算法：只尝试将加载指令尽可能前移
  for (size_t i = start + 1; i < end; i++) {
    const InstrInfo& info = getInstrInfo(instrs[i]);
    if (info.is_load) {
      // 尝试将加载指令向前移动
      for (size_t j = i; j > start; j--) {
        // 检查是否可以与前一条指令交换
        if (canSwapInstructions(instrs[j - 1], instrs[j])) {
          std::swap(instrs[j - 1], instrs[j]);
        } else {
          // 一旦遇到依赖关系就停止移动
          break;
        }
      }
    }
  }
}

static void scheduleBlock(MachineBasicBlock *mbb) {
  auto &instructions = mbb->getInstructions();
  if (instructions.size() <= 1 ||
      instructions.size() > MAX_SCHEDULING_BLOCK_SIZE) {
    return;
  }

  // 清理缓存，避免无效指针
  instr_info_cache.clear();

  // 构建指令列表
  std::vector<MachineInstr *> instr_list;
  instr_list.reserve(instructions.size()); // 预分配容量
  for (auto &instr : instructions) {
    instr_list.push_back(instr.get());
  }

  // 预计算所有指令信息
  for (auto* instr : instr_list) {
    getInstrInfo(instr);
  }

  // 找到调度边界
  std::vector<size_t> boundaries = findSchedulingBoundaries(instr_list);

  // 在每个调度区域内进行局部调度
  for (size_t i = 0; i < boundaries.size() - 1; i++) {
    size_t region_start = boundaries[i];
    size_t region_end = boundaries[i + 1];
    scheduleRegion(instr_list, region_start, region_end);
  }

  // 重建指令序列
  std::unordered_map<MachineInstr *, std::unique_ptr<MachineInstr>> instr_map;
  instr_map.reserve(instructions.size()); // 预分配容量
  for (auto &instr : instructions) {
    instr_map[instr.get()] = std::move(instr);
  }

  instructions.clear();
  instructions.reserve(instr_list.size()); // 预分配容量
  for (auto *instr : instr_list) {
    instructions.push_back(std::move(instr_map[instr]));
  }
}

bool PreRA_Scheduler::runOnFunction(Function *F, AnalysisManager &AM) {
  return false;
}

void PreRA_Scheduler::runOnMachineFunction(MachineFunction *mfunc) {
  for (auto &mbb : mfunc->getBlocks()) {
    scheduleBlock(mbb.get());
  }
  
  // 清理全局缓存
  instr_info_cache.clear();
}

} // namespace sysy