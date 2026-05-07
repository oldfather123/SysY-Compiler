#include "../include/ssa.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <sstream>

extern std::map<std::string, int> function_name_to_maxreg;

LLVMIR SSAOptimizer::optimize(const LLVMIR &ssa_ir) {
  LLVMIR optimized_ir = ssa_ir;

  std::cout << "Starting SSA optimizations..." << std::endl;

  // 执行多轮优化，直到没有更多改进
  bool changed = true;
  int iteration = 0;

  while (changed && iteration < 10) { // 限制迭代次数防止无限循环
    changed = false;
    iteration++;

    std::cout << "Optimization iteration " << iteration << std::endl;

    size_t before_count = countInstructions(optimized_ir);

    // 1. 常量传播
    std::cout << "  Running constant propagation..." << std::endl;
    constantPropagation(optimized_ir);

    // 2. 常量折叠
    std::cout << "  Running constant folding..." << std::endl;
    constantFolding(optimized_ir);

    // 3. 死代码消除
    std::cout << "  Running dead code elimination..." << std::endl;
    eliminateDeadCode(optimized_ir);

    // 4. 复制传播
    std::cout << "  Running copy propagation..." << std::endl;
    copyPropagation(optimized_ir);

    // 5. 代数化简
    std::cout << "  Running algebraic simplification..." << std::endl;
    algebraicSimplification(optimized_ir);

    // 6. φ函数简化
    std::cout << "  Running phi function simplification..." << std::endl;
    simplifyPhiFunctions(optimized_ir);

    // 7. 不可达代码消除
    std::cout << "  Running unreachable code elimination..." << std::endl;
    eliminateUnreachableCode(optimized_ir);

    // 8. 除法优化
    std::cout << "  Running division optimization..." << std::endl;
    optimizeDivision(optimized_ir);

    // 9. 公共子表达式消除
    std::cout << "  Running common subexpression elimination..." << std::endl;
    commonSubexpressionElimination(optimized_ir);

    // 10. 强度削减
    std::cout << "  Running strength reduction..." << std::endl;
    strengthReduction(optimized_ir);

    // 11. 循环不变量外提
    std::cout << "  Running loop invariant code motion..." << std::endl;
    loopInvariantCodeMotion(optimized_ir);

    // 12. 条件常量传播
    std::cout << "  Running conditional constant propagation..." << std::endl;
    conditionalConstantPropagation(optimized_ir);

    // 13. 全局值编号
    std::cout << "  Running global value numbering..." << std::endl;
    globalValueNumbering(optimized_ir);

    // 14. 循环展开（仅在前几轮迭代中运行）
    if (iteration <= 2) {
      std::cout << "  Running loop unrolling..." << std::endl;
      loopUnrolling(optimized_ir);
    }

    // 15. 函数内联（仅在前几轮迭代中运行）
    if (iteration <= 2) {
      std::cout << "  Running function inlining..." << std::endl;
      functionInlining(optimized_ir);
    }

    size_t after_count = countInstructions(optimized_ir);

    if (after_count < before_count) {
      changed = true;
      std::cout << "  Eliminated " << (before_count - after_count)
                << " instructions" << std::endl;
    }

    // 清理状态为下一轮迭代准备
    constants_map.clear();
    useful_instructions.clear();
  }

  std::cout << "SSA optimizations completed after " << iteration
            << " iterations" << std::endl;

  return optimized_ir;
}

void SSAOptimizer::eliminateDeadCode(LLVMIR &ir) {
  // 对每个函数进行死代码消除
  for (auto &func_pair : ir.function_block_map) {
    std::map<int, LLVMBlock> &blocks = func_pair.second;

    // 第一步：建立完整的指令映射和依赖关系
    std::unordered_map<int, Instruction> reg_to_inst;
    std::unordered_map<Instruction, std::vector<int>> inst_to_defined_regs;
    std::unordered_map<Instruction, std::vector<int>> inst_to_used_regs;

    // 建立寄存器到指令的映射
    for (const auto &block_pair : blocks) {
      LLVMBlock block = block_pair.second;
      for (const auto &inst : block->Instruction_list) {
        if (!inst)
          continue;

        // 记录指令定义的寄存器
        int result_reg = getInstructionResultRegister(inst);
        if (result_reg != -1) {
          reg_to_inst[result_reg] = inst;
          inst_to_defined_regs[inst].push_back(result_reg);
        }

        // 记录指令使用的寄存器
        std::vector<Operand> operands = getInstructionOperands(inst);
        for (const auto &operand : operands) {
          if (isRegisterOperand(operand)) {
            int reg_num = getRegisterFromOperand(operand);
            if (reg_num != -1) {
              inst_to_used_regs[inst].push_back(reg_num);
            }
          }
        }
      }
    }

    // 第二步：标记所有有用的指令
    useful_instructions.clear();
    std::queue<Instruction> work_list;
    std::unordered_set<Instruction> in_worklist;

    // 标记所有关键指令为有用
    for (const auto &block_pair : blocks) {
      LLVMBlock block = block_pair.second;
      for (const auto &inst : block->Instruction_list) {
        if (!inst)
          continue;

        if (isCriticalInstruction(inst)) {
          if (useful_instructions.find(inst) == useful_instructions.end()) {
            markInstructionAsUseful(inst);
            if (in_worklist.find(inst) == in_worklist.end()) {
              work_list.push(inst);
              in_worklist.insert(inst);
            }
          }
        }
      }
    }

    // 第三步：传播有用性（使用更强健的算法）
    while (!work_list.empty()) {
      Instruction inst = work_list.front();
      work_list.pop();
      in_worklist.erase(inst);

      // 标记该指令使用的所有寄存器的定义指令为有用
      auto used_regs_it = inst_to_used_regs.find(inst);
      if (used_regs_it != inst_to_used_regs.end()) {
        for (int reg_num : used_regs_it->second) {
          auto def_inst_it = reg_to_inst.find(reg_num);
          if (def_inst_it != reg_to_inst.end()) {
            Instruction def_inst = def_inst_it->second;
            if (def_inst && useful_instructions.find(def_inst) ==
                                useful_instructions.end()) {
              markInstructionAsUseful(def_inst);
              if (in_worklist.find(def_inst) == in_worklist.end()) {
                work_list.push(def_inst);
                in_worklist.insert(def_inst);
              }
            }
          }
        }
      }

      // 对于φ函数和控制流相关指令，标记控制依赖
      int opcode = inst->GetOpcode();
      if (opcode == PHI || opcode == BR_COND || opcode == BR_UNCOND) {
        markControlDependencies(inst, blocks, work_list);
        // 将新加入工作列表的指令标记
        while (!work_list.empty()) {
          Instruction control_inst = work_list.front();
          work_list.pop();
          if (in_worklist.find(control_inst) == in_worklist.end()) {
            work_list.push(control_inst);
            in_worklist.insert(control_inst);
          }
        }
      }
    }

    // 第四步：保守地保留所有地址计算指令
    // 这是为了确保我们不会删除任何可能被间接使用的指令
    for (const auto &block_pair : blocks) {
      LLVMBlock block = block_pair.second;
      for (const auto &inst : block->Instruction_list) {
        if (!inst)
          continue;

        int opcode = inst->GetOpcode();
        if (opcode == GETELEMENTPTR || opcode == ALLOCA) {
          if (useful_instructions.find(inst) == useful_instructions.end()) {
            // 检查是否有任何指令使用这个结果
            int result_reg = getInstructionResultRegister(inst);
            if (result_reg != -1) {
              bool is_used = false;
              for (const auto &check_block_pair : blocks) {
                LLVMBlock check_block = check_block_pair.second;
                for (const auto &check_inst : check_block->Instruction_list) {
                  if (!check_inst || check_inst == inst)
                    continue;

                  std::vector<Operand> operands =
                      getInstructionOperands(check_inst);
                  for (const auto &operand : operands) {
                    if (isRegisterOperand(operand) &&
                        getRegisterFromOperand(operand) == result_reg) {
                      is_used = true;
                      break;
                    }
                  }
                  if (is_used)
                    break;
                }
                if (is_used)
                  break;
              }

              if (is_used) {
                markInstructionAsUseful(inst);
                // 递归标记其依赖
                std::queue<Instruction> conservative_worklist;
                conservative_worklist.push(inst);
                while (!conservative_worklist.empty()) {
                  Instruction cons_inst = conservative_worklist.front();
                  conservative_worklist.pop();

                  auto used_regs_it = inst_to_used_regs.find(cons_inst);
                  if (used_regs_it != inst_to_used_regs.end()) {
                    for (int reg_num : used_regs_it->second) {
                      auto def_inst_it = reg_to_inst.find(reg_num);
                      if (def_inst_it != reg_to_inst.end()) {
                        Instruction def_inst = def_inst_it->second;
                        if (def_inst && useful_instructions.find(def_inst) ==
                                            useful_instructions.end()) {
                          markInstructionAsUseful(def_inst);
                          conservative_worklist.push(def_inst);
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    // 第五步：删除所有无用指令
    size_t removed_count = 0;
    for (const auto &block_pair : blocks) {
      LLVMBlock block = block_pair.second;

      auto it = block->Instruction_list.begin();
      while (it != block->Instruction_list.end()) {
        if (useful_instructions.find(*it) == useful_instructions.end()) {
          // 这条指令是无用的，删除它
          it = block->Instruction_list.erase(it);
          removed_count++;
        } else {
          ++it;
        }
      }
    }

    if (removed_count > 0) {
      std::cout << "    Removed " << removed_count << " dead instructions"
                << std::endl;
    }
  }
}

void SSAOptimizer::constantPropagation(LLVMIR &ir) {
  // 对每个函数进行常量传播
  for (auto &func_pair : ir.function_block_map) {
    std::map<int, LLVMBlock> &blocks = func_pair.second;

    // 工作列表算法进行常量传播
    std::queue<std::pair<int, size_t>>
        work_list; // (block_id, instruction_index)
    std::unordered_set<std::string> visited;

    // 初始化：将所有指令加入工作列表
    for (const auto &block_pair : blocks) {
      int block_id = block_pair.first;
      LLVMBlock block = block_pair.second;

      for (size_t i = 0; i < block->Instruction_list.size(); ++i) {
        work_list.push({block_id, i});
      }
    }

    // 处理工作列表
    while (!work_list.empty()) {
      auto [block_id, inst_idx] = work_list.front();
      work_list.pop();

      auto block_it = blocks.find(block_id);
      if (block_it == blocks.end() ||
          inst_idx >= block_it->second->Instruction_list.size()) {
        continue;
      }

      LLVMBlock block = block_it->second;
      Instruction inst = block->Instruction_list[inst_idx];

      if (!inst)
        continue;

      int opcode = inst->GetOpcode();

      // 处理不同类型的指令
      switch (opcode) {
      case LOAD: {
        // load指令：检查是否从常量地址加载
        // 这里需要根据具体的load指令格式来实现
        // 暂时跳过复杂的load常量传播
        break;
      }

      case ADD:
      case SUB:
      case MUL_OP:
      case DIV_OP:
      case FADD:
      case FSUB:
      case FMUL:
      case FDIV: {
        // 算术指令：检查操作数是否为常量
        if (canPropagateConstants(inst)) {
          propagateConstantsInInstruction(inst);

          // 更新常量映射
          updateConstantMapping(inst);

          // 将使用该结果的指令加入工作列表
          int result_reg = getInstructionResultRegister(inst);
          if (result_reg != -1) {
            addUsersToWorkList(result_reg, blocks, work_list);
          }
        }
        break;
      }

      case ICMP:
      case FCMP: {
        // 比较指令
        if (canPropagateConstants(inst)) {
          propagateConstantsInInstruction(inst);
          updateConstantMapping(inst);

          int result_reg = getInstructionResultRegister(inst);
          if (result_reg != -1) {
            addUsersToWorkList(result_reg, blocks, work_list);
          }
        }
        break;
      }

      case PHI: {
        // φ函数：检查所有操作数是否为相同常量
        if (canPropagateConstants(inst)) {
          propagateConstantsInInstruction(inst);
          updateConstantMapping(inst);

          int result_reg = getInstructionResultRegister(inst);
          if (result_reg != -1) {
            addUsersToWorkList(result_reg, blocks, work_list);
          }
        }
        break;
      }

      default:
        // 其他指令：更新常量映射但不传播
        updateConstantMapping(inst);
        break;
      }
    }
  }
}

void SSAOptimizer::constantFolding(LLVMIR &ir) {
  // 对每个函数进行常量折叠
  for (auto &func_pair : ir.function_block_map) {
    std::map<int, LLVMBlock> &blocks = func_pair.second;

    for (const auto &block_pair : blocks) {
      LLVMBlock block = block_pair.second;

      for (auto &inst : block->Instruction_list) {
        if (!inst)
          continue;

        int opcode = inst->GetOpcode();

        // 检查是否是可以折叠的指令
        switch (opcode) {
        case ADD:
        case SUB:
        case MUL_OP:
        case DIV_OP:
        case MOD_OP: {
          // 整数算术运算
          if (canFoldConstants(inst)) {
            foldConstantsInInstruction(inst);
          }
          break;
        }

        case FADD:
        case FSUB:
        case FMUL:
        case FDIV: {
          // 浮点算术运算
          if (canFoldConstants(inst)) {
            foldConstantsInInstruction(inst);
          }
          break;
        }

        case ICMP: {
          // 整数比较
          if (canFoldConstants(inst)) {
            foldConstantsInInstruction(inst);
          }
          break;
        }

        case FCMP: {
          // 浮点比较
          if (canFoldConstants(inst)) {
            foldConstantsInInstruction(inst);
          }
          break;
        }

        case BITAND:
        case BITOR:
        case BITXOR:
        case SHL: {
          // 位运算
          if (canFoldConstants(inst)) {
            foldConstantsInInstruction(inst);
          }
          break;
        }

        case ZEXT:
        case SITOFP:
        case FPTOSI: {
          // 类型转换
          if (canFoldConstants(inst)) {
            foldConstantsInInstruction(inst);
          }
          break;
        }

        case SELECT: {
          // select指令：select i1 %cond, type %val1, type %val2
          if (canFoldConstants(inst)) {
            foldConstantsInInstruction(inst);
          }
          break;
        }

        default:
          // 不支持折叠的指令类型
          break;
        }
      }
    }
  }
}

void SSAOptimizer::copyPropagation(LLVMIR &ir) {
  // 对每个函数进行复制传播
  for (auto &func_pair : ir.function_block_map) {
    std::map<int, LLVMBlock> &blocks = func_pair.second;

    // 建立复制关系映射 (目标寄存器 -> 源寄存器)
    std::unordered_map<int, int> copy_map;
    std::unordered_map<int, int> value_map; // 寄存器到其最终值的映射

    bool changed = true;
    int iteration = 0;

    while (changed && iteration < 5) {
      changed = false;
      iteration++;

      // 按照支配顺序处理基本块（简化：按ID顺序）
      for (const auto &block_pair : blocks) {
        LLVMBlock block = block_pair.second;

        for (auto &inst : block->Instruction_list) {
          if (!inst)
            continue;

          int opcode = inst->GetOpcode();

          // 识别复制指令
          if (isCopyInstruction(inst)) {
            int dest_reg, src_reg;
            if (extractCopyRegisters(inst, dest_reg, src_reg)) {
              // 传递复制关系：如果src已经是其他值的复制，则传递
              int final_src = src_reg;
              auto src_it = value_map.find(src_reg);
              if (src_it != value_map.end()) {
                final_src = src_it->second;
              }

              // 避免循环复制
              if (final_src != dest_reg) {
                copy_map[dest_reg] = src_reg;
                value_map[dest_reg] = final_src;
              }
            }
          }

          // 处理φ函数的特殊情况
          else if (opcode == PHI) {
            // φ函数可能产生复制机会
            if (isPhiCopyCandidate(inst)) {
              int dest_reg = getInstructionResultRegister(inst);
              int src_reg = getPhiSingleSource(inst);
              if (dest_reg != -1 && src_reg != -1 && dest_reg != src_reg) {
                int final_src = src_reg;
                auto src_it = value_map.find(src_reg);
                if (src_it != value_map.end()) {
                  final_src = src_it->second;
                }

                copy_map[dest_reg] = src_reg;
                value_map[dest_reg] = final_src;
              }
            }
          }

          // 替换指令中使用的寄存器
          if (replaceCopyUsages(inst, value_map)) {
            changed = true;
          }
        }
      }
    }

    // 移除变成无用的复制指令
    removeTrivialCopies(blocks, copy_map);
  }
}

void SSAOptimizer::commonSubexpressionElimination(LLVMIR &ir) {
  // 公共子表达式消除：识别并合并相同的表达式
  std::cout << "  Eliminating common subexpressions..." << std::endl;

  for (auto &func_pair : ir.function_block_map) {
    std::map<int, LLVMBlock> &blocks = func_pair.second;

    // 表达式到定义寄存器的映射
    std::unordered_map<std::string, int> expression_to_reg;
    // 寄存器到表达式的映射
    std::unordered_map<int, std::string> reg_to_expression;

    // 按支配顺序处理基本块（简化：按ID顺序）
    for (const auto &block_pair : blocks) {
      LLVMBlock block = block_pair.second;

      for (auto &inst : block->Instruction_list) {
        if (!inst)
          continue;

        int opcode = inst->GetOpcode();

        // 只对纯函数式指令进行CSE
        if (isPureFunctionalInstruction(inst)) {
          std::string expr = generateExpressionString(inst);

          if (!expr.empty()) {
            auto it = expression_to_reg.find(expr);
            if (it != expression_to_reg.end()) {
              // 找到了相同的表达式，可以复用
              int existing_reg = it->second;
              int current_reg = getInstructionResultRegister(inst);

              if (current_reg != -1 && existing_reg != current_reg) {
                // 用已存在的寄存器替换当前指令
                replaceInstructionWithCopy(inst, existing_reg);
                std::cout << "    CSE: Replaced " << expr
                          << " with existing result %" << existing_reg
                          << std::endl;
              }
            } else {
              // 新表达式，记录它
              int result_reg = getInstructionResultRegister(inst);
              if (result_reg != -1) {
                expression_to_reg[expr] = result_reg;
                reg_to_expression[result_reg] = expr;
              }
            }
          }
        }
      }
    }
  }
}

void SSAOptimizer::algebraicSimplification(LLVMIR &ir) {
  // 代数化简：应用数学恒等式简化表达式
  std::cout << "  Applying algebraic simplifications..." << std::endl;

  for (auto &func_pair : ir.function_block_map) {
    std::map<int, LLVMBlock> &blocks = func_pair.second;

    for (const auto &block_pair : blocks) {
      LLVMBlock block = block_pair.second;

      for (auto &inst : block->Instruction_list) {
        if (!inst)
          continue;

        int opcode = inst->GetOpcode();

        switch (opcode) {
        case ADD: {
          // x + 0 = x, 0 + x = x
          std::vector<Operand> operands = getInstructionOperands(inst);
          if (operands.size() >= 2) {
            ConstantValue left = getConstantFromOperand(operands[0]);
            ConstantValue right = getConstantFromOperand(operands[1]);

            if (left.isConstant() && left.isInt() && left.int_val == 0) {
              // 0 + x = x, 将指令替换为简单的复制
              replaceWithIdentity(inst, operands[1]);
            } else if (right.isConstant() && right.isInt() &&
                       right.int_val == 0) {
              // x + 0 = x
              replaceWithIdentity(inst, operands[0]);
            }
          }
          break;
        }

        case SUB: {
          // x - 0 = x, x - x = 0
          std::vector<Operand> operands = getInstructionOperands(inst);
          if (operands.size() >= 2) {
            ConstantValue right = getConstantFromOperand(operands[1]);

            if (right.isConstant() && right.isInt() && right.int_val == 0) {
              // x - 0 = x
              replaceWithIdentity(inst, operands[0]);
            } else if (operandEquals(operands[0], operands[1])) {
              // x - x = 0
              replaceWithConstant(inst, ConstantValue(0));
            }
          }
          break;
        }

        case MUL_OP: {
          // x * 1 = x, 1 * x = x, x * 0 = 0, 0 * x = 0
          std::vector<Operand> operands = getInstructionOperands(inst);
          if (operands.size() >= 2) {
            ConstantValue left = getConstantFromOperand(operands[0]);
            ConstantValue right = getConstantFromOperand(operands[1]);

            if (left.isConstant() && left.isInt()) {
              if (left.int_val == 0) {
                // 0 * x = 0
                replaceWithConstant(inst, ConstantValue(0));
              } else if (left.int_val == 1) {
                // 1 * x = x
                replaceWithIdentity(inst, operands[1]);
              }
            } else if (right.isConstant() && right.isInt()) {
              if (right.int_val == 0) {
                // x * 0 = 0
                replaceWithConstant(inst, ConstantValue(0));
              } else if (right.int_val == 1) {
                // x * 1 = x
                replaceWithIdentity(inst, operands[0]);
              }
            }
          }
          break;
        }

        case DIV_OP: {
          // x / 1 = x, x / x = 1 (if x != 0)
          std::vector<Operand> operands = getInstructionOperands(inst);
          if (operands.size() >= 2) {
            ConstantValue right = getConstantFromOperand(operands[1]);

            if (right.isConstant() && right.isInt() && right.int_val == 1) {
              // x / 1 = x
              replaceWithIdentity(inst, operands[0]);
            } else if (operandEquals(operands[0], operands[1])) {
              // x / x = 1 (保守处理，不考虑x=0的情况)
              replaceWithConstant(inst, ConstantValue(1));
            }
          }
          break;
        }

        case BITAND: {
          // x & x = x, x & 0 = 0, x & (-1) = x
          std::vector<Operand> operands = getInstructionOperands(inst);
          if (operands.size() >= 2) {
            if (operandEquals(operands[0], operands[1])) {
              // x & x = x
              replaceWithIdentity(inst, operands[0]);
            } else {
              ConstantValue left = getConstantFromOperand(operands[0]);
              ConstantValue right = getConstantFromOperand(operands[1]);

              if (left.isConstant() && left.isInt() && left.int_val == 0) {
                // 0 & x = 0
                replaceWithConstant(inst, ConstantValue(0));
              } else if (right.isConstant() && right.isInt() &&
                         right.int_val == 0) {
                // x & 0 = 0
                replaceWithConstant(inst, ConstantValue(0));
              }
            }
          }
          break;
        }

        case BITOR: {
          // x | x = x, x | 0 = x, x | (-1) = -1
          std::vector<Operand> operands = getInstructionOperands(inst);
          if (operands.size() >= 2) {
            if (operandEquals(operands[0], operands[1])) {
              // x | x = x
              replaceWithIdentity(inst, operands[0]);
            } else {
              ConstantValue left = getConstantFromOperand(operands[0]);
              ConstantValue right = getConstantFromOperand(operands[1]);

              if (left.isConstant() && left.isInt() && left.int_val == 0) {
                // 0 | x = x
                replaceWithIdentity(inst, operands[1]);
              } else if (right.isConstant() && right.isInt() &&
                         right.int_val == 0) {
                // x | 0 = x
                replaceWithIdentity(inst, operands[0]);
              }
            }
          }
          break;
        }

        case BITXOR: {
          // x ^ x = 0, x ^ 0 = x
          std::vector<Operand> operands = getInstructionOperands(inst);
          if (operands.size() >= 2) {
            if (operandEquals(operands[0], operands[1])) {
              // x ^ x = 0
              replaceWithConstant(inst, ConstantValue(0));
            } else {
              ConstantValue left = getConstantFromOperand(operands[0]);
              ConstantValue right = getConstantFromOperand(operands[1]);

              if (left.isConstant() && left.isInt() && left.int_val == 0) {
                // 0 ^ x = x
                replaceWithIdentity(inst, operands[1]);
              } else if (right.isConstant() && right.isInt() &&
                         right.int_val == 0) {
                // x ^ 0 = x
                replaceWithIdentity(inst, operands[0]);
              }
            }
          }
          break;
        }

        default:
          break;
        }
      }
    }
  }
}

void SSAOptimizer::simplifyPhiFunctions(LLVMIR &ir) {
  // φ函数简化：移除冗余的φ函数
  std::cout << "  Simplifying phi functions..." << std::endl;

  for (auto &func_pair : ir.function_block_map) {
    std::map<int, LLVMBlock> &blocks = func_pair.second;

    bool changed = true;
    while (changed) {
      changed = false;

      for (const auto &block_pair : blocks) {
        LLVMBlock block = block_pair.second;

        auto it = block->Instruction_list.begin();
        while (it != block->Instruction_list.end()) {
          if (!*it || (*it)->GetOpcode() != PHI) {
            ++it;
            continue;
          }

          Instruction phi_inst = *it;
          std::vector<Operand> operands = getInstructionOperands(phi_inst);

          if (operands.empty()) {
            // 空的φ函数，删除
            it = block->Instruction_list.erase(it);
            changed = true;
            continue;
          }

          // 检查所有输入是否相同
          bool all_same = true;
          Operand first_operand = operands[0];

          for (size_t i = 1; i < operands.size(); ++i) {
            if (!operandEquals(first_operand, operands[i])) {
              all_same = false;
              break;
            }
          }

          if (all_same) {
            // 所有输入相同，φ函数可以简化为简单赋值
            replaceWithIdentity(phi_inst, first_operand);
            it = block->Instruction_list.erase(it);
            changed = true;
            continue;
          }

          // 检查是否只有一个有效输入（其他都是该φ函数本身）
          Operand effective_operand = nullptr;
          int effective_count = 0;

          int phi_result_reg = getInstructionResultRegister(phi_inst);

          for (const auto &operand : operands) {
            if (isRegisterOperand(operand)) {
              int reg_num = getRegisterFromOperand(operand);
              if (reg_num != phi_result_reg) {
                effective_operand = operand;
                effective_count++;
              }
            } else {
              effective_operand = operand;
              effective_count++;
            }
          }

          if (effective_count == 1 && effective_operand) {
            // 只有一个有效输入，简化为简单赋值
            replaceWithIdentity(phi_inst, effective_operand);
            it = block->Instruction_list.erase(it);
            changed = true;
            continue;
          }

          ++it;
        }
      }
    }
  }
}

void SSAOptimizer::eliminateUnreachableCode(LLVMIR &ir) {
  // 不可达代码消除：移除永远不会被执行的基本块和指令
  std::cout << "  Eliminating unreachable code..." << std::endl;

  for (auto &func_pair : ir.function_block_map) {
    std::map<int, LLVMBlock> &blocks = func_pair.second;

    if (blocks.empty())
      continue;

    // 第一步：标记所有可达的基本块
    std::unordered_set<int> reachable_blocks;
    std::queue<int> work_list;

    // 入口块总是可达的（假设第一个块是入口块）
    int entry_block_id = blocks.begin()->first;
    reachable_blocks.insert(entry_block_id);
    work_list.push(entry_block_id);

    // BFS遍历所有可达块
    while (!work_list.empty()) {
      int current_block_id = work_list.front();
      work_list.pop();

      auto block_it = blocks.find(current_block_id);
      if (block_it == blocks.end())
        continue;

      LLVMBlock current_block = block_it->second;

      // 查找分支指令并标记目标块
      for (const auto &inst : current_block->Instruction_list) {
        if (!inst)
          continue;

        int opcode = inst->GetOpcode();
        if (opcode == BR_COND || opcode == BR_UNCOND) {
          // 提取分支目标（这里需要根据具体的分支指令格式实现）
          std::vector<int> target_blocks = getBranchTargets(inst);

          for (int target_id : target_blocks) {
            if (reachable_blocks.find(target_id) == reachable_blocks.end()) {
              reachable_blocks.insert(target_id);
              work_list.push(target_id);
            }
          }
        }
      }
    }

    // 第二步：删除不可达的基本块
    auto block_it = blocks.begin();
    while (block_it != blocks.end()) {
      int block_id = block_it->first;

      if (reachable_blocks.find(block_id) == reachable_blocks.end()) {
        // 不可达块，删除
        std::cout << "    Removing unreachable block " << block_id << std::endl;
        block_it = blocks.erase(block_it);
      } else {
        ++block_it;
      }
    }

    // 第三步：在可达块内删除无条件分支后的不可达指令
    for (const auto &block_pair : blocks) {
      LLVMBlock block = block_pair.second;

      auto inst_it = block->Instruction_list.begin();
      bool found_terminator = false;

      while (inst_it != block->Instruction_list.end()) {
        if (!*inst_it) {
          ++inst_it;
          continue;
        }

        int opcode = (*inst_it)->GetOpcode();

        if (found_terminator) {
          // 终结指令之后的指令都是不可达的
          inst_it = block->Instruction_list.erase(inst_it);
        } else {
          if (opcode == RET || opcode == BR_UNCOND || opcode == BR_COND) {
            found_terminator = true;
          }
          ++inst_it;
        }
      }
    }
  }
}

// ============================================================================
// 辅助函数实现
// ============================================================================

bool SSAOptimizer::isCriticalInstruction(const Instruction &inst) {
  if (!inst)
    return false;

  int opcode = inst->GetOpcode();

  // 关键指令：有副作用的指令，控制流指令，函数调用等
  switch (opcode) {
  case CALL:      // 函数调用
  case RET:       // 返回
  case STORE:     // 存储（可能有副作用）
  case BR_COND:   // 条件分支
  case BR_UNCOND: // 无条件分支
  case ALLOCA:    // 内存分配（绝对不能删除）
    return true;

  // 某些内在函数可能有副作用
  case LOAD:
    // 对于load，我们保守地认为它是关键的
    // 实际应该检查是否从volatile内存加载
    return true;

  default:
    return false;
  }
}

size_t SSAOptimizer::countInstructions(const LLVMIR &ir) {
  size_t count = 0;
  for (const auto &func_pair : ir.function_block_map) {
    for (const auto &block_pair : func_pair.second) {
      count += block_pair.second->Instruction_list.size();
    }
  }
  return count;
}

std::vector<Operand>
SSAOptimizer::getInstructionOperands(const Instruction &inst) {
  std::vector<Operand> operands;

  if (!inst)
    return operands;

  // 根据指令类型提取操作数
  int opcode = inst->GetOpcode();

  switch (opcode) {
  case ADD:
  case SUB:
  case MUL_OP:
  case DIV_OP:
  case MOD_OP:
  case FADD:
  case FSUB:
  case FMUL:
  case FDIV:
  case ICMP:
  case FCMP:
  case BITAND:
  case BITOR:
  case BITXOR:
  case SHL: {
    // 二元运算指令通常有两个或三个操作数
    ArithmeticInstruction *arith_inst =
        dynamic_cast<ArithmeticInstruction *>(inst);
    if (arith_inst) {
      Operand op1 = arith_inst->GetOp1();
      Operand op2 = arith_inst->GetOp2();
      Operand op3 = arith_inst->GetOp3();
      if (op1)
        operands.push_back(op1);
      if (op2)
        operands.push_back(op2);
      if (op3)
        operands.push_back(op3);
    }
    break;
  }

  case LOAD: {
    // load指令有一个地址操作数
    LoadInstruction *load_inst = dynamic_cast<LoadInstruction *>(inst);
    if (load_inst) {
      Operand ptr = load_inst->GetPointer();
      if (ptr)
        operands.push_back(ptr);
    }
    break;
  }

  case STORE: {
    // store指令有值和地址两个操作数
    StoreInstruction *store_inst = dynamic_cast<StoreInstruction *>(inst);
    if (store_inst) {
      Operand value = store_inst->GetValue();
      Operand ptr = store_inst->GetPointer();
      if (value)
        operands.push_back(value);
      if (ptr)
        operands.push_back(ptr);
    }
    break;
  }

  case PHI: {
    // φ函数有多个值-标签对操作数
    PhiInstruction *phi_inst = dynamic_cast<PhiInstruction *>(inst);
    if (phi_inst) {
      auto &phi_list = phi_inst->phi_list;
      for (const auto &phi_pair : phi_list) {
        // phi_list: first = label, second = value
        if (phi_pair.second)
          operands.push_back(phi_pair.second);
      }
    }
    break;
  }

  case RET: {
    // return指令可能有一个返回值操作数
    RetInstruction *ret_inst = dynamic_cast<RetInstruction *>(inst);
    if (ret_inst) {
      Operand ret_val = ret_inst->GetRetVal();
      if (ret_val)
        operands.push_back(ret_val);
    }
    break;
  }

  case BR_COND: {
    // 条件分支指令有一个条件操作数
    BrCondInstruction *br_inst = dynamic_cast<BrCondInstruction *>(inst);
    if (br_inst) {
      Operand cond = br_inst->GetCond();
      if (cond)
        operands.push_back(cond);
    }
    break;
  }

  case CALL: {
    // 函数调用指令有多个参数操作数
    CallInstruction *call_inst = dynamic_cast<CallInstruction *>(inst);
    if (call_inst) {
      auto &args = call_inst->GetArgs();
      for (const auto &arg : args) {
        if (arg.second)
          operands.push_back(arg.second);
      }
    }
    break;
  }

  case GETELEMENTPTR: {
    // getelementptr指令有基础指针和索引操作数
    GetElementptrInstruction *gep_inst =
        dynamic_cast<GetElementptrInstruction *>(inst);
    if (gep_inst) {
      Operand ptr = gep_inst->GetPtrVal();
      if (ptr)
        operands.push_back(ptr);

      const auto &indexes = gep_inst->GetIndexes();
      for (const auto &index : indexes) {
        if (index)
          operands.push_back(index);
      }
    }
    break;
  }

  case ZEXT:
  case SITOFP:
  case FPTOSI:
  case FPEXT:
  case BITCAST: {
    // 类型转换指令通常有一个操作数
    if (opcode == SITOFP) {
      SitofpInstruction *sitofp_inst = dynamic_cast<SitofpInstruction *>(inst);
      if (sitofp_inst && sitofp_inst->value) {
        operands.push_back(sitofp_inst->value);
      }
    } else if (opcode == FPTOSI) {
      FptosiInstruction *fptosi_inst = dynamic_cast<FptosiInstruction *>(inst);
      if (fptosi_inst && fptosi_inst->value) {
        operands.push_back(fptosi_inst->value);
      }
    } else if (opcode == FPEXT) {
      FpextInstruction *fpext_inst = dynamic_cast<FpextInstruction *>(inst);
      if (fpext_inst && fpext_inst->value) {
        operands.push_back(fpext_inst->value);
      }
    } else if (opcode == BITCAST) {
      BitCastInstruction *bitcast_inst =
          dynamic_cast<BitCastInstruction *>(inst);
      if (bitcast_inst && bitcast_inst->src) {
        operands.push_back(bitcast_inst->src);
      }
    } else {
      // 其他类型转换指令的通用处理
      std::vector<Operand> generic_operands =
          getGenericInstructionOperands(inst);
      operands.insert(operands.end(), generic_operands.begin(),
                      generic_operands.end());
    }
    break;
  }

  case SELECT: {
    // select指令：select i1 %cond, type %val1, type %val2
    SelectInstruction *select_inst = dynamic_cast<SelectInstruction *>(inst);
    if (select_inst) {
      if (select_inst->cond)
        operands.push_back(select_inst->cond);
      if (select_inst->op1)
        operands.push_back(select_inst->op1);
      if (select_inst->op2)
        operands.push_back(select_inst->op2);
    }
    break;
  }

  default:
    // 对于未明确处理的指令类型，尝试通用方法
    std::vector<Operand> generic_operands = getGenericInstructionOperands(inst);
    operands.insert(operands.end(), generic_operands.begin(),
                    generic_operands.end());
    break;
  }

  return operands;
}

bool SSAOptimizer::isRegisterOperand(const Operand operand) {
  if (!operand)
    return false;

  // 检查操作数类型是否为寄存器
  return operand->GetOperandType() == BasicOperand::REG;
}

Instruction
SSAOptimizer::findDefiningInstruction(const Operand operand,
                                      const std::map<int, LLVMBlock> &blocks) {
  if (!operand || !isRegisterOperand(operand)) {
    return nullptr;
  }

  int reg_num = getRegisterFromOperand(operand);
  if (reg_num == -1)
    return nullptr;

  // 在所有基本块中查找定义该寄存器的指令
  for (const auto &block_pair : blocks) {
    LLVMBlock block = block_pair.second;

    for (const auto &inst : block->Instruction_list) {
      if (!inst)
        continue;

      int result_reg = getInstructionResultRegister(inst);
      if (result_reg == reg_num) {
        return inst;
      }
    }
  }

  return nullptr;
}

bool SSAOptimizer::canPropagateConstants(const Instruction &inst) {
  if (!inst)
    return false;

  // 检查指令的所有操作数是否都是常量
  std::vector<Operand> operands = getInstructionOperands(inst);

  for (const auto &operand : operands) {
    if (isRegisterOperand(operand)) {
      int reg_num = getRegisterFromOperand(operand);
      if (constants_map.find(reg_num) == constants_map.end()) {
        return false; // 有非常量操作数
      }
    }
    // 立即数操作数已经是常量，跳过
  }

  return !operands.empty(); // 至少要有操作数才能传播
}

void SSAOptimizer::propagateConstantsInInstruction(Instruction &inst) {
  if (!inst)
    return;
  // 直接基于当前 constants_map 写回到指令字段（替换寄存器为立即数）
  rewriteOperandsWithConstMap(inst, constants_map);
}

void SSAOptimizer::updateConstantMapping(const Instruction &inst) {
  if (!inst)
    return;

  int result_reg = getInstructionResultRegister(inst);
  if (result_reg == -1)
    return;

  int opcode = inst->GetOpcode();

  // 根据指令类型更新常量映射
  switch (opcode) {
  case ADD:
  case SUB:
  case MUL_OP:
  case DIV_OP:
  case MOD_OP:
  case FADD:
  case FSUB:
  case FMUL:
  case FDIV: {
    // 算术运算：如果操作数都是常量，计算结果
    if (canFoldConstants(inst)) {
      ConstantValue result = computeArithmeticResult(inst);
      if (result.isConstant()) {
        constants_map[result_reg] = result;
      }
    } else {
      // 移除可能存在的映射
      constants_map.erase(result_reg);
    }
    break;
  }

  case ICMP:
  case FCMP: {
    // 比较运算
    if (canFoldConstants(inst)) {
      ConstantValue result = computeComparisonResult(inst);
      if (result.isConstant()) {
        constants_map[result_reg] = result;
      }
    } else {
      constants_map.erase(result_reg);
    }
    break;
  }

  case PHI: {
    // φ函数：检查所有输入是否为相同常量
    ConstantValue phi_result = computePhiResult(inst);
    if (phi_result.isConstant()) {
      constants_map[result_reg] = phi_result;
    } else {
      constants_map.erase(result_reg);
    }
    break;
  }

  default:
    // 其他指令：保守地移除常量信息
    constants_map.erase(result_reg);
    break;
  }
}

bool SSAOptimizer::canFoldConstants(const Instruction &inst) {
  if (!inst)
    return false;

  // 检查指令是否可以进行常量折叠
  std::vector<Operand> operands = getInstructionOperands(inst);

  if (operands.empty())
    return false;

  // 检查所有操作数是否都是常量
  for (const auto &operand : operands) {
    ConstantValue const_val = getConstantFromOperand(operand);
    if (!const_val.isConstant()) {
      return false;
    }
  }

  return true;
}

void SSAOptimizer::foldConstantsInInstruction(Instruction &inst) {
  if (!inst || !canFoldConstants(inst))
    return;

  int opcode = inst->GetOpcode();
  std::vector<Operand> operands = getInstructionOperands(inst);

  if (operands.size() < 2)
    return; // 需要至least两个操作数

  ConstantValue left = getConstantFromOperand(operands[0]);
  ConstantValue right = getConstantFromOperand(operands[1]);

  ConstantValue result;

  // 根据操作码计算结果
  if (isArithmeticInstruction(inst)) {
    result =
        evaluateArithmeticOp(static_cast<LLVMIROpcode>(opcode), left, right);
  } else if (isComparisonInstruction(inst)) {
    result =
        evaluateComparisonOp(static_cast<LLVMIROpcode>(opcode), left, right);
  }

  if (result.isConstant()) {
    // 将整个指令替换为常量赋值
    replaceInstructionWithConstant(inst, result);

    // 更新常量映射
    int result_reg = getInstructionResultRegister(inst);
    if (result_reg != -1) {
      constants_map[result_reg] = result;
    }
  }
}

bool SSAOptimizer::isCopyInstruction(const Instruction &inst) {
  if (!inst)
    return false;

  // 检查是否是复制指令
  // 在SSA中，复制可能以多种形式出现：
  // 1. 简单的load/store序列
  // 2. φ函数只有一个有效输入
  // 3. 类型转换指令（如果不改变值）

  int opcode = inst->GetOpcode();

  switch (opcode) {
  case PHI: {
    // φ函数如果所有输入都是同一个值，则为复制
    return isPhiCopyCandidate(inst);
  }

  case BITCAST: {
    // 位转换在某些情况下相当于复制
    return true;
  }

  case ZEXT:
  case SITOFP:
  case FPTOSI: {
    // 类型转换可能是复制（需要更细致的分析）
    return false; // 暂时保守处理
  }

  default:
    return false;
  }
}

bool SSAOptimizer::extractCopyRegisters(const Instruction &inst, int &dest_reg,
                                        int &src_reg) {
  if (!inst || !isCopyInstruction(inst))
    return false;

  dest_reg = getInstructionResultRegister(inst);
  if (dest_reg == -1)
    return false;

  int opcode = inst->GetOpcode();

  switch (opcode) {
  case PHI: {
    src_reg = getPhiSingleSource(inst);
    return src_reg != -1 && src_reg != dest_reg;
  }

  case BITCAST: {
    std::vector<Operand> operands = getInstructionOperands(inst);
    if (!operands.empty() && isRegisterOperand(operands[0])) {
      src_reg = getRegisterFromOperand(operands[0]);
      return src_reg != -1 && src_reg != dest_reg;
    }
    return false;
  }

  default:
    return false;
  }
}

bool SSAOptimizer::replaceCopyUsages(
    Instruction &inst, const std::unordered_map<int, int> &copy_map) {
  if (!inst || copy_map.empty())
    return false;

  // 先判断是否命中
  bool hit = false;
  std::vector<Operand> ops = getInstructionOperands(inst);
  for (auto &op : ops) {
    if (isRegisterOperand(op)) {
      int r = getRegisterFromOperand(op);
      if (copy_map.find(r) != copy_map.end()) {
        hit = true;
        break;
      }
    }
  }
  if (!hit)
    return false;

  // 命中则做写回
  return rewriteOperandsWithRegMap(inst, copy_map);
}

void SSAOptimizer::replaceInstructionWithCopy(Instruction &inst,
                                              int source_reg) {
  // 将当前指令替换为 "dst = %source + 0" 的算术复制
  int result_reg = getInstructionResultRegister(inst);
  if (result_reg == -1)
    return;

  Instruction old = inst;
  Operand dst = GetNewRegOperand(result_reg);
  Operand src = GetNewRegOperand(source_reg);

  // 使用 I32 的 ADD 作为通用拷贝
  inst = new ArithmeticInstruction(ADD, I32, src, new ImmI32Operand(0), dst);

  // 可选：保留常量映射的传递
  auto it = constants_map.find(source_reg);
  if (it != constants_map.end()) {
    constants_map[result_reg] = it->second;
  }

  // 释放旧指令
  delete old;
}

void SSAOptimizer::replaceWithIdentity(Instruction &inst,
                                       const Operand &source) {
  // 将指令替换为简单复制（reg: x+0），或常量赋值（imm）
  int result_reg = getInstructionResultRegister(inst);
  if (result_reg == -1 || !source)
    return;

  if (auto *immI = dynamic_cast<ImmI32Operand *>(source)) {
    replaceWithConstant(inst, ConstantValue(immI->GetIntImmVal()));
    return;
  } else if (auto *immF = dynamic_cast<ImmF32Operand *>(source)) {
    ConstantValue c = ConstantValue::Float(immF->GetFloatVal());
    replaceWithConstant(inst, c);
    return;
  } else if (!isRegisterOperand(source)) {
    // 其他非常见类型，保守放弃
    return;
  }

  Instruction old = inst;
  int source_reg = getRegisterFromOperand(source);
  Operand dst = GetNewRegOperand(result_reg);
  Operand src = GetNewRegOperand(source_reg);

  inst = new ArithmeticInstruction(ADD, I32, src, new ImmI32Operand(0), dst);

  // 同步常量映射
  auto it = constants_map.find(source_reg);
  if (it != constants_map.end()) {
    constants_map[result_reg] = it->second;
  }

  delete old;
}

void SSAOptimizer::replaceWithConstant(Instruction &inst,
                                       const ConstantValue &constant) {
  // 将指令整体替换为常量赋值（INT 用 ADD; FLOAT 用 FADD）
  int result_reg = getInstructionResultRegister(inst);
  if (result_reg == -1 || !constant.isConstant())
    return;

  Instruction old = inst;
  Operand dst = GetNewRegOperand(result_reg);

  if (constant.isInt()) {
    inst =
        new ArithmeticInstruction(ADD, I32, new ImmI32Operand(constant.int_val),
                                  new ImmI32Operand(0), dst);
  } else if (constant.isFloat()) {
    inst = new ArithmeticInstruction(FADD, F32,
                                     new ImmF32Operand(constant.float_val),
                                     new ImmF32Operand(0.0f), dst);
  } else {
    return; // 未知类型，放弃替换
  }

  constants_map[result_reg] = constant;
  delete old;
}

// 工具1：单个寄存器操作数的重写（若命中映射则返回新寄存器）
static inline Operand
remapRegIfNeeded(Operand op, const std::unordered_map<int, int> &reg_map) {
  if (!op)
    return op;
  if (op->GetOperandType() == BasicOperand::REG) {
    if (auto *r = dynamic_cast<RegOperand *>(op)) {
      auto it = reg_map.find(r->GetRegNo());
      if (it != reg_map.end()) {
        return GetNewRegOperand(it->second);
      }
    }
  }
  return op;
}

// 工具2：单个操作数的常量写回（若寄存器在常量表中，则替换为立即数）
static inline Operand
replaceWithConstIfKnown(Operand op,
                        const std::unordered_map<int, ConstantValue> &consts) {
  if (!op)
    return op;
  if (op->GetOperandType() == BasicOperand::REG) {
    if (auto *r = dynamic_cast<RegOperand *>(op)) {
      auto it = consts.find(r->GetRegNo());
      if (it != consts.end() && it->second.isConstant()) {
        const auto &cv = it->second;
        if (cv.isInt())
          return new ImmI32Operand(cv.int_val);
        if (cv.isFloat())
          return new ImmF32Operand(cv.float_val);
      }
    }
  }
  return op;
}

// 工具3：对整条指令基于“寄存器重映射表”做写回
static bool
rewriteOperandsWithRegMap(Instruction ins,
                          const std::unordered_map<int, int> &reg_map) {
  if (!ins || reg_map.empty())
    return false;
  bool changed = false;

  if (auto *arith = dynamic_cast<ArithmeticInstruction *>(ins)) {
    Operand n1 = remapRegIfNeeded(arith->GetOp1(), reg_map);
    Operand n2 = remapRegIfNeeded(arith->GetOp2(), reg_map);
    Operand n3 =
        arith->GetOp3() ? remapRegIfNeeded(arith->GetOp3(), reg_map) : nullptr;
    changed |= (n1 != arith->GetOp1()) || (n2 != arith->GetOp2()) ||
               (n3 != arith->GetOp3());
    arith->op1 = n1;
    arith->op2 = n2;
    if (arith->GetOp3())
      arith->op3 = n3;
  } else if (auto *load = dynamic_cast<LoadInstruction *>(ins)) {
    Operand np = remapRegIfNeeded(load->GetPointer(), reg_map);
    changed |= (np != load->GetPointer());
    load->pointer = np;
  } else if (auto *store = dynamic_cast<StoreInstruction *>(ins)) {
    Operand nv = remapRegIfNeeded(store->GetValue(), reg_map);
    Operand np = remapRegIfNeeded(store->GetPointer(), reg_map);
    changed |= (nv != store->GetValue()) || (np != store->GetPointer());
    store->value = nv;
    store->pointer = np;
  } else if (auto *icmp = dynamic_cast<IcmpInstruction *>(ins)) {
    Operand n1 = remapRegIfNeeded(icmp->GetOp1(), reg_map);
    Operand n2 = remapRegIfNeeded(icmp->GetOp2(), reg_map);
    changed |= (n1 != icmp->GetOp1()) || (n2 != icmp->GetOp2());
    icmp->op1 = n1;
    icmp->op2 = n2;
  } else if (auto *fcmp = dynamic_cast<FcmpInstruction *>(ins)) {
    Operand n1 = remapRegIfNeeded(fcmp->GetOp1(), reg_map);
    Operand n2 = remapRegIfNeeded(fcmp->GetOp2(), reg_map);
    changed |= (n1 != fcmp->GetOp1()) || (n2 != fcmp->GetOp2());
    fcmp->op1 = n1;
    fcmp->op2 = n2;
  } else if (auto *phi = dynamic_cast<PhiInstruction *>(ins)) {
    for (auto &p : phi->phi_list) {
      // phi_list: first = label, second = value
      Operand nv = remapRegIfNeeded(p.second, reg_map);
      changed |= (nv != p.second);
      p.second = nv;
    }
  } else if (auto *call = dynamic_cast<CallInstruction *>(ins)) {
    for (auto &arg : call->args) {
      Operand na = remapRegIfNeeded(arg.second, reg_map);
      changed |= (na != arg.second);
      arg.second = na;
    }
  } else if (auto *brc = dynamic_cast<BrCondInstruction *>(ins)) {
    Operand nc = remapRegIfNeeded(brc->GetCond(), reg_map);
    changed |= (nc != brc->GetCond());
    brc->cond = nc;
  } else if (auto *ret = dynamic_cast<RetInstruction *>(ins)) {
    if (ret->GetRetVal()) {
      Operand nr = remapRegIfNeeded(ret->GetRetVal(), reg_map);
      changed |= (nr != ret->GetRetVal());
      ret->ret_val = nr;
    }
  } else if (auto *gep = dynamic_cast<GetElementptrInstruction *>(ins)) {
    Operand np = remapRegIfNeeded(gep->GetPtrVal(), reg_map);
    changed |= (np != gep->GetPtrVal());
    gep->ptrval = np;
    for (auto &idx : gep->indexes) {
      Operand nx = remapRegIfNeeded(idx, reg_map);
      changed |= (nx != idx);
      idx = nx;
    }
  } else if (auto *zext = dynamic_cast<ZextInstruction *>(ins)) {
    Operand nv = remapRegIfNeeded(zext->value, reg_map);
    changed |= (nv != zext->value);
    zext->value = nv;
  } else if (auto *sitofp = dynamic_cast<SitofpInstruction *>(ins)) {
    Operand nv = remapRegIfNeeded(sitofp->value, reg_map);
    changed |= (nv != sitofp->value);
    sitofp->value = nv;
  } else if (auto *fptosi = dynamic_cast<FptosiInstruction *>(ins)) {
    Operand nv = remapRegIfNeeded(fptosi->value, reg_map);
    changed |= (nv != fptosi->value);
    fptosi->value = nv;
  } else if (auto *fpext = dynamic_cast<FpextInstruction *>(ins)) {
    Operand nv = remapRegIfNeeded(fpext->value, reg_map);
    changed |= (nv != fpext->value);
    fpext->value = nv;
  } else if (auto *bitcast = dynamic_cast<BitCastInstruction *>(ins)) {
    Operand ns = remapRegIfNeeded(bitcast->src, reg_map);
    changed |= (ns != bitcast->src);
    bitcast->src = ns;
  } else if (auto *select = dynamic_cast<SelectInstruction *>(ins)) {
    Operand nc = remapRegIfNeeded(select->cond, reg_map);
    Operand n1 = remapRegIfNeeded(select->op1, reg_map);
    Operand n2 = remapRegIfNeeded(select->op2, reg_map);
    changed |=
        (nc != select->cond) || (n1 != select->op1) || (n2 != select->op2);
    select->cond = nc;
    select->op1 = n1;
    select->op2 = n2;
  } else if (auto *trunc = dynamic_cast<TruncInstruction *>(ins)) {
    Operand nv = remapRegIfNeeded(trunc->value, reg_map);
    changed |= (nv != trunc->value);
    trunc->value = nv;
  }

  return changed;
}

// 工具4：对整条指令基于“常量表”做写回（把可替换的寄存器操作数替换为立即数）
static bool rewriteOperandsWithConstMap(
    Instruction ins, const std::unordered_map<int, ConstantValue> &consts) {
  if (!ins || consts.empty())
    return false;
  bool changed = false;

  if (auto *arith = dynamic_cast<ArithmeticInstruction *>(ins)) {
    Operand n1 = replaceWithConstIfKnown(arith->GetOp1(), consts);
    Operand n2 = replaceWithConstIfKnown(arith->GetOp2(), consts);
    Operand n3 = arith->GetOp3()
                     ? replaceWithConstIfKnown(arith->GetOp3(), consts)
                     : nullptr;
    changed |= (n1 != arith->GetOp1()) || (n2 != arith->GetOp2()) ||
               (n3 != arith->GetOp3());
    arith->op1 = n1;
    arith->op2 = n2;
    if (arith->GetOp3())
      arith->op3 = n3;
  } else if (auto *load = dynamic_cast<LoadInstruction *>(ins)) {
    Operand np = replaceWithConstIfKnown(load->GetPointer(), consts);
    changed |= (np != load->GetPointer());
    load->pointer = np;
  } else if (auto *store = dynamic_cast<StoreInstruction *>(ins)) {
    Operand nv = replaceWithConstIfKnown(store->GetValue(), consts);
    Operand np = replaceWithConstIfKnown(store->GetPointer(), consts);
    changed |= (nv != store->GetValue()) || (np != store->GetPointer());
    store->value = nv;
    store->pointer = np;
  } else if (auto *icmp = dynamic_cast<IcmpInstruction *>(ins)) {
    Operand n1 = replaceWithConstIfKnown(icmp->GetOp1(), consts);
    Operand n2 = replaceWithConstIfKnown(icmp->GetOp2(), consts);
    changed |= (n1 != icmp->GetOp1()) || (n2 != icmp->GetOp2());
    icmp->op1 = n1;
    icmp->op2 = n2;
  } else if (auto *fcmp = dynamic_cast<FcmpInstruction *>(ins)) {
    Operand n1 = replaceWithConstIfKnown(fcmp->GetOp1(), consts);
    Operand n2 = replaceWithConstIfKnown(fcmp->GetOp2(), consts);
    changed |= (n1 != fcmp->GetOp1()) || (n2 != fcmp->GetOp2());
    fcmp->op1 = n1;
    fcmp->op2 = n2;
  } else if (auto *phi = dynamic_cast<PhiInstruction *>(ins)) {
    for (auto &p : phi->phi_list) {
      // phi_list: first = label, second = value
      Operand nv = replaceWithConstIfKnown(p.second, consts);
      changed |= (nv != p.second);
      p.second = nv;
    }
  } else if (auto *call = dynamic_cast<CallInstruction *>(ins)) {
    for (auto &arg : call->args) {
      Operand na = replaceWithConstIfKnown(arg.second, consts);
      changed |= (na != arg.second);
      arg.second = na;
    }
  } else if (auto *brc = dynamic_cast<BrCondInstruction *>(ins)) {
    Operand nc = replaceWithConstIfKnown(brc->GetCond(), consts);
    changed |= (nc != brc->GetCond());
    brc->cond = nc;
  } else if (auto *ret = dynamic_cast<RetInstruction *>(ins)) {
    if (ret->GetRetVal()) {
      Operand nr = replaceWithConstIfKnown(ret->GetRetVal(), consts);
      changed |= (nr != ret->GetRetVal());
      ret->ret_val = nr;
    }
  } else if (auto *gep = dynamic_cast<GetElementptrInstruction *>(ins)) {
    Operand np = replaceWithConstIfKnown(gep->GetPtrVal(), consts);
    changed |= (np != gep->GetPtrVal());
    gep->ptrval = np;
    for (auto &idx : gep->indexes) {
      Operand nx = replaceWithConstIfKnown(idx, consts);
      changed |= (nx != idx);
      idx = nx;
    }
  } else if (auto *zext = dynamic_cast<ZextInstruction *>(ins)) {
    Operand nv = replaceWithConstIfKnown(zext->value, consts);
    changed |= (nv != zext->value);
    zext->value = nv;
  } else if (auto *sitofp = dynamic_cast<SitofpInstruction *>(ins)) {
    Operand nv = replaceWithConstIfKnown(sitofp->value, consts);
    changed |= (nv != sitofp->value);
    sitofp->value = nv;
  } else if (auto *fptosi = dynamic_cast<FptosiInstruction *>(ins)) {
    Operand nv = replaceWithConstIfKnown(fptosi->value, consts);
    changed |= (nv != fptosi->value);
    fptosi->value = nv;
  } else if (auto *fpext = dynamic_cast<FpextInstruction *>(ins)) {
    Operand nv = replaceWithConstIfKnown(fpext->value, consts);
    changed |= (nv != fpext->value);
    fpext->value = nv;
  } else if (auto *bitcast = dynamic_cast<BitCastInstruction *>(ins)) {
    Operand ns = replaceWithConstIfKnown(bitcast->src, consts);
    changed |= (ns != bitcast->src);
    bitcast->src = ns;
  } else if (auto *select = dynamic_cast<SelectInstruction *>(ins)) {
    Operand nc = replaceWithConstIfKnown(select->cond, consts);
    Operand n1 = replaceWithConstIfKnown(select->op1, consts);
    Operand n2 = replaceWithConstIfKnown(select->op2, consts);
    changed |=
        (nc != select->cond) || (n1 != select->op1) || (n2 != select->op2);
    select->cond = nc;
    select->op1 = n1;
    select->op2 = n2;
  } else if (auto *trunc = dynamic_cast<TruncInstruction *>(ins)) {
    Operand nv = replaceWithConstIfKnown(trunc->value, consts);
    changed |= (nv != trunc->value);
    trunc->value = nv;
  }

  return changed;
}

// ============================================================================
// 追加：高级优化所需的通用工具与占位实现
// ============================================================================

// 轻量常量容器
struct ConstantValue {
  bool known = false;
  bool is_int = true;
  int int_val = 0;
  float float_val = 0.0f;

  ConstantValue() = default;
  explicit ConstantValue(int v) : known(true), is_int(true), int_val(v) {}
  static ConstantValue Float(float f) {
    ConstantValue c;
    c.known = true;
    c.is_int = false;
    c.float_val = f;
    return c;
  }
  bool isConstant() const { return known; }
  bool isInt() const { return known && is_int; }
  bool isFloat() const { return known && !is_int; }
};

// 本文件内使用的全局表（与类成员同名以兼容调用处）
static std::unordered_map<int, ConstantValue> constants_map;
static std::unordered_set<Instruction> useful_instructions;

// 小工具
static inline bool isImmI32(Operand o, int &v) {
  if (auto *imm = dynamic_cast<ImmI32Operand *>(o)) {
    v = imm->GetIntImmVal();
    return true;
  }
  return false;
}
static inline bool isPowerOfTwoInt(int n) {
  return n > 0 && (n & (n - 1)) == 0;
}
static inline int log2_floor_u32(unsigned int x) {
  int k = -1;
  while (x) {
    x >>= 1;
    ++k;
  }
  return std::max(k, 0);
}

int SSAOptimizer::getRegisterFromOperand(Operand operand) {
  if (auto *r = dynamic_cast<RegOperand *>(operand))
    return r->GetRegNo();
  return -1;
}

int SSAOptimizer::getInstructionResultRegister(const Instruction &inst) {
  if (!inst)
    return -1;
  if (auto *a = dynamic_cast<ArithmeticInstruction *>(inst))
    return getRegisterFromOperand(a->GetResult());
  if (auto *i = dynamic_cast<IcmpInstruction *>(inst))
    return getRegisterFromOperand(i->GetResult());
  if (auto *f = dynamic_cast<FcmpInstruction *>(inst))
    return getRegisterFromOperand(f->GetResult());
  if (auto *p = dynamic_cast<PhiInstruction *>(inst))
    return getRegisterFromOperand(p->GetResult());
  if (auto *l = dynamic_cast<LoadInstruction *>(inst))
    return getRegisterFromOperand(l->GetResult());
  if (auto *s = dynamic_cast<SelectInstruction *>(inst))
    return getRegisterFromOperand(s->result);
  if (auto *g = dynamic_cast<GetElementptrInstruction *>(inst))
    return getRegisterFromOperand(g->GetResult());
  if (auto *z = dynamic_cast<ZextInstruction *>(inst))
    return getRegisterFromOperand(z->result);
  if (auto *si = dynamic_cast<SitofpInstruction *>(inst))
    return getRegisterFromOperand(si->result);
  if (auto *fp = dynamic_cast<FptosiInstruction *>(inst))
    return getRegisterFromOperand(fp->result);
  if (auto *fe = dynamic_cast<FpextInstruction *>(inst))
    return getRegisterFromOperand(fe->result);
  if (auto *bc = dynamic_cast<BitCastInstruction *>(inst))
    return getRegisterFromOperand(bc->dst);
  if (auto *tr = dynamic_cast<TruncInstruction *>(inst))
    return getRegisterFromOperand(tr->result);
  if (auto *c = dynamic_cast<CallInstruction *>(inst))
    return getRegisterFromOperand(c->GetResult());
  if (auto *al = dynamic_cast<AllocaInstruction *>(inst))
    return getRegisterFromOperand(al->GetResult());
  return -1;
}

bool SSAOptimizer::operandEquals(const Operand &a, const Operand &b) {
  if (!a || !b)
    return a == b;
  if (a->GetOperandType() != b->GetOperandType())
    return false;
  if (auto *ra = dynamic_cast<RegOperand *>(a))
    return ra->GetRegNo() == dynamic_cast<RegOperand *>(b)->GetRegNo();
  if (auto *ia = dynamic_cast<ImmI32Operand *>(a))
    return ia->GetIntImmVal() ==
           dynamic_cast<ImmI32Operand *>(b)->GetIntImmVal();
  if (auto *fa = dynamic_cast<ImmF32Operand *>(a))
    return fa->GetFloatVal() == dynamic_cast<ImmF32Operand *>(b)->GetFloatVal();
  if (auto *la = dynamic_cast<LabelOperand *>(a))
    return la->GetLabelNo() == dynamic_cast<LabelOperand *>(b)->GetLabelNo();
  if (auto *ga = dynamic_cast<GlobalOperand *>(a))
    return ga->GetFullName() == b->GetFullName();
  return a->GetFullName() == b->GetFullName();
}

std::vector<Operand>
SSAOptimizer::getGenericInstructionOperands(const Instruction &) {
  return {}; // 保守：未知类型不暴露任何操作数
}

ConstantValue SSAOptimizer::getConstantFromOperand(const Operand &op) {
  if (!op)
    return ConstantValue();
  if (auto *i = dynamic_cast<ImmI32Operand *>(op))
    return ConstantValue(i->GetIntImmVal());
  if (auto *f = dynamic_cast<ImmF32Operand *>(op))
    return ConstantValue::Float(f->GetFloatVal());
  if (auto *r = dynamic_cast<RegOperand *>(op)) {
    auto it = constants_map.find(r->GetRegNo());
    if (it != constants_map.end())
      return it->second;
  }
  return ConstantValue();
}

bool SSAOptimizer::isArithmeticInstruction(const Instruction &inst) {
  if (!inst)
    return false;
  switch (inst->GetOpcode()) {
  case ADD:
  case SUB:
  case MUL_OP:
  case DIV_OP:
  case MOD_OP:
  case FADD:
  case FSUB:
  case FMUL:
  case FDIV:
  case BITAND:
  case BITOR:
  case BITXOR:
  case SHL:
  case ASHR:
  case LSHR:
    return true;
  default:
    return false;
  }
}

bool SSAOptimizer::isComparisonInstruction(const Instruction &inst) {
  if (!inst)
    return false;
  return inst->GetOpcode() == ICMP || inst->GetOpcode() == FCMP;
}

ConstantValue SSAOptimizer::evaluateArithmeticOp(LLVMIROpcode opc,
                                                 const ConstantValue &l,
                                                 const ConstantValue &r) {
  if (!l.isConstant() || !r.isConstant())
    return ConstantValue();
  switch (opc) {
  case ADD:
    return ConstantValue(l.int_val + r.int_val);
  case SUB:
    return ConstantValue(l.int_val - r.int_val);
  case MUL_OP:
    return ConstantValue(l.int_val * r.int_val);
  case DIV_OP:
    if (r.int_val != 0)
      return ConstantValue(l.int_val / r.int_val);
    else
      return ConstantValue();
  case MOD_OP:
    if (r.int_val != 0)
      return ConstantValue(l.int_val % r.int_val);
    else
      return ConstantValue();
  case FADD:
    return ConstantValue::Float(
        l.isFloat() ? l.float_val
                    : (float)l.int_val +
                          (r.isFloat() ? r.float_val : (float)r.int_val));
  case FSUB:
    return ConstantValue::Float((l.isFloat() ? l.float_val : (float)l.int_val) -
                                (r.isFloat() ? r.float_val : (float)r.int_val));
  case FMUL:
    return ConstantValue::Float((l.isFloat() ? l.float_val : (float)l.int_val) *
                                (r.isFloat() ? r.float_val : (float)r.int_val));
  case FDIV:
    if ((r.isFloat() ? r.float_val : (float)r.int_val) != 0.0f)
      return ConstantValue::Float(
          (l.isFloat() ? l.float_val : (float)l.int_val) /
          (r.isFloat() ? r.float_val : (float)r.int_val));
    else
      return ConstantValue();
  case BITAND:
    return ConstantValue(l.int_val & r.int_val);
  case BITOR:
    return ConstantValue(l.int_val | r.int_val);
  case BITXOR:
    return ConstantValue(l.int_val ^ r.int_val);
  case SHL:
    return ConstantValue(l.int_val << r.int_val);
  case ASHR:
    return ConstantValue(l.int_val >> r.int_val);
  case LSHR:
    return ConstantValue((int)((unsigned)l.int_val >> r.int_val));
  default:
    return ConstantValue();
  }
}

ConstantValue SSAOptimizer::evaluateComparisonOp(LLVMIROpcode opc,
                                                 const ConstantValue &l,
                                                 const ConstantValue &r) {
  if (!l.isConstant() || !r.isConstant())
    return ConstantValue();
  switch (opc) {
  case ICMP: // 不含具体cond，这里仅示例，不在此使用
    return ConstantValue();
  case FCMP:
    return ConstantValue();
  default:
    return ConstantValue();
  }
}

ConstantValue SSAOptimizer::computeArithmeticResult(const Instruction &inst) {
  auto ops = getInstructionOperands(inst);
  if (ops.size() < 2)
    return ConstantValue();
  return evaluateArithmeticOp(static_cast<LLVMIROpcode>(inst->GetOpcode()),
                              getConstantFromOperand(ops[0]),
                              getConstantFromOperand(ops[1]));
}

ConstantValue SSAOptimizer::computeComparisonResult(const Instruction &inst) {
  auto ops = getInstructionOperands(inst);
  if (ops.size() < 2)
    return ConstantValue();
  // 简化：仅支持整数 icmp eq/ne，其余返回未知
  if (auto *icmp = dynamic_cast<IcmpInstruction *>(inst)) {
    auto L = getConstantFromOperand(ops[0]);
    auto R = getConstantFromOperand(ops[1]);
    if (L.isInt() && R.isInt()) {
      bool res = false;
      switch (icmp->GetCond()) {
      case eq:
        res = (L.int_val == R.int_val);
        break;
      case ne:
        res = (L.int_val != R.int_val);
        break;
      case sgt:
        res = (L.int_val > R.int_val);
        break;
      case sge:
        res = (L.int_val >= R.int_val);
        break;
      case slt:
        res = (L.int_val < R.int_val);
        break;
      case sle:
        res = (L.int_val <= R.int_val);
        break;
      default:
        return ConstantValue();
      }
      return ConstantValue(res ? 1 : 0);
    }
  }
  return ConstantValue();
}

ConstantValue SSAOptimizer::computePhiResult(const Instruction &inst) {
  auto *phi = dynamic_cast<PhiInstruction *>(inst);
  if (!phi)
    return ConstantValue();
  bool first_set = false;
  ConstantValue base;
  for (auto &pr : phi->phi_list) {
    ConstantValue cv = getConstantFromOperand(pr.second); // value
    if (!cv.isConstant())
      return ConstantValue();
    if (!first_set) {
      base = cv;
      first_set = true;
    } else {
      if (base.isInt() != cv.isInt())
        return ConstantValue();
      if (base.isInt() && base.int_val != cv.int_val)
        return ConstantValue();
      if (base.isFloat() && base.float_val != cv.float_val)
        return ConstantValue();
    }
  }
  return base;
}

void SSAOptimizer::replaceInstructionWithConstant(Instruction &inst,
                                                  const ConstantValue &c) {
  if (!c.isConstant())
    return;
  replaceWithConstant(inst, c);
}

void SSAOptimizer::addUsersToWorkList(
    int def_reg, const std::map<int, LLVMBlock> &blocks,
    std::queue<std::pair<int, size_t>> &work_list) {
  for (const auto &bp : blocks) {
    int bid = bp.first;
    LLVMBlock blk = bp.second;
    for (size_t i = 0; i < blk->Instruction_list.size(); ++i) {
      Instruction ins = blk->Instruction_list[i];
      auto ops = getInstructionOperands(ins);
      for (auto &op : ops) {
        if (isRegisterOperand(op) && getRegisterFromOperand(op) == def_reg) {
          work_list.push({bid, i});
          break;
        }
      }
    }
  }
}

void SSAOptimizer::markInstructionAsUseful(const Instruction &inst) {
  if (inst)
    useful_instructions.insert(inst);
}

void SSAOptimizer::markControlDependencies(
    const Instruction &inst, const std::map<int, LLVMBlock> &blocks,
    std::queue<Instruction> &work) {
  if (!inst)
    return;
  int opc = inst->GetOpcode();
  if (opc == BR_COND) {
    // 标记分支自身和条件的定义
    work.push(inst);
    auto *br = dynamic_cast<BrCondInstruction *>(inst);
    if (br && br->GetCond() && isRegisterOperand(br->GetCond())) {
      int r = getRegisterFromOperand(br->GetCond());
      // 找到定义该寄存器的指令并加入
      for (const auto &bp : blocks) {
        for (auto &ins : bp.second->Instruction_list) {
          if (getInstructionResultRegister(ins) == r) {
            work.push(ins);
            break;
          }
        }
      }
    }
  } else if (opc == BR_UNCOND) {
    work.push(inst);
  }
}

std::vector<int> SSAOptimizer::getBranchTargets(const Instruction &inst) {
  std::vector<int> targets;
  if (!inst)
    return targets;
  if (auto *buc = dynamic_cast<BrUncondInstruction *>(inst)) {
    if (auto *lab = dynamic_cast<LabelOperand *>(buc->GetLabel()))
      targets.push_back(lab->GetLabelNo());
  } else if (auto *bc = dynamic_cast<BrCondInstruction *>(inst)) {
    if (auto *t = dynamic_cast<LabelOperand *>(bc->GetTrueLabel()))
      targets.push_back(t->GetLabelNo());
    if (auto *f = dynamic_cast<LabelOperand *>(bc->GetFalseLabel()))
      targets.push_back(f->GetLabelNo());
  }
  return targets;
}

bool SSAOptimizer::isPhiCopyCandidate(const Instruction &inst) {
  auto *phi = dynamic_cast<PhiInstruction *>(inst);
  if (!phi)
    return false;
  Operand base = nullptr;
  bool init = false;
  for (auto &pr : phi->phi_list) {
    Operand val = pr.second; // value
    if (!init) {
      base = val;
      init = true;
    } else if (!operandEquals(base, val))
      return false;
  }
  return init && base != nullptr;
}

int SSAOptimizer::getPhiSingleSource(const Instruction &inst) {
  auto *phi = dynamic_cast<PhiInstruction *>(inst);
  if (!phi)
    return -1;
  int src = -1;
  bool set = false;
  int self = getInstructionResultRegister(inst);
  for (auto &pr : phi->phi_list) {
    Operand val = pr.second;
    if (isRegisterOperand(val)) {
      int r = getRegisterFromOperand(val);
      if (r == self)
        continue;
      if (!set) {
        src = r;
        set = true;
      } else if (src != r)
        return -1;
    } else {
      // 立即数与寄存器混合，不视为单源复制
      return -1;
    }
  }
  return set ? src : -1;
}

void SSAOptimizer::removeTrivialCopies(
    std::map<int, LLVMBlock> &blocks,
    const std::unordered_map<int, int> &copy_map) {
  if (copy_map.empty())
    return;
  for (auto &bp : blocks) {
    auto &list = bp.second->Instruction_list;
    for (auto it = list.begin(); it != list.end();) {
      int def = getInstructionResultRegister(*it);
      if (def != -1) {
        auto f = copy_map.find(def);
        if (f != copy_map.end() && isCopyInstruction(*it)) {
          delete *it;
          it = list.erase(it);
          continue;
        }
      }
      ++it;
    }
  }
}

bool SSAOptimizer::isPureFunctionalInstruction(const Instruction &inst) {
  if (!inst)
    return false;
  switch (inst->GetOpcode()) {
  case ADD:
  case SUB:
  case MUL_OP:
  case DIV_OP:
  case MOD_OP:
  case FADD:
  case FSUB:
  case FMUL:
  case FDIV:
  case ICMP:
  case FCMP:
  case BITAND:
  case BITOR:
  case BITXOR:
  case SHL:
  case ASHR:
  case LSHR:
  case ZEXT:
  case SITOFP:
  case FPTOSI:
  case FPEXT:
  case BITCAST:
  case TRUNC:
  case SELECT:
  case GETELEMENTPTR:
    return true;
  default:
    return false; // 排除 LOAD/STORE/CALL/PHI/BR/RET/ALLOCA 等
  }
}

static std::string operandKey(Operand op) {
  if (!op)
    return "_";
  switch (op->GetOperandType()) {
  case BasicOperand::REG:
    return std::string("r") +
           std::to_string(dynamic_cast<RegOperand *>(op)->GetRegNo());
  case BasicOperand::IMMI32:
    return std::string("i") +
           std::to_string(dynamic_cast<ImmI32Operand *>(op)->GetIntImmVal());
  case BasicOperand::IMMF32:
    return std::string("f") +
           std::to_string(dynamic_cast<ImmF32Operand *>(op)->GetFloatVal());
  case BasicOperand::LABEL:
    return std::string("L") +
           std::to_string(dynamic_cast<LabelOperand *>(op)->GetLabelNo());
  case BasicOperand::GLOBAL:
    return std::string("g") + dynamic_cast<GlobalOperand *>(op)->GetFullName();
  case BasicOperand::IMMI64:
    return std::string("I64");
  }
  return op->GetFullName();
}

static bool isCommutative(LLVMIROpcode opc) {
  switch (opc) {
  case ADD:
  case MUL_OP:
  case BITAND:
  case BITOR:
  case BITXOR:
  case FADD:
  case FMUL:
    return true;
  default:
    return false;
  }
}

std::string SSAOptimizer::generateExpressionString(const Instruction &inst) {
  if (!inst)
    return "";
  std::ostringstream oss;
  int opc = inst->GetOpcode();
  oss << opc << ":";
  if (auto *a = dynamic_cast<ArithmeticInstruction *>(inst)) {
    Operand a1 = a->GetOp1(), a2 = a->GetOp2();
    std::string s1 = operandKey(a1), s2 = operandKey(a2);
    if (isCommutative((LLVMIROpcode)opc) && s2 < s1)
      std::swap(s1, s2);
    oss << (int)a->GetType() << ":" << s1 << "," << s2;
  } else if (auto *i = dynamic_cast<IcmpInstruction *>(inst)) {
    std::string s1 = operandKey(i->GetOp1()), s2 = operandKey(i->GetOp2());
    if (s2 < s1) { /*icmp 非交换，保持顺序*/
    }
    oss << (int)i->GetType() << ":" << (int)i->GetCond() << ":" << s1 << ","
        << s2;
  } else if (auto *f = dynamic_cast<FcmpInstruction *>(inst)) {
    oss << (int)f->GetType() << ":" << (int)f->GetCond() << ":"
        << operandKey(f->GetOp1()) << "," << operandKey(f->GetOp2());
  } else if (auto *z = dynamic_cast<ZextInstruction *>(inst)) {
    oss << "zext:" << (int)z->from_type << ":" << (int)z->to_type << ":"
        << operandKey(z->value);
  } else if (auto *si = dynamic_cast<SitofpInstruction *>(inst)) {
    oss << "sitofp:" << operandKey(si->value);
  } else if (auto *fp = dynamic_cast<FptosiInstruction *>(inst)) {
    oss << "fptosi:" << operandKey(fp->value);
  } else if (auto *fe = dynamic_cast<FpextInstruction *>(inst)) {
    oss << "fpext:" << operandKey(fe->value);
  } else if (auto *bc = dynamic_cast<BitCastInstruction *>(inst)) {
    oss << "bitcast:" << (int)bc->src_type << ":" << (int)bc->dst_type << ":"
        << operandKey(bc->src);
  } else if (auto *sel = dynamic_cast<SelectInstruction *>(inst)) {
    oss << "select:" << (int)sel->GetType() << ":" << operandKey(sel->cond)
        << ":" << operandKey(sel->op1) << ":" << operandKey(sel->op2);
  } else if (auto *gep = dynamic_cast<GetElementptrInstruction *>(inst)) {
    oss << "gep:" << (int)gep->GetType() << ":" << operandKey(gep->GetPtrVal());
    for (auto &idx : gep->GetIndexes())
      oss << "," << operandKey(idx);
  } else {
    return "";
  }
  return oss.str();
}

void SSAOptimizer::strengthReduction(LLVMIR &ir) {
  for (auto &func_pair : ir.function_block_map) {
    auto &blocks = func_pair.second;
    for (auto &bp : blocks) {
      auto &ilist = bp.second->Instruction_list;
      for (auto it = ilist.begin(); it != ilist.end(); ++it) {
        Instruction ins = *it;
        auto *arith = dynamic_cast<ArithmeticInstruction *>(ins);
        if (!arith)
          continue;
        int opc = arith->GetOpcode();
        int k;
        // 仅处理 (reg op imm)
        if ((opc == MUL_OP || opc == DIV_OP || opc == MOD_OP) &&
            arith->GetOp1() && arith->GetOp2()) {
          Operand lhs = arith->GetOp1();
          Operand rhs = arith->GetOp2();
          Operand result = arith->GetResult();
          LLVMType ty = arith->GetType();
          if (dynamic_cast<RegOperand *>(lhs) && isImmI32(rhs, k) &&
              isPowerOfTwoInt(std::abs(k))) {
            int sh = log2_floor_u32((unsigned)std::abs(k));
            if (opc == MUL_OP) {
              *it = new ArithmeticInstruction(SHL, ty, lhs,
                                              new ImmI32Operand(sh), result);
              delete ins;
            } else if (opc == DIV_OP) {
              // 保守使用算术右移
              *it = new ArithmeticInstruction(ASHR, ty, lhs,
                                              new ImmI32Operand(sh), result);
              delete ins;
            } else if (opc == MOD_OP) {
              int mask = (1 << sh) - 1;
              *it = new ArithmeticInstruction(BITAND, ty, lhs,
                                              new ImmI32Operand(mask), result);
              delete ins;
            }
          }
        }
      }
    }
  }
}

// ============================================================================
// 高级优化占位/轻量实现，确保可链接
// ============================================================================

void SSAOptimizer::loopInvariantCodeMotion(LLVMIR &ir) {
  // TODO: 未来实现真正的LICM；此处暂不更改IR，仅作为占位避免链接错误
  (void)ir;
}

void SSAOptimizer::conditionalConstantPropagation(LLVMIR &ir) {
  // 简化版 CCP：
  // - 若 br i1 的条件为编译期常量，则改写为无条件分支
  // - 若 select 的条件为编译期常量，替换为恒定分支的值
  for (auto &func_pair : ir.function_block_map) {
    auto &blocks = func_pair.second;

    for (auto &bp : blocks) {
      auto &ilist = bp.second->Instruction_list;
      for (auto it = ilist.begin(); it != ilist.end();) {
        Instruction ins = *it;
        if (!ins) {
          ++it;
          continue;
        }

        if (auto *br = dynamic_cast<BrCondInstruction *>(ins)) {
          Operand c = br->GetCond();
          bool has_const = false;
          bool truth = false;
          if (auto *imm = dynamic_cast<ImmI32Operand *>(c)) {
            has_const = true;
            truth = (imm->GetIntImmVal() != 0);
          }
          if (has_const) {
            // 用无条件分支替换
            Operand target = truth ? br->GetTrueLabel() : br->GetFalseLabel();
            Instruction newbr = new BrUncondInstruction(target);
            *it = newbr;
            delete ins;
            ++it;
            continue;
          }
        } else if (auto *sel = dynamic_cast<SelectInstruction *>(ins)) {
          Operand c = sel->cond;
          bool has_const = false;
          bool truth = false;
          if (auto *imm = dynamic_cast<ImmI32Operand *>(c)) {
            has_const = true;
            truth = (imm->GetIntImmVal() != 0);
          }
          if (has_const) {
            Operand chosen = truth ? sel->op1 : sel->op2;
            replaceWithIdentity(ins, chosen);
            // 当前指令已变为简单复制，保留位置
          }
          ++it;
          continue;
        }

        ++it;
      }
    }
  }
  // 后处理：不可达路径消除
  eliminateUnreachableCode(ir);
}

void SSAOptimizer::globalValueNumbering(LLVMIR &ir) {
  // 轻量版：复用CSE
  commonSubexpressionElimination(ir);
}

void SSAOptimizer::loopUnrolling(LLVMIR &ir) {
  // 保守版 2x 循环展开：
  // 仅处理如下形态：
  //  Header(H): phi iv = [init, Preheader(P)] [iv_next, Latch(L)] ; icmp iv,
  //  bound ; brcond L or Exit(E) Latch(L):  [pure body ops...] ; inc1: iv_next
  //  = iv + c ; br H
  // 要求：L 只有该回跳边，且 body 全为纯计算（不含
  // store/call/alloca/ret/branch/phi/load）。 展开：在 L 中插入 inc_tmp1: iv1 =
  // iv + c；克隆 body（将 iv 映射为 iv1，块内临时寄存器重命名）；
  //       再插入 inc_tmp2: iv2 = iv1 + c；将 H 的 phi 从 L 的来边改为 iv2。

  auto computeMaxReg = [&](const std::map<int, LLVMBlock> &blocks) {
    int mx = -1;
    for (const auto &bp : blocks) {
      const auto &ilist = bp.second->Instruction_list;
      for (auto ins : ilist) {
        if (!ins)
          continue;
        int r = getInstructionResultRegister(ins);
        if (r > mx)
          mx = r;
        auto ops = getInstructionOperands(ins);
        for (auto &op : ops) {
          if (isRegisterOperand(op)) {
            int v = getRegisterFromOperand(op);
            if (v > mx)
              mx = v;
          }
        }
      }
    }
    return mx;
  };

  auto cloneForUnroll = [&](Instruction ins, int iv_reg, Operand iv1_op,
                            std::unordered_map<int, Operand> &regMap,
                            int &nextReg) -> Instruction {
    if (!ins)
      return nullptr;
    int opc = ins->GetOpcode();

    auto mapOp = [&](Operand op) -> Operand {
      if (!op)
        return op;
      if (auto *r = dynamic_cast<RegOperand *>(op)) {
        int no = r->GetRegNo();
        if (no == iv_reg)
          return iv1_op; // 第二次迭代使用 iv1
        auto it = regMap.find(no);
        if (it != regMap.end())
          return it->second; // 使用已克隆的寄存器
        return op;           // 外部寄存器保持不变
      }
      return op;
    };

    auto mapRes = [&](Operand oldRes) -> Operand {
      if (!oldRes)
        return oldRes;
      if (auto *r = dynamic_cast<RegOperand *>(oldRes)) {
        int no = r->GetRegNo();
        auto it = regMap.find(no);
        if (it != regMap.end())
          return it->second;
        Operand nr = GetNewRegOperand(++nextReg);
        regMap[no] = nr;
        return nr;
      }
      return oldRes;
    };

    switch (opc) {
    case ADD:
    case SUB:
    case MUL_OP:
    case DIV_OP:
    case MOD_OP:
    case FADD:
    case FSUB:
    case FMUL:
    case FDIV:
    case BITAND:
    case BITOR:
    case BITXOR:
    case SHL:
    case ASHR:
    case LSHR: {
      auto *a = dynamic_cast<ArithmeticInstruction *>(ins);
      return new ArithmeticInstruction((LLVMIROpcode)opc, a->GetType(),
                                       mapOp(a->GetOp1()), mapOp(a->GetOp2()),
                                       mapRes(a->GetResult()));
    }
    case ICMP: {
      auto *i = dynamic_cast<IcmpInstruction *>(ins);
      return new IcmpInstruction(i->GetType(), mapOp(i->GetOp1()),
                                 mapOp(i->GetOp2()), i->GetCond(),
                                 mapRes(i->GetResult()));
    }
    case FCMP: {
      auto *f = dynamic_cast<FcmpInstruction *>(ins);
      return new FcmpInstruction(f->GetType(), mapOp(f->GetOp1()),
                                 mapOp(f->GetOp2()), f->GetCond(),
                                 mapRes(f->GetResult()));
    }
    case ZEXT: {
      auto *z = dynamic_cast<ZextInstruction *>(ins);
      return new ZextInstruction(z->to_type, mapRes(z->result), z->from_type,
                                 mapOp(z->value));
    }
    case SITOFP: {
      auto *si = dynamic_cast<SitofpInstruction *>(ins);
      return new SitofpInstruction(mapRes(si->result), mapOp(si->value));
    }
    case FPTOSI: {
      auto *fp = dynamic_cast<FptosiInstruction *>(ins);
      return new FptosiInstruction(mapRes(fp->result), mapOp(fp->value));
    }
    case FPEXT: {
      auto *fe = dynamic_cast<FpextInstruction *>(ins);
      return new FpextInstruction(mapRes(fe->result), mapOp(fe->value));
    }
    case BITCAST: {
      auto *bc = dynamic_cast<BitCastInstruction *>(ins);
      return new BitCastInstruction(mapOp(bc->src), mapRes(bc->dst),
                                    bc->src_type, bc->dst_type);
    }
    case SELECT: {
      auto *s = dynamic_cast<SelectInstruction *>(ins);
      return new SelectInstruction(s->GetType(), mapOp(s->op1), mapOp(s->op2),
                                   mapOp(s->cond), mapRes(s->result));
    }
    case GETELEMENTPTR: {
      auto *g = dynamic_cast<GetElementptrInstruction *>(ins);
      std::vector<Operand> idx;
      idx.reserve(g->GetIndexes().size());
      for (auto &o : g->GetIndexes())
        idx.push_back(mapOp(o));
      return new GetElementptrInstruction(g->GetType(), mapRes(g->GetResult()),
                                          mapOp(g->GetPtrVal()), g->GetDims(),
                                          idx);
    }
    default:
      return nullptr; // 其他类型不克隆
    }
  };

  for (auto &func_pair : ir.function_block_map) {
    FuncDefInstruction fdef = func_pair.first;
    auto &blocks = func_pair.second;
    if (blocks.empty())
      continue;

    int maxReg = computeMaxReg(blocks);
    int nextReg = maxReg;

    bool changed_any = false;

    // 遍历所有可能的 Header
    for (auto &hb : blocks) {
      int hid = hb.first;
      LLVMBlock hblk = hb.second;
      if (hblk->Instruction_list.empty())
        continue;

      Instruction termH = hblk->Instruction_list.back();
      auto *brH = dynamic_cast<BrCondInstruction *>(termH);
      if (!brH)
        continue; // 仅考虑条件 Header

      int tId = -1, fId = -1;
      if (auto *t = dynamic_cast<LabelOperand *>(brH->GetTrueLabel()))
        tId = t->GetLabelNo();
      if (auto *f = dynamic_cast<LabelOperand *>(brH->GetFalseLabel()))
        fId = f->GetLabelNo();
      if (tId == -1 || fId == -1)
        continue;

      // 选择 Latch：其末尾必须无条件跳回 Header
      auto latch_it = blocks.find(tId);
      if (latch_it == blocks.end())
        continue;
      auto exit_it = blocks.find(fId);
      if (exit_it == blocks.end())
        continue;

      int latchId = -1, exitId = -1;
      auto endsWithBrTo = [&](int bid, int target) -> bool {
        auto itb = blocks.find(bid);
        if (itb == blocks.end())
          return false;
        auto &ilist = itb->second->Instruction_list;
        if (ilist.empty())
          return false;
        if (auto *bu = dynamic_cast<BrUncondInstruction *>(ilist.back())) {
          if (auto *lab = dynamic_cast<LabelOperand *>(bu->GetLabel()))
            return lab->GetLabelNo() == target;
        }
        return false;
      };

      if (endsWithBrTo(tId, hid)) {
        latchId = tId;
        exitId = fId;
      } else if (endsWithBrTo(fId, hid)) {
        latchId = fId;
        exitId = tId;
      } else {
        continue; // 无回边
      }

      // 定位 Header 中的 PHI（带有来自 Latch 的入口）
      PhiInstruction *phi_iv = nullptr;
      int iv_reg = -1;
      Operand latch_in_val = nullptr;
      size_t phi_index = 0;
      for (size_t i = 0; i < hblk->Instruction_list.size(); ++i) {
        auto *phi = dynamic_cast<PhiInstruction *>(hblk->Instruction_list[i]);
        if (!phi)
          continue;
        for (auto &pr : phi->phi_list) {
          auto *lab = dynamic_cast<LabelOperand *>(pr.first);
          if (lab && lab->GetLabelNo() == latchId) {
            phi_iv = phi;
            iv_reg = getRegisterFromOperand(phi->GetResult());
            latch_in_val = pr.second;
            phi_index = i;
            break;
          }
        }
        if (phi_iv)
          break;
      }
      if (!phi_iv || iv_reg == -1 || !latch_in_val)
        continue;

      // latch 来边值必须在 Latch 块内由 iv + imm 形成
      int latch_val_reg = -1;
      if (auto *r = dynamic_cast<RegOperand *>(latch_in_val))
        latch_val_reg = r->GetRegNo();
      if (latch_val_reg == -1)
        continue;

      auto &l_ilist = blocks[latchId]->Instruction_list;
      if (l_ilist.size() < 2)
        continue;

      // 找到 inc1，并抽取步长
      ArithmeticInstruction *inc1 = nullptr;
      int step = 0;
      size_t inc_pos = 0;
      for (size_t i = 0; i + 1 < l_ilist.size(); ++i) { // 跳过末尾的 br
        auto *a = dynamic_cast<ArithmeticInstruction *>(l_ilist[i]);
        if (!a)
          continue;
        int res = getRegisterFromOperand(a->GetResult());
        if (res != latch_val_reg)
          continue;
        if (a->GetOpcode() != ADD)
          continue; // 只处理 iv + imm
        int k = 0;
        if (!isRegisterOperand(a->GetOp1()) ||
            getRegisterFromOperand(a->GetOp1()) != iv_reg)
          continue;
        if (!isImmI32(a->GetOp2(), k))
          continue;
        step = k;
        inc1 = a;
        inc_pos = i;
        break;
      }
      if (!inc1)
        continue;
      if (step <= 0)
        continue; // 仅处理正步长

      // Body 为 [0, inc_pos) 且必须纯净
      bool pure = true;
      for (size_t i = 0; i < inc_pos; ++i) {
        if (!isPureFunctionalInstruction(l_ilist[i])) {
          pure = false;
          break;
        }
      }
      if (!pure)
        continue;

      // 生成 iv1 = iv + step（新寄存器）
      Operand iv_op = GetNewRegOperand(iv_reg);
      Operand iv1_res = GetNewRegOperand(++nextReg);
      Instruction inc_tmp1 = new ArithmeticInstruction(
          ADD, inc1->GetType(), iv_op, new ImmI32Operand(step), iv1_res);

      // 查找 Header 的条件寄存器定义（应为 icmp iv ? bound）
      int cond_reg = -1; // brH 条件寄存器
      if (isRegisterOperand(brH->GetCond()))
        cond_reg = getRegisterFromOperand(brH->GetCond());
      IcmpInstruction *icmpH = nullptr;
      for (auto &insH : hblk->Instruction_list) {
        if (getInstructionResultRegister(insH) == cond_reg) {
          icmpH = dynamic_cast<IcmpInstruction *>(insH);
          break;
        }
      }
      if (!icmpH)
        continue;
      // 仅支持 op1 为 iv 的 icmp
      if (!isRegisterOperand(icmpH->GetOp1()) ||
          getRegisterFromOperand(icmpH->GetOp1()) != iv_reg)
        continue;
      Operand boundOp = icmpH->GetOp2();
      // 保守：若 bound 是寄存器，不能在 Latch 内被定义
      if (isRegisterOperand(boundOp)) {
        int breg = getRegisterFromOperand(boundOp);
        bool defined_in_latch = false;
        for (auto &li : l_ilist) {
          if (getInstructionResultRegister(li) == breg) {
            defined_in_latch = true;
            break;
          }
        }
        if (defined_in_latch)
          continue;
      }

      // 克隆 body：将 iv 映射为 iv1，块内临时寄存器重命名
      std::unordered_map<int, Operand> regMap; // old -> new（仅 body 内）
      std::vector<Instruction> clones;
      clones.reserve(inc_pos);
      for (size_t i = 0; i < inc_pos; ++i) {
        Instruction oi = l_ilist[i];
        Instruction ni = cloneForUnroll(oi, iv_reg, iv1_res, regMap, nextReg);
        if (!ni) {
          pure = false;
          break;
        }
        clones.push_back(ni);
      }
      if (!pure)
        continue;

      // 在 inc1 位置插入 inc_tmp1
      auto insert_pos = l_ilist.begin() + static_cast<std::ptrdiff_t>(inc_pos);
      l_ilist.insert(insert_pos, inc_tmp1);

      // 生成 cond2: icmp (iv1 ? bound)
      Operand cond2_res = GetNewRegOperand(++nextReg);
      Instruction cond2 = new IcmpInstruction(
          icmpH->GetType(), iv1_res, boundOp, icmpH->GetCond(), cond2_res);
      // 在 inc1 原位置（被 inc_tmp1 占据后为 inc_pos+1）插入 cond2
      insert_pos = l_ilist.begin() + static_cast<std::ptrdiff_t>(inc_pos + 1);
      l_ilist.insert(insert_pos, cond2);

      // 创建新的块，用于第二次迭代的 body 与 iv2 更新
      int maxLabel = -1;
      for (auto &bp2 : blocks)
        maxLabel = std::max(maxLabel, bp2.first);
      int newLabel = maxLabel + 1;
      LLVMBlock newBlk = new BasicBlock(newLabel);
      // 填充新块：clones + iv2 + br header
      for (auto ci : clones)
        newBlk->Instruction_list.push_back(ci);
      Operand iv2_res = GetNewRegOperand(++nextReg);
      Instruction inc_tmp2 = new ArithmeticInstruction(
          ADD, inc1->GetType(), iv1_res, new ImmI32Operand(step), iv2_res);
      newBlk->Instruction_list.push_back(inc_tmp2);
      newBlk->Instruction_list.push_back(
          new BrUncondInstruction(GetNewLabelOperand(hid)));
      blocks[newLabel] = newBlk;

      // 修改 Header 所有 PHI：将来自 Latch 的入口改为来自 newBlk；
      // 值映射：latch_val_reg 或 iv_reg -> iv2_res；其他若在 body 定义，则用
      // regMap 映射；否则保持不变。
      for (auto &insH : hblk->Instruction_list) {
        auto *phiH = dynamic_cast<PhiInstruction *>(insH);
        if (!phiH)
          break;
        for (auto &pr : phiH->phi_list) {
          auto *lab = dynamic_cast<LabelOperand *>(pr.first);
          if (!(lab && lab->GetLabelNo() == latchId))
            continue;
          pr.first = GetNewLabelOperand(newLabel);
          Operand ov = pr.second;
          if (auto *rr = dynamic_cast<RegOperand *>(ov)) {
            int rno = rr->GetRegNo();
            if (rno == latch_val_reg || rno == iv_reg) {
              pr.second = iv2_res;
            } else {
              auto itrm = regMap.find(rno);
              if (itrm != regMap.end())
                pr.second = itrm->second;
            }
          }
        }
      }

      // 替换 Latch 的末尾无条件跳为条件跳：基于 cond2
      Operand labNew = GetNewLabelOperand(newLabel);
      Operand labExit = GetNewLabelOperand(exitId);
      // 移除 l_ilist 末尾原 br
      if (!l_ilist.empty() &&
          dynamic_cast<BrUncondInstruction *>(l_ilist.back())) {
        delete l_ilist.back();
        l_ilist.pop_back();
      }
      // 同时删除原 inc1（其结果已不再使用）
      for (size_t i = 0; i < l_ilist.size(); ++i) {
        if (l_ilist[i] == inc1) {
          delete l_ilist[i];
          l_ilist.erase(l_ilist.begin() + static_cast<std::ptrdiff_t>(i));
          break;
        }
      }
      // 追加新 brcond：true -> newBlk（二次迭代），false -> exit
      l_ilist.push_back(new BrCondInstruction(cond2_res, labNew, labExit));

      // 更新函数最大寄存器
      if (fdef && !fdef->Func_name.empty()) {
        int usedMax = nextReg;
        auto itmax = function_name_to_maxreg.find(fdef->Func_name);
        if (itmax != function_name_to_maxreg.end())
          itmax->second = std::max(itmax->second, usedMax);
      }

      changed_any = true;
      // 一个 Header 仅展开一次，继续寻找下一个 Header
    }

    if (changed_any) {
      // 展开后，做一次不可达清理以去除可能的死代码
      eliminateUnreachableCode(ir);
    }
  }
}

void SSAOptimizer::functionInlining(LLVMIR &ir) {
  // 仅对单基本块、无内存/分支/调用的纯计算小函数进行内联
  // 安全约束：callee 只有1个基本块，除最后一条为 ret 外，不包含
  // BR/PHI/LOAD/STORE/CALL/ALLOCA

  // 辅助：计算函数内出现的最大寄存器号
  auto computeMaxReg = [&](const std::map<int, LLVMBlock> &blocks) {
    int mx = -1;
    for (const auto &bp : blocks) {
      const auto &ilist = bp.second->Instruction_list;
      for (auto ins : ilist) {
        if (!ins)
          continue;
        // 结果寄存器
        int r = getInstructionResultRegister(ins);
        if (r > mx)
          mx = r;
        // 使用寄存器
        auto ops = getInstructionOperands(ins);
        for (auto &op : ops) {
          if (isRegisterOperand(op)) {
            int v = getRegisterFromOperand(op);
            if (v > mx)
              mx = v;
          }
        }
      }
    }
    return mx;
  };

  // 辅助：克隆一条无副作用指令（不含load/store/call/phi/branch/alloca/ret）
  auto clonePureInst =
      [&](Instruction ins, const std::unordered_map<int, Operand> &argMap,
          int regOffset,
          std::unordered_map<int, Operand> &regMap) -> Instruction {
    if (!ins)
      return nullptr;
    int opc = ins->GetOpcode();
    auto mapOp = [&](Operand op) -> Operand {
      if (!op)
        return op;
      if (auto *r = dynamic_cast<RegOperand *>(op)) {
        int no = r->GetRegNo();
        auto it = argMap.find(no);
        if (it != argMap.end())
          return it->second; // 形参→实参
        auto it2 = regMap.find(no);
        if (it2 != regMap.end())
          return it2->second;
        Operand nr = GetNewRegOperand(no + regOffset);
        regMap[no] = nr;
        return nr;
      }
      // 其他操作数直接复用
      return op;
    };

    // 结果寄存器映射
    auto mapResult = [&](Operand oldRes) -> Operand {
      if (!oldRes)
        return oldRes;
      if (auto *r = dynamic_cast<RegOperand *>(oldRes)) {
        int no = r->GetRegNo();
        // 形参寄存器不应作为定义目标出现
        auto it = regMap.find(no);
        if (it != regMap.end())
          return it->second;
        Operand nr = GetNewRegOperand(no + regOffset);
        regMap[no] = nr;
        return nr;
      }
      return oldRes;
    };

    switch (opc) {
    case ADD:
    case SUB:
    case MUL_OP:
    case DIV_OP:
    case MOD_OP:
    case FADD:
    case FSUB:
    case FMUL:
    case FDIV:
    case BITAND:
    case BITOR:
    case BITXOR:
    case SHL:
    case ASHR:
    case LSHR: {
      auto *a = dynamic_cast<ArithmeticInstruction *>(ins);
      return new ArithmeticInstruction((LLVMIROpcode)opc, a->GetType(),
                                       mapOp(a->GetOp1()), mapOp(a->GetOp2()),
                                       mapResult(a->GetResult()));
    }
    case ICMP: {
      auto *i = dynamic_cast<IcmpInstruction *>(ins);
      return new IcmpInstruction(i->GetType(), mapOp(i->GetOp1()),
                                 mapOp(i->GetOp2()), i->GetCond(),
                                 mapResult(i->GetResult()));
    }
    case FCMP: {
      auto *f = dynamic_cast<FcmpInstruction *>(ins);
      return new FcmpInstruction(f->GetType(), mapOp(f->GetOp1()),
                                 mapOp(f->GetOp2()), f->GetCond(),
                                 mapResult(f->GetResult()));
    }
    case ZEXT: {
      auto *z = dynamic_cast<ZextInstruction *>(ins);
      Operand res = mapResult(z->result);
      Operand val = mapOp(z->value);
      return new ZextInstruction(z->to_type, res, z->from_type, val);
    }
    case SITOFP: {
      auto *si = dynamic_cast<SitofpInstruction *>(ins);
      return new SitofpInstruction(mapResult(si->result), mapOp(si->value));
    }
    case FPTOSI: {
      auto *fp = dynamic_cast<FptosiInstruction *>(ins);
      return new FptosiInstruction(mapResult(fp->result), mapOp(fp->value));
    }
    case FPEXT: {
      auto *fe = dynamic_cast<FpextInstruction *>(ins);
      return new FpextInstruction(mapResult(fe->result), mapOp(fe->value));
    }
    case BITCAST: {
      auto *bc = dynamic_cast<BitCastInstruction *>(ins);
      return new BitCastInstruction(mapOp(bc->src), mapResult(bc->dst),
                                    bc->src_type, bc->dst_type);
    }
    case SELECT: {
      auto *s = dynamic_cast<SelectInstruction *>(ins);
      return new SelectInstruction(s->GetType(), mapOp(s->op1), mapOp(s->op2),
                                   mapOp(s->cond), mapResult(s->result));
    }
    case GETELEMENTPTR: {
      auto *g = dynamic_cast<GetElementptrInstruction *>(ins);
      std::vector<Operand> idx;
      idx.reserve(g->GetIndexes().size());
      for (auto &o : g->GetIndexes())
        idx.push_back(mapOp(o));
      return new GetElementptrInstruction(
          g->GetType(), mapResult(g->GetResult()), mapOp(g->GetPtrVal()),
          g->GetDims(), idx);
    }
    default:
      return nullptr; // 不克隆其他类型
    }
  };

  // 遍历所有调用点
  for (auto &func_pair : ir.function_block_map) {
    FuncDefInstruction caller_def = func_pair.first;
    std::string caller_name = caller_def->Func_name;
    auto &caller_blocks = func_pair.second;

    // 计算 caller 当前最大寄存器
    int caller_max = computeMaxReg(caller_blocks);

    for (auto &bp : caller_blocks) {
      auto &ilist = bp.second->Instruction_list;
      for (auto it = ilist.begin(); it != ilist.end(); /*in body*/) {
        Instruction ins = *it;
        auto *call = dynamic_cast<CallInstruction *>(ins);
        if (!call) {
          ++it;
          continue;
        }

        // 查找 callee 定义
        FuncDefInstruction callee_def = nullptr;
        std::map<int, LLVMBlock> *callee_blocks_ptr = nullptr;
        for (auto &fp2 : ir.function_block_map) {
          if (fp2.first->Func_name == call->GetFuncName()) {
            callee_def = fp2.first;
            callee_blocks_ptr = &fp2.second;
            break;
          }
        }
        if (!callee_def || !callee_blocks_ptr) {
          ++it;
          continue;
        }
        if (callee_def->Func_name == caller_name) {
          ++it;
          continue;
        } // 避免自内联

        auto &callee_blocks = *callee_blocks_ptr;
        if (callee_blocks.size() != 1) {
          ++it;
          continue;
        }
        LLVMBlock callee_block = callee_blocks.begin()->second;
        if (callee_block->Instruction_list.empty()) {
          ++it;
          continue;
        }

        // 校验 callee 只含纯计算 + 末尾ret
        bool ok = true;
        size_t n = callee_block->Instruction_list.size();
        for (size_t i = 0; i < n; ++i) {
          Instruction ci = callee_block->Instruction_list[i];
          int opc = ci->GetOpcode();
          if (i == n - 1) {
            if (opc != RET) {
              ok = false;
              break;
            }
          } else {
            if (!isPureFunctionalInstruction(ci)) {
              ok = false;
              break;
            }
          }
        }
        if (!ok) {
          ++it;
          continue;
        }

        // 形参与实参映射
        if (callee_def->formals.size() != call->args.size()) {
          ++it;
          continue;
        }
        std::unordered_map<int, Operand>
            argMap; // callee param reg_no -> caller operand
        for (size_t ai = 0; ai < call->args.size(); ++ai) {
          Operand formal_reg = callee_def->formals_reg[ai];
          int preg = dynamic_cast<RegOperand *>(formal_reg)->GetRegNo();
          Operand actual = call->args[ai].second; // 已是 caller 上下文
          argMap[preg] = actual;
        }

        // 计算 callee 使用的最大寄存器
        int callee_max = computeMaxReg(callee_blocks);
        int regOffset = std::max(0, caller_max + 1);

        // 克隆 callee 指令到 caller（在 call 位置之前插入），跳过最后的 ret
        std::unordered_map<int, Operand>
            regMap; // callee reg -> mapped caller reg
        std::vector<Instruction> clones;
        clones.reserve(callee_block->Instruction_list.size());
        for (size_t i = 0; i + 1 < callee_block->Instruction_list.size(); ++i) {
          Instruction ci = callee_block->Instruction_list[i];
          Instruction ni = clonePureInst(ci, argMap, regOffset, regMap);
          if (!ni) {
            ok = false;
            break;
          }
          clones.push_back(ni);
        }
        if (!ok) {
          ++it;
          continue;
        }

        // 处理 ret，将返回值赋给 call 的结果（如果有）
        Instruction retIns = callee_block->Instruction_list.back();
        Operand retVal = dynamic_cast<RetInstruction *>(retIns)->GetRetVal();
        Operand mappedRet = nullptr;
        if (retVal) {
          // map retVal
          if (auto *rr = dynamic_cast<RegOperand *>(retVal)) {
            int rno = rr->GetRegNo();
            auto itp = argMap.find(rno);
            if (itp != argMap.end())
              mappedRet = itp->second;
            else {
              auto itm = regMap.find(rno);
              if (itm != regMap.end())
                mappedRet = itm->second;
              else
                mappedRet = GetNewRegOperand(rno + regOffset);
            }
          } else {
            mappedRet = retVal; // 立即数等
          }
        }

        // 保存原 call 位置
        auto call_it = it;

        // 在 call 前插入克隆指令
        ilist.insert(call_it, clones.begin(), clones.end());

        // 若 call 有返回结果，则写回（整型用 ADD，浮点用 FADD）
        if (call->GetResult() && mappedRet) {
          Operand dst = call->GetResult();
          Instruction copyIns = nullptr;
          if (call->GetType() == I32) {
            copyIns = new ArithmeticInstruction(ADD, I32, mappedRet,
                                                new ImmI32Operand(0), dst);
          } else if (call->GetType() == FLOAT32) {
            copyIns = new ArithmeticInstruction(FADD, FLOAT32, mappedRet,
                                                new ImmF32Operand(0.0f), dst);
          } else {
            // 其他类型暂不处理
          }
          if (copyIns)
            ilist.insert(call_it, copyIns);
        }

        // 更新 caller 最大寄存器
        caller_max = std::max(caller_max, regOffset + callee_max);
        if (!caller_name.empty()) {
          auto itmax = function_name_to_maxreg.find(caller_name);
          if (itmax != function_name_to_maxreg.end())
            itmax->second = std::max(itmax->second, caller_max);
        }

        // 删除原始 call，并把迭代器移动到删除后的位置
        delete *call_it;
        it = ilist.erase(call_it);
      }
    }
  }
}
