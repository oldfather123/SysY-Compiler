#include "Peephole.h"
#include <functional>

namespace sysy {

char PeepholeOptimizer::ID = 0;
bool PeepholeOptimizer::fusedMulAddEnabled = true; // 默认启用浮点乘加融合优化

bool PeepholeOptimizer::runOnFunction(Function *F, AnalysisManager& AM) {
    // This pass works on MachineFunction level, not IR level
    return false;
}

void PeepholeOptimizer::runOnMachineFunction(MachineFunction *mfunc) {
  if (!mfunc)
    return;
  using namespace sysy;

  // areRegsEqual: 检查两个寄存器操作数是否相等（考虑虚拟和物理寄存器）。
  auto areRegsEqual = [](RegOperand *r1, RegOperand *r2) {
    if (!r1 || !r2 || r1->isVirtual() != r2->isVirtual()) {
      return false;
    }
    if (r1->isVirtual()) {
      return r1->getVRegNum() == r2->getVRegNum();
    } else {
      return r1->getPReg() == r2->getPReg();
    }
  };

  // 改进的 isRegUsedLater 函数 - 更完整和准确的实现
  auto isRegUsedLater =
      [&](const std::vector<std::unique_ptr<MachineInstr>> &instrs,
          RegOperand *reg, size_t start_idx) -> bool {
    for (size_t j = start_idx; j < instrs.size(); ++j) {
      auto *instr = instrs[j].get();
      auto opcode = instr->getOpcode();

      // 检查所有操作数
      for (size_t k = 0; k < instr->getOperands().size(); ++k) {
        bool isDefOperand = false;

        // 更完整的定义操作数判断逻辑
        if (k == 0) { // 第一个操作数通常是目标寄存器
          switch (opcode) {
          // 算术和逻辑指令 - 第一个操作数是定义
          case RVOpcodes::MV:
          case RVOpcodes::ADDI:
          case RVOpcodes::SLLI:
          case RVOpcodes::SRLI:
          case RVOpcodes::SRAI:
          case RVOpcodes::SLTI:
          case RVOpcodes::SLTIU:
          case RVOpcodes::XORI:
          case RVOpcodes::ORI:
          case RVOpcodes::ANDI:
          case RVOpcodes::ADD:
          case RVOpcodes::SUB:
          case RVOpcodes::SLL:
          case RVOpcodes::SLT:
          case RVOpcodes::SLTU:
          case RVOpcodes::XOR:
          case RVOpcodes::SRL:
          case RVOpcodes::SRA:
          case RVOpcodes::OR:
          case RVOpcodes::AND:
          case RVOpcodes::MUL:
          case RVOpcodes::DIV:
          case RVOpcodes::REM:
          case RVOpcodes::LW:
          case RVOpcodes::LH:
          case RVOpcodes::LB:
          case RVOpcodes::LHU:
          case RVOpcodes::LBU:

          // 存储指令 - 第一个操作数是使用（要存储的值）
          case RVOpcodes::SW:
          case RVOpcodes::SH:
          case RVOpcodes::SB:
          // 分支指令 - 第一个操作数是使用
          case RVOpcodes::BEQ:
          case RVOpcodes::BNE:
          case RVOpcodes::BLT:
          case RVOpcodes::BGE:
          case RVOpcodes::BLTU:
          case RVOpcodes::BGEU:
          // 跳转指令 - 可能使用寄存器
          case RVOpcodes::JALR:
            isDefOperand = false;
            break;

          default:
            // 对于未知指令，保守地假设第一个操作数可能是使用
            isDefOperand = false;
            break;
          }
        }

        // 如果不是定义操作数，检查是否使用了目标寄存器
        if (!isDefOperand) {
          if (instr->getOperands()[k]->getKind() == MachineOperand::KIND_REG) {
            auto *use_reg =
                static_cast<RegOperand *>(instr->getOperands()[k].get());
            if (areRegsEqual(reg, use_reg))
              return true;
          }
          // 检查内存操作数中的基址寄存器
          if (instr->getOperands()[k]->getKind() == MachineOperand::KIND_MEM) {
            auto *mem =
                static_cast<MemOperand *>(instr->getOperands()[k].get());
            if (areRegsEqual(reg, mem->getBase()))
              return true;
          }
        }
      }
    }
    return false;
  };

  // 检查寄存器是否在指令中被重新定义（用于更精确的分析）
  auto isRegRedefinedAt =
      [](MachineInstr *instr, RegOperand *reg,
         const std::function<bool(RegOperand *, RegOperand *)> &areRegsEqual)
      -> bool {
    if (instr->getOperands().empty())
      return false;

    auto opcode = instr->getOpcode();
    // 只有当第一个操作数是定义操作数时才检查
    switch (opcode) {
    case RVOpcodes::MV:
    case RVOpcodes::ADDI:
    case RVOpcodes::ADD:
    case RVOpcodes::SUB:
    case RVOpcodes::MUL:
    case RVOpcodes::LW:
      // ... 其他定义指令
      if (instr->getOperands()[0]->getKind() == MachineOperand::KIND_REG) {
        auto *def_reg =
            static_cast<RegOperand *>(instr->getOperands()[0].get());
        return areRegsEqual(reg, def_reg);
      }
      break;
    default:
      break;
    }
    return false;
  };

  // 检查是否为存储-加载模式，支持不同大小的访问
  auto isStoreLoadPattern = [](MachineInstr *store_instr,
                               MachineInstr *load_instr) -> bool {
    auto store_op = store_instr->getOpcode();
    auto load_op = load_instr->getOpcode();

    // 检查存储-加载对应关系
    return (store_op == RVOpcodes::SW && load_op == RVOpcodes::LW) || // 32位
           (store_op == RVOpcodes::SH &&
            load_op == RVOpcodes::LH) || // 16位有符号
           (store_op == RVOpcodes::SH &&
            load_op == RVOpcodes::LHU) || // 16位无符号
           (store_op == RVOpcodes::SB &&
            load_op == RVOpcodes::LB) || // 8位有符号
           (store_op == RVOpcodes::SB &&
            load_op == RVOpcodes::LBU) ||                           // 8位无符号
           (store_op == RVOpcodes::SD && load_op == RVOpcodes::LD); // 64位
  };

  // 检查两个内存访问是否访问相同的内存位置
  auto areMemoryAccessesEqual =
      [&areRegsEqual](MachineInstr *store_instr, MemOperand *store_mem,
                      MachineInstr *load_instr, MemOperand *load_mem) -> bool {
    // 基址寄存器必须相同
    if (!areRegsEqual(store_mem->getBase(), load_mem->getBase())) {
      return false;
    }

    // 偏移量必须相同
    if (store_mem->getOffset()->getValue() !=
        load_mem->getOffset()->getValue()) {
      return false;
    }

    // 检查访问大小是否兼容
    auto store_op = store_instr->getOpcode();
    auto load_op = load_instr->getOpcode();

    // 获取访问大小（字节数）
    auto getAccessSize = [](RVOpcodes opcode) -> int {
      switch (opcode) {
      case RVOpcodes::LB:
      case RVOpcodes::LBU:
      case RVOpcodes::SB:
        return 1; // 8位
      case RVOpcodes::LH:
      case RVOpcodes::LHU:
      case RVOpcodes::SH:
        return 2; // 16位
      case RVOpcodes::LW:
      case RVOpcodes::SW:
        return 4; // 32位
      case RVOpcodes::LD:
      case RVOpcodes::SD:
        return 8; // 64位
      default:
        return -1; // 未知
      }
    };

    int store_size = getAccessSize(store_op);
    int load_size = getAccessSize(load_op);

    // 只有访问大小完全匹配时才能进行优化
    // 这避免了部分重叠访问的复杂情况
    return store_size > 0 && store_size == load_size;
  };

  // 简单的内存别名分析：检查两个内存访问之间是否可能有冲突的内存操作
  auto isMemoryAccessSafe =
      [&](const std::vector<std::unique_ptr<MachineInstr>> &instrs,
          size_t store_idx, size_t load_idx, MemOperand *mem) -> bool {
    // 检查存储和加载之间是否有可能影响内存的指令
    for (size_t j = store_idx + 1; j < load_idx; ++j) {
      auto *between_instr = instrs[j].get();
      auto between_op = between_instr->getOpcode();

      // 检查是否有其他内存写入操作
      switch (between_op) {
      case RVOpcodes::SW:
      case RVOpcodes::SH:
      case RVOpcodes::SB:
      case RVOpcodes::SD: {
        // 如果有其他存储操作，需要检查是否可能访问相同的内存
        if (between_instr->getOperands().size() >= 2 &&
            between_instr->getOperands()[1]->getKind() ==
                MachineOperand::KIND_MEM) {

          auto *other_mem =
              static_cast<MemOperand *>(between_instr->getOperands()[1].get());

          // 保守的别名分析：如果使用不同的基址寄存器，假设可能别名
          if (!areRegsEqual(mem->getBase(), other_mem->getBase())) {
            return false; // 可能的别名，不安全
          }

          // 如果基址相同但偏移量不同，检查是否重叠
          int64_t offset1 = mem->getOffset()->getValue();
          int64_t offset2 = other_mem->getOffset()->getValue();

          // 获取访问大小来检查重叠
          auto getAccessSize = [](RVOpcodes opcode) -> int {
            switch (opcode) {
            case RVOpcodes::SB:
              return 1;
            case RVOpcodes::SH:
              return 2;
            case RVOpcodes::SW:
              return 4;
            case RVOpcodes::SD:
              return 8;
            default:
              return 4; // 默认假设4字节
            }
          };

          int size1 = getAccessSize(RVOpcodes::SW); // 从原存储指令推断
          int size2 = getAccessSize(between_op);

          // 检查内存区域是否重叠
          bool overlaps =
              !(offset1 + size1 <= offset2 || offset2 + size2 <= offset1);
          if (overlaps) {
            return false; // 内存重叠，不安全
          }
        }
        break;
      }

      // 函数调用可能有副作用
      case RVOpcodes::JAL:
      case RVOpcodes::JALR:
        return false; // 函数调用可能修改内存，不安全

      // 原子操作或其他可能修改内存的指令
      // 根据具体的RISC-V扩展添加更多指令
      default:
        // 对于未知指令，采用保守策略
        // 可以根据具体需求调整
        break;
      }
    }

    return true; // 没有发现潜在的内存冲突
  };

  // isPowerOfTwo: 检查数值是否为2的幂次，并返回其指数。
  auto isPowerOfTwo = [](int64_t n) -> int {
    if (n <= 0 || (n & (n - 1)) != 0)
      return -1;
    int shift = 0;
    while (n > 1) {
      n >>= 1;
      shift++;
    }
    return shift;
  };

  for (auto &mbb_uptr : mfunc->getBlocks()) {
    auto &mbb = *mbb_uptr;
    auto &instrs = mbb.getInstructions();
    if (instrs.size() < 2)
      continue; // 基本块至少需要两条指令进行窥孔

    // 遍历指令序列进行窥孔优化
    for (size_t i = 0; i + 1 < instrs.size();) {
      auto *mi1 = instrs[i].get();
      auto *mi2 = instrs[i + 1].get();
      bool changed = false;

      // 1. 消除冗余交换移动: mv a, b; mv b, a  -> mv a, b
      if (mi1->getOpcode() == RVOpcodes::MV &&
          mi2->getOpcode() == RVOpcodes::MV) {
        if (mi1->getOperands().size() == 2 && mi2->getOperands().size() == 2) {
          if (mi1->getOperands()[0]->getKind() == MachineOperand::KIND_REG &&
              mi1->getOperands()[1]->getKind() == MachineOperand::KIND_REG &&
              mi2->getOperands()[0]->getKind() == MachineOperand::KIND_REG &&
              mi2->getOperands()[1]->getKind() == MachineOperand::KIND_REG) {
            auto *dst1 = static_cast<RegOperand *>(mi1->getOperands()[0].get());
            auto *src1 = static_cast<RegOperand *>(mi1->getOperands()[1].get());
            auto *dst2 = static_cast<RegOperand *>(mi2->getOperands()[0].get());
            auto *src2 = static_cast<RegOperand *>(mi2->getOperands()[1].get());
            if (areRegsEqual(dst1, src2) && areRegsEqual(src1, dst2)) {
              instrs.erase(instrs.begin() + i + 1); // 移除第二条指令
              changed = true;
            }
          }
        }
      }
      // 2. 冗余加载消除: sw t0, offset(base); lw t1, offset(base) -> 替换或消除
      // lw 添加ld sd支持
      else if (isStoreLoadPattern(mi1, mi2)) {
        if (mi1->getOperands().size() == 2 && mi2->getOperands().size() == 2) {
          if (mi1->getOperands()[0]->getKind() == MachineOperand::KIND_REG &&
              mi1->getOperands()[1]->getKind() == MachineOperand::KIND_MEM &&
              mi2->getOperands()[0]->getKind() == MachineOperand::KIND_REG &&
              mi2->getOperands()[1]->getKind() == MachineOperand::KIND_MEM) {

            auto *store_val =
                static_cast<RegOperand *>(mi1->getOperands()[0].get());
            auto *store_mem =
                static_cast<MemOperand *>(mi1->getOperands()[1].get());
            auto *load_val =
                static_cast<RegOperand *>(mi2->getOperands()[0].get());
            auto *load_mem =
                static_cast<MemOperand *>(mi2->getOperands()[1].get());

            // 检查内存访问是否匹配（基址、偏移量和访问大小）
            if (areMemoryAccessesEqual(mi1, store_mem, mi2, load_mem)) {
              // 进行简单的内存别名分析
              if (isMemoryAccessSafe(instrs, i, i + 1, store_mem)) {
                if (areRegsEqual(store_val, load_val)) {
                  // sw r1, mem; lw r1, mem -> 消除冗余的lw
                  instrs.erase(instrs.begin() + i + 1);
                  changed = true;
                } else {
                  // sw r1, mem; lw r2, mem -> 替换lw为mv r2, r1
                  auto newInstr = std::make_unique<MachineInstr>(RVOpcodes::MV);
                  newInstr->addOperand(std::make_unique<RegOperand>(*load_val));
                  newInstr->addOperand(
                      std::make_unique<RegOperand>(*store_val));
                  instrs[i + 1] = std::move(newInstr);
                  changed = true;
                }
              }
            }
          }
        }
      }
      // 3. 强度削减: mul y, x, 2^n -> slli y, x, n
      else if (mi1->getOpcode() == RVOpcodes::MUL &&
               mi1->getOperands().size() == 3) {
        auto *dst_op = mi1->getOperands()[0].get();
        auto *src1_op = mi1->getOperands()[1].get();
        auto *src2_op = mi1->getOperands()[2].get();

        if (dst_op->getKind() == MachineOperand::KIND_REG) {
          auto *dst_reg = static_cast<RegOperand *>(dst_op);
          RegOperand *src_reg = nullptr;
          int shift = -1;

          if (src1_op->getKind() == MachineOperand::KIND_REG &&
              src2_op->getKind() == MachineOperand::KIND_IMM) {
            shift =
                isPowerOfTwo(static_cast<ImmOperand *>(src2_op)->getValue());
            if (shift >= 0)
              src_reg = static_cast<RegOperand *>(src1_op);
          } else if (src1_op->getKind() == MachineOperand::KIND_IMM &&
                     src2_op->getKind() == MachineOperand::KIND_REG) {
            shift =
                isPowerOfTwo(static_cast<ImmOperand *>(src1_op)->getValue());
            if (shift >= 0)
              src_reg = static_cast<RegOperand *>(src2_op);
          }

          if (src_reg && shift >= 0 &&
              shift <= 31) { // RISC-V 移位量限制 (0-31)
            auto newInstr = std::make_unique<MachineInstr>(RVOpcodes::SLLI);
            newInstr->addOperand(std::make_unique<RegOperand>(*dst_reg));
            newInstr->addOperand(std::make_unique<RegOperand>(*src_reg));
            newInstr->addOperand(std::make_unique<ImmOperand>(shift));
            instrs[i] = std::move(newInstr);
            changed = true;
          }
        }
      }
      // 4. 地址计算优化: addi dst, base, imm1; lw/sw val, imm2(dst) -> lw/sw
      // val, (imm1+imm2)(base)
      else if (mi1->getOpcode() == RVOpcodes::ADDI &&
               mi1->getOperands().size() == 3) {
        auto opcode2 = mi2->getOpcode();
        if (opcode2 == RVOpcodes::LW || opcode2 == RVOpcodes::SW) {
          if (mi2->getOperands().size() == 2 &&
              mi2->getOperands()[1]->getKind() == MachineOperand::KIND_MEM &&
              mi1->getOperands()[0]->getKind() == MachineOperand::KIND_REG &&
              mi1->getOperands()[1]->getKind() == MachineOperand::KIND_REG &&
              mi1->getOperands()[2]->getKind() == MachineOperand::KIND_IMM) {

            auto *addi_dst =
                static_cast<RegOperand *>(mi1->getOperands()[0].get());
            auto *addi_base =
                static_cast<RegOperand *>(mi1->getOperands()[1].get());
            auto *addi_imm =
                static_cast<ImmOperand *>(mi1->getOperands()[2].get());

            auto *mem_op =
                static_cast<MemOperand *>(mi2->getOperands()[1].get());
            auto *mem_base = mem_op->getBase();
            auto *mem_imm = mem_op->getOffset();

            // 检查 ADDI 的目标寄存器是否是内存操作的基址
            if (areRegsEqual(addi_dst, mem_base)) {
              // 改进的使用检查：考虑寄存器可能在后续被重新定义的情况
              bool canOptimize = true;

              // 检查从 i+2 开始的指令
              for (size_t j = i + 2; j < instrs.size(); ++j) {
                auto *later_instr = instrs[j].get();

                // 如果寄存器被重新定义，那么它后面的使用就不相关了
                if (isRegRedefinedAt(later_instr, addi_dst, areRegsEqual)) {
                  break; // 寄存器被重新定义，可以安全优化
                }

                // 如果寄存器被使用，则不能优化
                if (isRegUsedLater(instrs, addi_dst, j)) {
                  canOptimize = false;
                  break;
                }
              }

              if (canOptimize) {
                int64_t new_offset = addi_imm->getValue() + mem_imm->getValue();
                // 检查新偏移量是否符合 RISC-V 12位有符号立即数范围
                if (new_offset >= -2048 && new_offset <= 2047) {
                  auto new_mem_op = std::make_unique<MemOperand>(
                      std::make_unique<RegOperand>(*addi_base),
                      std::make_unique<ImmOperand>(new_offset));
                  mi2->getOperands()[1] = std::move(new_mem_op);
                  instrs.erase(instrs.begin() + i);
                  changed = true;
                }
              }
            }
          }
        }
      }
      // 5. 冗余移动指令消除: mv x, y; op z, x, ... -> op z, y, ... (如果 x
      // 之后不再使用)
      else if (mi1->getOpcode() == RVOpcodes::MV &&
               mi1->getOperands().size() == 2) {
        if (mi1->getOperands()[0]->getKind() == MachineOperand::KIND_REG &&
            mi1->getOperands()[1]->getKind() == MachineOperand::KIND_REG) {

          auto *mv_dst = static_cast<RegOperand *>(mi1->getOperands()[0].get());
          auto *mv_src = static_cast<RegOperand *>(mi1->getOperands()[1].get());

          // 检查第二条指令是否使用了 mv 的目标寄存器
          std::vector<size_t> use_positions;
          for (size_t k = 1; k < mi2->getOperands().size(); ++k) {
            if (mi2->getOperands()[k]->getKind() == MachineOperand::KIND_REG) {
              auto *use_reg =
                  static_cast<RegOperand *>(mi2->getOperands()[k].get());
              if (areRegsEqual(mv_dst, use_reg)) {
                use_positions.push_back(k);
              }
            }
            // 也检查内存操作数中的基址寄存器
            else if (mi2->getOperands()[k]->getKind() ==
                     MachineOperand::KIND_MEM) {
              auto *mem =
                  static_cast<MemOperand *>(mi2->getOperands()[k].get());
              if (areRegsEqual(mv_dst, mem->getBase())) {
                // 对于内存操作数，我们需要创建新的MemOperand
                auto new_mem = std::make_unique<MemOperand>(
                    std::make_unique<RegOperand>(*mv_src),
                    std::make_unique<ImmOperand>(mem->getOffset()->getValue()));
                mi2->getOperands()[k] = std::move(new_mem);
                use_positions.push_back(k); // 标记已处理
              }
            }
          }

          if (!use_positions.empty()) {
            // 改进的后续使用检查
            bool canOptimize = true;
            for (size_t j = i + 2; j < instrs.size(); ++j) {
              auto *later_instr = instrs[j].get();

              // 如果寄存器被重新定义，后续使用就不相关了
              if (isRegRedefinedAt(later_instr, mv_dst, areRegsEqual)) {
                break;
              }

              // 检查是否还有其他使用
              if (isRegUsedLater(instrs, mv_dst, j)) {
                canOptimize = false;
                break;
              }
            }

            if (canOptimize) {
              // 替换所有寄存器使用（内存操作数已在上面处理）
              for (size_t pos : use_positions) {
                if (mi2->getOperands()[pos]->getKind() ==
                    MachineOperand::KIND_REG) {
                  mi2->getOperands()[pos] =
                      std::make_unique<RegOperand>(*mv_src);
                }
              }
              instrs.erase(instrs.begin() + i);
              changed = true;
            }
          }
        }
      }
      // 6. 连续加法指令合并: addi t1, t0, imm1; addi t2, t1, imm2 -> addi t2,
      // t0, (imm1+imm2)
      else if (mi1->getOpcode() == RVOpcodes::ADDI &&
               mi2->getOpcode() == RVOpcodes::ADDI) {
        if (mi1->getOperands().size() == 3 && mi2->getOperands().size() == 3) {
          if (mi1->getOperands()[0]->getKind() == MachineOperand::KIND_REG &&
              mi1->getOperands()[1]->getKind() == MachineOperand::KIND_REG &&
              mi1->getOperands()[2]->getKind() == MachineOperand::KIND_IMM &&
              mi2->getOperands()[0]->getKind() == MachineOperand::KIND_REG &&
              mi2->getOperands()[1]->getKind() == MachineOperand::KIND_REG &&
              mi2->getOperands()[2]->getKind() == MachineOperand::KIND_IMM) {

            auto *addi1_dst =
                static_cast<RegOperand *>(mi1->getOperands()[0].get());
            auto *addi1_src =
                static_cast<RegOperand *>(mi1->getOperands()[1].get());
            auto *addi1_imm =
                static_cast<ImmOperand *>(mi1->getOperands()[2].get());

            auto *addi2_dst =
                static_cast<RegOperand *>(mi2->getOperands()[0].get());
            auto *addi2_src =
                static_cast<RegOperand *>(mi2->getOperands()[1].get());
            auto *addi2_imm =
                static_cast<ImmOperand *>(mi2->getOperands()[2].get());

            // 检查第一个ADDI的目标是否是第二个ADDI的源
            if (areRegsEqual(addi1_dst, addi2_src)) {
              // 改进的中间寄存器使用检查
              bool canOptimize = true;
              for (size_t j = i + 2; j < instrs.size(); ++j) {
                auto *later_instr = instrs[j].get();

                // 如果中间寄存器被重新定义，后续使用不相关
                if (isRegRedefinedAt(later_instr, addi1_dst, areRegsEqual)) {
                  break;
                }

                // 检查是否有其他使用
                if (isRegUsedLater(instrs, addi1_dst, j)) {
                  canOptimize = false;
                  break;
                }
              }

              if (canOptimize) {
                int64_t new_imm = addi1_imm->getValue() + addi2_imm->getValue();
                // 检查新立即数范围
                if (new_imm >= -2048 && new_imm <= 2047) {
                  auto newInstr =
                      std::make_unique<MachineInstr>(RVOpcodes::ADDI);
                  newInstr->addOperand(
                      std::make_unique<RegOperand>(*addi2_dst));
                  newInstr->addOperand(
                      std::make_unique<RegOperand>(*addi1_src));
                  newInstr->addOperand(std::make_unique<ImmOperand>(new_imm));
                  instrs[i + 1] = std::move(newInstr);
                  instrs.erase(instrs.begin() + i);
                  changed = true;
                }
              }
            }
          }
        }
      }

      // 7. ADD with zero optimization: add r1, r2, zero -> mv r1, r2
      else if (mi1->getOpcode() == RVOpcodes::ADD &&
               mi1->getOperands().size() == 3) {
        if (mi1->getOperands()[0]->getKind() == MachineOperand::KIND_REG &&
            mi1->getOperands()[1]->getKind() == MachineOperand::KIND_REG &&
            mi1->getOperands()[2]->getKind() == MachineOperand::KIND_REG) {

          auto *add_dst =
              static_cast<RegOperand *>(mi1->getOperands()[0].get());
          auto *add_src1 =
              static_cast<RegOperand *>(mi1->getOperands()[1].get());
          auto *add_src2 =
              static_cast<RegOperand *>(mi1->getOperands()[2].get());

          // 检查第二个源操作数是否为ZERO寄存器
          if (!add_src2->isVirtual() &&
              add_src2->getPReg() == PhysicalReg::ZERO) {
            // 创建新的 MV 指令
            auto newInstr = std::make_unique<MachineInstr>(RVOpcodes::MV);
            newInstr->addOperand(std::make_unique<RegOperand>(*add_dst));
            newInstr->addOperand(std::make_unique<RegOperand>(*add_src1));
            instrs[i] = std::move(newInstr);
            changed = true;
          }
        }
      }
      // 8. 浮点乘加融合优化
      // 8.1 fmul.s t1, t2, t3; fadd.s t4, t1, t5 -> fmadd.s t4, t2, t3, t5
      else if (isFusedMulAddEnabled() &&
               mi1->getOpcode() == RVOpcodes::FMUL_S &&
               mi2->getOpcode() == RVOpcodes::FADD_S) {
        if (mi1->getOperands().size() == 3 && mi2->getOperands().size() == 3) {
          auto *fmul_dst = static_cast<RegOperand *>(mi1->getOperands()[0].get());
          auto *fmul_src1 = static_cast<RegOperand *>(mi1->getOperands()[1].get());
          auto *fmul_src2 = static_cast<RegOperand *>(mi1->getOperands()[2].get());

          auto *fadd_dst = static_cast<RegOperand *>(mi2->getOperands()[0].get());
          auto *fadd_src1 = static_cast<RegOperand *>(mi2->getOperands()[1].get());
          auto *fadd_src2 = static_cast<RegOperand *>(mi2->getOperands()[2].get());

          // 检查fmul的目标是否是fadd的第一个源操作数
          if (areRegsEqual(fmul_dst, fadd_src1)) {
            // 检查中间寄存器是否在后续还会被使用
            bool canOptimize = true;
            for (size_t j = i + 2; j < instrs.size(); ++j) {
              auto *later_instr = instrs[j].get();
              
              // 如果中间寄存器被重新定义，则可以优化
              if (isRegRedefinedAt(later_instr, fmul_dst, areRegsEqual)) {
                break;
              }
              
              // 如果中间寄存器被使用，则不能优化
              if (isRegUsedLater(instrs, fmul_dst, j)) {
                canOptimize = false;
                break;
              }
            }

            if (canOptimize) {
              // 创建新的FMADD_S指令: fmadd.s t4, t2, t3, t5
              auto newInstr = std::make_unique<MachineInstr>(RVOpcodes::FMADD_S);
              newInstr->addOperand(std::make_unique<RegOperand>(*fadd_dst));
              newInstr->addOperand(std::make_unique<RegOperand>(*fmul_src1));
              newInstr->addOperand(std::make_unique<RegOperand>(*fmul_src2));
              newInstr->addOperand(std::make_unique<RegOperand>(*fadd_src2));
              instrs[i + 1] = std::move(newInstr);
              instrs.erase(instrs.begin() + i);
              changed = true;
            }
          }
        }
      }
      // 8.2 fmul.s t1, t2, t3; fadd.s t4, t5, t1 -> fmadd.s t4, t2, t3, t5
      else if (isFusedMulAddEnabled() &&
               mi1->getOpcode() == RVOpcodes::FMUL_S &&
               mi2->getOpcode() == RVOpcodes::FADD_S) {
        if (mi1->getOperands().size() == 3 && mi2->getOperands().size() == 3) {
          auto *fmul_dst = static_cast<RegOperand *>(mi1->getOperands()[0].get());
          auto *fmul_src1 = static_cast<RegOperand *>(mi1->getOperands()[1].get());
          auto *fmul_src2 = static_cast<RegOperand *>(mi1->getOperands()[2].get());

          auto *fadd_dst = static_cast<RegOperand *>(mi2->getOperands()[0].get());
          auto *fadd_src1 = static_cast<RegOperand *>(mi2->getOperands()[1].get());
          auto *fadd_src2 = static_cast<RegOperand *>(mi2->getOperands()[2].get());

          // 检查fmul的目标是否是fadd的第二个源操作数
          if (areRegsEqual(fmul_dst, fadd_src2)) {
            // 检查中间寄存器是否在后续还会被使用
            bool canOptimize = true;
            for (size_t j = i + 2; j < instrs.size(); ++j) {
              auto *later_instr = instrs[j].get();
              
              // 如果中间寄存器被重新定义，则可以优化
              if (isRegRedefinedAt(later_instr, fmul_dst, areRegsEqual)) {
                break;
              }
              
              // 如果中间寄存器被使用，则不能优化
              if (isRegUsedLater(instrs, fmul_dst, j)) {
                canOptimize = false;
                break;
              }
            }

            if (canOptimize) {
              // 创建新的FMADD_S指令: fmadd.s t4, t2, t3, t5
              auto newInstr = std::make_unique<MachineInstr>(RVOpcodes::FMADD_S);
              newInstr->addOperand(std::make_unique<RegOperand>(*fadd_dst));
              newInstr->addOperand(std::make_unique<RegOperand>(*fmul_src1));
              newInstr->addOperand(std::make_unique<RegOperand>(*fmul_src2));
              newInstr->addOperand(std::make_unique<RegOperand>(*fadd_src1));
              instrs[i + 1] = std::move(newInstr);
              instrs.erase(instrs.begin() + i);
              changed = true;
            }
          }
        }
      }

      // 根据是否发生变化调整遍历索引
      if (!changed) {
        ++i; // 没有优化，继续检查下一对指令
      } else {
        // 发生变化，适当回退以捕获新的优化机会。
        // 这是一种安全的回退策略，可以触发连锁优化，且不会导致无限循环。
        if (i > 0) {
          --i;
        }
      }
    }
  }
}

} // namespace sysy