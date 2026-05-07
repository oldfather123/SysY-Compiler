#include "../include/ssa.h"
#include <algorithm>
#include <iostream>

extern std::map<std::string, int> function_name_to_maxreg;

LLVMIR SSAOptimizer::optimize(const LLVMIR &ssa_ir) {
  LLVMIR optimized_ir = ssa_ir;

  std::cout << "Starting SSA optimizations..." << std::endl;
  // 除法优化
  // std::cout << "  Running division optimization..." << std::endl;
  // optimizeDivision(optimized_ir);
  // 删除多余add
  std::cout << "  Running redundant add removal..." << std::endl;
  removeRedundantAdd(optimized_ir);
  // 删除冗余的alloca和store
  std::cout << "  Running redundant param alloca removal..." << std::endl;
  removeRedundantParamAlloca(optimized_ir);

  // std::cout << "  Running redundant param load/store removal..." <<
  // std::endl; removeRedundantParamLoadStore(optimized_ir);
  // std::cout << "  Running redundant param load/store removal..." <<
  // std::endl;
  std::cout << "  Running redundant param load/store removal..." << std::endl;
  removeRedundantParamLoadStore(optimized_ir);

  // 延迟alloca
  std::cout << "  Running alloca delay optimization..." << std::endl;
  delayAllocaInstructions(optimized_ir);

  std::cout << "SSA optimizations completed";

  return optimized_ir;
}

bool SSAOptimizer::isPowerOfTwo(int n) { return n > 0 && (n & (n - 1)) == 0; }

int SSAOptimizer::log2_upper(int x) {
  int y = x;
  int count = 0;
  while (y != 0) {
    y = y >> 1;
    count += 1;
  }
  if ((1 << (count - 1)) == x) {
    return count - 1;
  } else {
    return count;
  }
}

std::tuple<long long, int, int> SSAOptimizer::choose_multiplier(int d,
                                                                int prec) {
  int l = log2_upper(d);
  int sh_post = l;
  const int N = 32;

  long long m_low = ((long long)1 << (N + l)) / d;
  long long m_high =
      (((long long)1 << (N + l)) + ((long long)1 << (N + l - prec))) / d;
  while ((m_low / 2) < (m_high / 2) && sh_post > 0) {
    m_low = m_low / 2;
    m_high = m_high / 2;
    sh_post -= 1;
  }
  return {m_high, sh_post, l};
}

void SSAOptimizer::optimizeDivision(LLVMIR &ir) {
  for (auto &func : ir.function_block_map) {
    std::string func_name = func.first->Func_name;
    int &max_reg = function_name_to_maxreg[func_name];
    max_reg++;
    for (auto &block : func.second) {
      auto &inst_list = block.second->Instruction_list;
      for (auto it = inst_list.begin(); it != inst_list.end();) {
        if (auto *arith = dynamic_cast<ArithmeticInstruction *>(*it)) {
          if (arith->GetOpcode() == DIV_OP) {
            Operand op1 = arith->GetOp1();
            Operand op2 = arith->GetOp2();

            // 情况 1: 两个操作数均为立即数
            if (auto *imm1 = dynamic_cast<ImmI32Operand *>(op1)) {
              if (auto *imm2 = dynamic_cast<ImmI32Operand *>(op2)) {
                int dividend = imm1->GetIntImmVal();
                int divisor = imm2->GetIntImmVal();
                if (divisor != 0) {
                  int result_val = dividend / divisor;
                  *it = new ArithmeticInstruction(
                      ADD, I32, new ImmI32Operand(result_val),
                      new ImmI32Operand(0), arith->GetResult());
                  ++it;
                  continue;
                }
              }
            }

            // 情况 2: 除数为立即数
            if (auto *imm2 = dynamic_cast<ImmI32Operand *>(op2)) {
              int d = imm2->GetIntImmVal();
              if (d == 0) {
                ++it;
                continue;
              }

              int abs_d = d >= 0 ? d : -d;
              Operand n = op1;
              Operand result = arith->GetResult();
              const int N = 32;

              if (abs_d == 1) {
                *it = new ArithmeticInstruction(ADD, I32, n,
                                                new ImmI32Operand(0), result);
                if (d < 0) {
                  it = inst_list.insert(
                      it + 1,
                      new ArithmeticInstruction(SUB, I32, new ImmI32Operand(0),
                                                result, result));
                }
                it = inst_list.erase(it);
                continue;
              }

              auto [m, sh_post, l] = choose_multiplier(abs_d, N - 1);
              std::vector<Instruction> new_insts;

              if (isPowerOfTwo(abs_d) && abs_d == (1 << l)) {
                Operand t1 = GetNewRegOperand(++max_reg);
                Operand t2 = GetNewRegOperand(++max_reg);
                new_insts.push_back(new ArithmeticInstruction(
                    ASHR, I32, n, new ImmI32Operand(l - 1), t1));
                new_insts.push_back(new ArithmeticInstruction(
                    LSHR, I32, t1, new ImmI32Operand(N - l), t2));
                Operand t3 = GetNewRegOperand(++max_reg);
                new_insts.push_back(
                    new ArithmeticInstruction(ADD, I32, n, t2, t3));
                new_insts.push_back(new ArithmeticInstruction(
                    ASHR, I32, t3, new ImmI32Operand(l), result));
              } else if (m < ((long long)1 << (N - 1))) {
                Operand t1 = GetNewRegOperand(++max_reg);
                Operand t2 = GetNewRegOperand(++max_reg);
                Operand t3 = GetNewRegOperand(++max_reg);
                Operand t4 = GetNewRegOperand(++max_reg);
                Operand t5 = GetNewRegOperand(++max_reg);

                new_insts.push_back(new ZextInstruction(I64, t1, I32, n));
                new_insts.push_back(new ZextInstruction(
                    I64, t2, I32, new ImmI32Operand((int)m)));
                new_insts.push_back(
                    new ArithmeticInstruction(MUL_OP, I64, t1, t2, t3));
                new_insts.push_back(new ArithmeticInstruction(
                    LSHR, I64, t3, new ImmI32Operand(32), t4));
                new_insts.push_back(new TruncInstruction(I64, t4, I32, t5));

                Operand t6 = GetNewRegOperand(++max_reg);
                new_insts.push_back(new ArithmeticInstruction(
                    ASHR, I32, t5, new ImmI32Operand(sh_post), t6));
                Operand t7 = GetNewRegOperand(++max_reg);
                new_insts.push_back(new ArithmeticInstruction(
                    ASHR, I32, n, new ImmI32Operand(N - 1), t7));
                new_insts.push_back(
                    new ArithmeticInstruction(SUB, I32, t6, t7, result));
              } else {
                long long m_adj = m - ((long long)1 << N);
                Operand t1 = GetNewRegOperand(++max_reg);
                Operand t2 = GetNewRegOperand(++max_reg);
                Operand t3 = GetNewRegOperand(++max_reg);
                Operand t4 = GetNewRegOperand(++max_reg);
                Operand t5 = GetNewRegOperand(++max_reg);
                Operand t6 = GetNewRegOperand(++max_reg);

                new_insts.push_back(new ZextInstruction(I64, t1, I32, n));
                new_insts.push_back(new ZextInstruction(
                    I64, t2, I32, new ImmI32Operand((int)m_adj)));
                new_insts.push_back(
                    new ArithmeticInstruction(MUL_OP, I64, t1, t2, t3));
                new_insts.push_back(new ArithmeticInstruction(
                    LSHR, I64, t3, new ImmI32Operand(32), t4));
                new_insts.push_back(new TruncInstruction(I64, t4, I32, t5));
                new_insts.push_back(
                    new ArithmeticInstruction(ADD, I32, n, t5, t6));

                Operand t7 = GetNewRegOperand(++max_reg);
                new_insts.push_back(new ArithmeticInstruction(
                    ASHR, I32, t6, new ImmI32Operand(sh_post), t7));
                Operand t8 = GetNewRegOperand(++max_reg);
                new_insts.push_back(
                    new ArithmeticInstruction(SUB, I32, t7, t8, result));
              }

              if (d < 0) {
                new_insts.push_back(new ArithmeticInstruction(
                    SUB, I32, new ImmI32Operand(0), result, result));
              }

              it = inst_list.erase(it);
              it = inst_list.insert(it, new_insts.begin(), new_insts.end());
              it += new_insts.size();
              continue;
            }

            // 情况 3: 被除数为立即数，除数为寄存器
            if (auto *imm1 = dynamic_cast<ImmI32Operand *>(op1)) {
              int dividend = imm1->GetIntImmVal();
              if (dividend == 0) {
                *it = new ArithmeticInstruction(ADD, I32, new ImmI32Operand(0),
                                                new ImmI32Operand(0),
                                                arith->GetResult());
                ++it;
                continue;
              }
            }
          }
        }
        ++it;
      }
      function_name_to_maxreg[func_name] = max_reg;
    }
  }
}

// Check if an operand is a specific register
bool SSAOptimizer::isRegAndEqual(Operand op, int reg_no) {
  if (auto *reg = dynamic_cast<RegOperand *>(op)) {
    return reg->GetRegNo() == reg_no;
  }
  return false;
}

// Check if an instruction uses a specific register
bool SSAOptimizer::isUsedInInstruction(Instruction ins, int reg_no) {
  if (auto *arith = dynamic_cast<ArithmeticInstruction *>(ins)) {
    if (isRegAndEqual(arith->GetOp1(), reg_no) ||
        isRegAndEqual(arith->GetOp2(), reg_no)) {
      return true;
    }
    if (arith->GetOp3() && isRegAndEqual(arith->GetOp3(), reg_no)) {
      return true;
    }
  } else if (auto *load = dynamic_cast<LoadInstruction *>(ins)) {
    if (isRegAndEqual(load->GetPointer(), reg_no)) {
      return true;
    }
  } else if (auto *store = dynamic_cast<StoreInstruction *>(ins)) {
    if (isRegAndEqual(store->GetValue(), reg_no) ||
        isRegAndEqual(store->GetPointer(), reg_no)) {
      return true;
    }
  } else if (auto *icmp = dynamic_cast<IcmpInstruction *>(ins)) {
    if (isRegAndEqual(icmp->GetOp1(), reg_no) ||
        isRegAndEqual(icmp->GetOp2(), reg_no)) {
      return true;
    }
  } else if (auto *fcmp = dynamic_cast<FcmpInstruction *>(ins)) {
    if (isRegAndEqual(fcmp->GetOp1(), reg_no) ||
        isRegAndEqual(fcmp->GetOp2(), reg_no)) {
      return true;
    }
  } else if (auto *phi = dynamic_cast<PhiInstruction *>(ins)) {
    for (const auto &pair : phi->phi_list) {
      if (isRegAndEqual(pair.second, reg_no)) {
        return true;
      }
    }
  } else if (auto *call = dynamic_cast<CallInstruction *>(ins)) {
    for (const auto &arg : call->GetArgs()) {
      if (isRegAndEqual(arg.second, reg_no)) {
        return true;
      }
    }
  } else if (auto *br_cond = dynamic_cast<BrCondInstruction *>(ins)) {
    if (isRegAndEqual(br_cond->GetCond(), reg_no)) {
      return true;
    }
  } else if (auto *ret = dynamic_cast<RetInstruction *>(ins)) {
    if (ret->GetRetVal() && isRegAndEqual(ret->GetRetVal(), reg_no)) {
      return true;
    }
  } else if (auto *gep = dynamic_cast<GetElementptrInstruction *>(ins)) {
    if (isRegAndEqual(gep->GetPtrVal(), reg_no)) {
      return true;
    }
    for (const auto &idx : gep->GetIndexes()) {
      if (isRegAndEqual(idx, reg_no)) {
        return true;
      }
    }
  } else if (auto *zext = dynamic_cast<ZextInstruction *>(ins)) {
    if (isRegAndEqual(zext->value, reg_no)) {
      return true;
    }
  } else if (auto *sitofp = dynamic_cast<SitofpInstruction *>(ins)) {
    if (isRegAndEqual(sitofp->value, reg_no)) {
      return true;
    }
  } else if (auto *fptosi = dynamic_cast<FptosiInstruction *>(ins)) {
    if (isRegAndEqual(fptosi->value, reg_no)) {
      return true;
    }
  } else if (auto *fpext = dynamic_cast<FpextInstruction *>(ins)) {
    if (isRegAndEqual(fpext->value, reg_no)) {
      return true;
    }
  } else if (auto *bitcast = dynamic_cast<BitCastInstruction *>(ins)) {
    if (isRegAndEqual(bitcast->src, reg_no)) {
      return true;
    }
  } else if (auto *select = dynamic_cast<SelectInstruction *>(ins)) {
    if (isRegAndEqual(select->cond, reg_no) ||
        isRegAndEqual(select->op1, reg_no) ||
        isRegAndEqual(select->op2, reg_no)) {
      return true;
    }
  } else if (auto *trunc = dynamic_cast<TruncInstruction *>(ins)) {
    if (isRegAndEqual(trunc->value, reg_no)) {
      return true;
    }
  }
  return false;
}

// Remove redundant add operations
void SSAOptimizer::removeRedundantAdd(LLVMIR &ir) {
  for (auto &func : ir.function_block_map) {
    for (auto &block_pair : func.second) {
      auto &inst_list = block_pair.second->Instruction_list;
      for (auto it = inst_list.begin(); it != inst_list.end();) {
        if (auto *arith = dynamic_cast<ArithmeticInstruction *>(*it)) {
          if (arith->GetOpcode() == ADD &&
              dynamic_cast<ImmI32Operand *>(arith->GetOp1()) &&
              dynamic_cast<ImmI32Operand *>(arith->GetOp2())) {
            Operand result = arith->GetResult();
            if (auto *reg = dynamic_cast<RegOperand *>(result)) {
              int reg_no = reg->GetRegNo();
              bool is_used_after = false;
              for (auto check_it = it + 1; check_it != inst_list.end();
                   ++check_it) {
                if (isUsedInInstruction(*check_it, reg_no)) {
                  is_used_after = true;
                  break;
                }
              }
              if (!is_used_after) {
                delete *it; // Free memory
                it = inst_list.erase(
                    it); // Remove instruction and update iterator
                continue;
              }
            }
          }
        }
        ++it;
      }
    }
  }
}

// New: Remove redundant alloca and store instructions for function parameters
void SSAOptimizer::removeRedundantParamAlloca(LLVMIR &ir) {
  for (auto &func : ir.function_block_map) {
    auto &block_map = func.second;
    if (block_map.empty())
      continue;

    // Get the entry block
    auto entry_block_it = block_map.begin();
    auto &entry_block = entry_block_it->second;
    auto &inst_list = entry_block->Instruction_list;

    // Get parameter registers
    std::unordered_set<int> param_regs;
    for (auto &formal : func.first->formals_reg) {
      if (auto *reg = dynamic_cast<RegOperand *>(formal)) {
        param_regs.insert(reg->GetRegNo());
      }
    }

    // Maps and lists for optimization
    std::unordered_map<int, Instruction> allocas;
    std::unordered_map<int, int> remap;
    std::vector<Instruction> to_remove;

    // Process instructions in the entry block
    for (auto it = inst_list.begin(); it != inst_list.end(); ++it) {
      if (auto *alloca_ins = dynamic_cast<AllocaInstruction *>(*it)) {
        if (alloca_ins->GetType() == PTR) {
          // 修改点1: 使用动态转换获取寄存器号
          if (auto *result_reg =
                  dynamic_cast<RegOperand *>(alloca_ins->GetResult())) {
            int reg_no = result_reg->GetRegNo();
            allocas[reg_no] = *it;
          }
        }
      } else if (auto *store_ins = dynamic_cast<StoreInstruction *>(*it)) {
        if (store_ins->GetType() == PTR) {
          Operand value = store_ins->GetValue();
          Operand pointer = store_ins->GetPointer();
          // 修改点2: 使用动态转换获取寄存器号
          if (auto *reg_value = dynamic_cast<RegOperand *>(value)) {
            if (auto *reg_pointer = dynamic_cast<RegOperand *>(pointer)) {
              int rY = reg_value->GetRegNo();
              int rZ = reg_pointer->GetRegNo();
              if (allocas.count(rZ) && param_regs.count(rY)) {
                remap[rZ] = rY;
                to_remove.push_back(allocas[rZ]);
                to_remove.push_back(*it);
                allocas.erase(rZ);
              }
            }
          }
        }
      }
    }

    // Remove marked instructions
    std::deque<Instruction> new_inst_list;
    for (auto ins : inst_list) {
      if (std::find(to_remove.begin(), to_remove.end(), ins) ==
          to_remove.end()) {
        new_inst_list.push_back(ins);
      } else {
        delete ins; // Free memory
      }
    }
    inst_list = std::move(new_inst_list);

    // Replace operands in all blocks
    for (auto &block_pair : block_map) {
      auto &block_inst_list = block_pair.second->Instruction_list;
      for (auto &ins : block_inst_list) {
        replaceOperands(ins, remap);
      }
    }
  }
}

// Helper function: Replace operands in an instruction
void SSAOptimizer::replaceOperands(Instruction ins,
                                   const std::unordered_map<int, int> &remap) {
  if (auto *arith = dynamic_cast<ArithmeticInstruction *>(ins)) {
    arith->op1 = replaceOperand(arith->GetOp1(), remap);
    arith->op2 = replaceOperand(arith->GetOp2(), remap);
    if (arith->GetOp3())
      arith->op3 = replaceOperand(arith->GetOp3(), remap);
  } else if (auto *load = dynamic_cast<LoadInstruction *>(ins)) {
    load->pointer = replaceOperand(load->GetPointer(), remap);
  } else if (auto *store = dynamic_cast<StoreInstruction *>(ins)) {
    store->value = replaceOperand(store->GetValue(), remap);
    store->pointer = replaceOperand(store->GetPointer(), remap);
  } else if (auto *icmp = dynamic_cast<IcmpInstruction *>(ins)) {
    icmp->op1 = replaceOperand(icmp->GetOp1(), remap);
    icmp->op2 = replaceOperand(icmp->GetOp2(), remap);
  } else if (auto *fcmp = dynamic_cast<FcmpInstruction *>(ins)) {
    fcmp->op1 = replaceOperand(fcmp->GetOp1(), remap);
    fcmp->op2 = replaceOperand(fcmp->GetOp2(), remap);
  } else if (auto *phi = dynamic_cast<PhiInstruction *>(ins)) {
    for (auto &pair : phi->phi_list) {
      pair.second = replaceOperand(pair.second, remap);
    }
  } else if (auto *call = dynamic_cast<CallInstruction *>(ins)) {
    for (auto &arg : call->args) {
      arg.second = replaceOperand(arg.second, remap);
    }
  } else if (auto *br_cond = dynamic_cast<BrCondInstruction *>(ins)) {
    br_cond->cond = replaceOperand(br_cond->GetCond(), remap);
  } else if (auto *ret = dynamic_cast<RetInstruction *>(ins)) {
    if (ret->GetRetVal())
      ret->ret_val = replaceOperand(ret->GetRetVal(), remap);
  } else if (auto *gep = dynamic_cast<GetElementptrInstruction *>(ins)) {
    gep->ptrval = replaceOperand(gep->GetPtrVal(), remap);
    for (auto &idx : gep->indexes) {
      idx = replaceOperand(idx, remap);
    }
  } else if (auto *zext = dynamic_cast<ZextInstruction *>(ins)) {
    zext->value = replaceOperand(zext->value, remap);
  } else if (auto *sitofp = dynamic_cast<SitofpInstruction *>(ins)) {
    sitofp->value = replaceOperand(sitofp->value, remap);
  } else if (auto *fptosi = dynamic_cast<FptosiInstruction *>(ins)) {
    fptosi->value = replaceOperand(fptosi->value, remap);
  } else if (auto *fpext = dynamic_cast<FpextInstruction *>(ins)) {
    fpext->value = replaceOperand(fpext->value, remap);
  } else if (auto *bitcast = dynamic_cast<BitCastInstruction *>(ins)) {
    bitcast->src = replaceOperand(bitcast->src, remap);
  } else if (auto *select = dynamic_cast<SelectInstruction *>(ins)) {
    select->cond = replaceOperand(select->cond, remap);
    select->op1 = replaceOperand(select->op1, remap);
    select->op2 = replaceOperand(select->op2, remap);
  } else if (auto *trunc = dynamic_cast<TruncInstruction *>(ins)) {
    trunc->value = replaceOperand(trunc->value, remap);
  }
}

// Helper function: Replace a single operand
Operand
SSAOptimizer::replaceOperand(Operand op,
                             const std::unordered_map<int, int> &remap) {
  if (auto *reg = dynamic_cast<RegOperand *>(op)) {
    int reg_no = reg->GetRegNo();
    auto it = remap.find(reg_no);
    if (it != remap.end()) {
      return GetNewRegOperand(it->second);
    }
  }
  return op;
}

void SSAOptimizer::removeRedundantParamLoadStore(LLVMIR &ir) {
  for (auto &func : ir.function_block_map) {
    if (func.second.empty())
      continue;

    // Step 1: Get parameter registers
    std::unordered_set<int> param_regs;
    // Also map param register number -> its Operand for later replacement
    std::unordered_map<int, Operand> param_reg_to_operand;
    for (auto &formal : func.first->formals_reg) {
      if (auto *reg = dynamic_cast<RegOperand *>(formal)) {
        param_regs.insert(reg->GetRegNo());
        param_reg_to_operand[reg->GetRegNo()] = formal;
      }
    }

    // Step 2: Get entry block
    auto &entry_block = func.second.begin()->second;
    auto &inst_list = entry_block->Instruction_list;

    // Step 3: Collect allocas in entry block
    std::unordered_map<int, AllocaInstruction *>
        allocas; // reg_no -> alloca instruction
    for (auto ins : inst_list) {
      if (auto *alloca_ins = dynamic_cast<AllocaInstruction *>(ins)) {
        if (auto *result_reg =
                dynamic_cast<RegOperand *>(alloca_ins->GetResult())) {
          int reg_no = result_reg->GetRegNo();
          allocas[reg_no] = alloca_ins;
        }
      }
    }

    // Step 4: Collect initial stores (parameter to alloca)
    std::unordered_map<int, int>
        alloca_to_param; // alloca_reg_no -> param_reg_no
    std::unordered_map<int, StoreInstruction *>
        initial_stores; // alloca_reg_no -> store instruction
    for (auto ins : inst_list) {
      if (auto *store_ins = dynamic_cast<StoreInstruction *>(ins)) {
        if (auto *pointer_reg =
                dynamic_cast<RegOperand *>(store_ins->GetPointer())) {
          int pointer_reg_no = pointer_reg->GetRegNo();
          if (allocas.count(pointer_reg_no)) {
            if (auto *value_reg =
                    dynamic_cast<RegOperand *>(store_ins->GetValue())) {
              int value_reg_no = value_reg->GetRegNo();
              if (param_regs.count(value_reg_no)) {
                alloca_to_param[pointer_reg_no] = value_reg_no;
                initial_stores[pointer_reg_no] = store_ins;
              }
            }
          }
        }
      }
    }

    // Step 5: Identify candidate allocas (only one store)
    std::unordered_set<int> candidate_allocas; // alloca_reg_no
    for (const auto &pair : alloca_to_param) {
      int alloca_reg = pair.first;
      int store_count = 0;
      for (const auto &block_pair : func.second) {
        auto &block_inst_list = block_pair.second->Instruction_list;
        for (auto ins : block_inst_list) {
          if (auto *store_ins = dynamic_cast<StoreInstruction *>(ins)) {
            if (auto *pointer_reg =
                    dynamic_cast<RegOperand *>(store_ins->GetPointer())) {
              if (pointer_reg->GetRegNo() == alloca_reg) {
                store_count++;
              }
            }
          }
        }
      }
      if (store_count == 1) { // Only the initial store
        candidate_allocas.insert(alloca_reg);
      }
    }

    // Step 6: Map load results to parameters and collect instructions to remove
    std::unordered_map<int, Operand>
        replace_map; // load_result_reg_no -> param_operand
    std::vector<Instruction> to_remove;
    for (const auto &block_pair : func.second) {
      auto &block_inst_list = block_pair.second->Instruction_list;
      for (auto ins : block_inst_list) {
        if (auto *load_ins = dynamic_cast<LoadInstruction *>(ins)) {
          if (auto *pointer_reg =
                  dynamic_cast<RegOperand *>(load_ins->GetPointer())) {
            int pointer_reg_no = pointer_reg->GetRegNo();
            if (candidate_allocas.count(pointer_reg_no)) {
              int param_reg_no = alloca_to_param[pointer_reg_no];
              // Use the captured mapping instead of indexing by register number
              auto it_param = param_reg_to_operand.find(param_reg_no);
              if (it_param == param_reg_to_operand.end()) {
                continue; // safety guard; unexpected, skip
              }
              Operand param_operand =
                  it_param->second; // Direct use of parameter operand
              if (auto *result_reg =
                      dynamic_cast<RegOperand *>(load_ins->GetResult())) {
                int result_reg_no = result_reg->GetRegNo();
                replace_map[result_reg_no] = param_operand;
                to_remove.push_back(load_ins);
              }
            }
          }
        }
      }
    }

    // Step 7: Add initial stores and allocas to removal list
    for (int alloca_reg : candidate_allocas) {
      if (initial_stores.count(alloca_reg)) {
        to_remove.push_back(initial_stores[alloca_reg]);
      }
      if (allocas.count(alloca_reg)) {
        to_remove.push_back(allocas[alloca_reg]);
      }
    }

    // Step 8: Replace operands in all instructions
    for (auto &block_pair : func.second) {
      auto &block_inst_list = block_pair.second->Instruction_list;
      for (auto &ins : block_inst_list) {
        if (std::find(to_remove.begin(), to_remove.end(), ins) ==
            to_remove.end()) {
          replaceOperands(ins, replace_map);
        }
      }
    }

    // Step 9: Remove marked instructions from all blocks
    for (auto &block_pair : func.second) {
      auto &block_inst_list = block_pair.second->Instruction_list;
      std::deque<Instruction> new_inst_list;
      for (auto ins : block_inst_list) {
        if (std::find(to_remove.begin(), to_remove.end(), ins) ==
            to_remove.end()) {
          new_inst_list.push_back(ins);
        } else {
          delete ins; // Free memory
        }
      }
      block_inst_list = std::move(new_inst_list);
    }
  }
}

// New helper function: Replace operands with Operand map
void SSAOptimizer::replaceOperands(
    Instruction ins, const std::unordered_map<int, Operand> &replace_map) {
  if (auto *arith = dynamic_cast<ArithmeticInstruction *>(ins)) {
    arith->op1 = replaceOperand(arith->GetOp1(), replace_map);
    arith->op2 = replaceOperand(arith->GetOp2(), replace_map);
    if (arith->GetOp3())
      arith->op3 = replaceOperand(arith->GetOp3(), replace_map);
  } else if (auto *load = dynamic_cast<LoadInstruction *>(ins)) {
    load->pointer = replaceOperand(load->GetPointer(), replace_map);
  } else if (auto *store = dynamic_cast<StoreInstruction *>(ins)) {
    store->value = replaceOperand(store->GetValue(), replace_map);
    store->pointer = replaceOperand(store->GetPointer(), replace_map);
  } else if (auto *icmp = dynamic_cast<IcmpInstruction *>(ins)) {
    icmp->op1 = replaceOperand(icmp->GetOp1(), replace_map);
    icmp->op2 = replaceOperand(icmp->GetOp2(), replace_map);
  } else if (auto *fcmp = dynamic_cast<FcmpInstruction *>(ins)) {
    fcmp->op1 = replaceOperand(fcmp->GetOp1(), replace_map);
    fcmp->op2 = replaceOperand(fcmp->GetOp2(), replace_map);
  } else if (auto *phi = dynamic_cast<PhiInstruction *>(ins)) {
    for (auto &pair : phi->phi_list) {
      pair.second = replaceOperand(pair.second, replace_map);
    }
  } else if (auto *call = dynamic_cast<CallInstruction *>(ins)) {
    for (auto &arg : call->args) {
      arg.second = replaceOperand(arg.second, replace_map);
    }
  } else if (auto *br_cond = dynamic_cast<BrCondInstruction *>(ins)) {
    br_cond->cond = replaceOperand(br_cond->GetCond(), replace_map);
  } else if (auto *ret = dynamic_cast<RetInstruction *>(ins)) {
    if (ret->GetRetVal())
      ret->ret_val = replaceOperand(ret->GetRetVal(), replace_map);
  } else if (auto *gep = dynamic_cast<GetElementptrInstruction *>(ins)) {
    gep->ptrval = replaceOperand(gep->GetPtrVal(), replace_map);
    for (auto &idx : gep->indexes) {
      idx = replaceOperand(idx, replace_map);
    }
  } else if (auto *zext = dynamic_cast<ZextInstruction *>(ins)) {
    zext->value = replaceOperand(zext->value, replace_map);
  } else if (auto *sitofp = dynamic_cast<SitofpInstruction *>(ins)) {
    sitofp->value = replaceOperand(sitofp->value, replace_map);
  } else if (auto *fptosi = dynamic_cast<FptosiInstruction *>(ins)) {
    fptosi->value = replaceOperand(fptosi->value, replace_map);
  } else if (auto *fpext = dynamic_cast<FpextInstruction *>(ins)) {
    fpext->value = replaceOperand(fpext->value, replace_map);
  } else if (auto *bitcast = dynamic_cast<BitCastInstruction *>(ins)) {
    bitcast->src = replaceOperand(bitcast->src, replace_map);
  } else if (auto *select = dynamic_cast<SelectInstruction *>(ins)) {
    select->cond = replaceOperand(select->cond, replace_map);
    select->op1 = replaceOperand(select->op1, replace_map);
    select->op2 = replaceOperand(select->op2, replace_map);
  } else if (auto *trunc = dynamic_cast<TruncInstruction *>(ins)) {
    trunc->value = replaceOperand(trunc->value, replace_map);
  }
}

// New helper function: Replace a single operand with Operand map
Operand SSAOptimizer::replaceOperand(
    Operand op, const std::unordered_map<int, Operand> &replace_map) {
  if (auto *reg = dynamic_cast<RegOperand *>(op)) {
    int reg_no = reg->GetRegNo();
    auto it = replace_map.find(reg_no);
    if (it != replace_map.end()) {
      return it->second; // Return the parameter operand directly
    }
  }
  return op;
}

void SSAOptimizer::delayAllocaInstructions(LLVMIR &ir) {
  for (auto &func : ir.function_block_map) {
    auto &block_map = func.second;
    if (block_map.empty())
      continue;

    // Get the entry block
    auto entry_block_it = block_map.begin();
    auto &entry_block = entry_block_it->second;
    auto &entry_inst_list = entry_block->Instruction_list;

    // Collect all alloca instructions from the entry block
    std::unordered_map<int, AllocaInstruction *> alloca_map;
    for (auto it = entry_inst_list.begin(); it != entry_inst_list.end();) {
      if (auto *alloca_ins = dynamic_cast<AllocaInstruction *>(*it)) {
        if (auto *result_reg =
                dynamic_cast<RegOperand *>(alloca_ins->GetResult())) {
          int reg_no = result_reg->GetRegNo();
          alloca_map[reg_no] = alloca_ins;
          it = entry_inst_list.erase(it);
          continue;
        }
      }
      ++it;
    }

    // Process each alloca instruction
    for (auto &pair : alloca_map) {
      int reg_no = pair.first;
      AllocaInstruction *alloca_ins = pair.second;
      bool placed = false;
      bool find = false;

      // Search all blocks for the first use
      for (auto block_it = block_map.begin();
           block_it != block_map.end() && !placed; ++block_it) {
        auto &block = block_it->second;
        auto &inst_list = block->Instruction_list;

        for (size_t i = 0; i < inst_list.size(); ++i) {
          if (isUsedInInstruction(inst_list[i], reg_no)) {
            find = true;
            // Found the first use, check block conditions
            bool is_last_block = (std::next(block_it) == block_map.end());
            Instruction last_ins = inst_list.back();
            bool ends_with_br_uncond =
                dynamic_cast<BrUncondInstruction *>(last_ins) != nullptr;
            bool jumps_to_next = false;

            if (ends_with_br_uncond) {
              auto *br_uncond = dynamic_cast<BrUncondInstruction *>(last_ins);
              if (br_uncond) {
                auto *label_op =
                    dynamic_cast<LabelOperand *>(br_uncond->GetLabel());
                if (label_op) {
                  int target_label = label_op->GetLabelNo();
                  auto next_block_it = std::next(block_it);
                  if (next_block_it != block_map.end() &&
                      next_block_it->first == target_label) {
                    jumps_to_next = true;
                  }
                }
              }
            }

            // If conditions are met, place alloca before the first use
            if (is_last_block || jumps_to_next) {
              inst_list.insert(inst_list.begin() + i, alloca_ins);
              placed = true;
            }
            break; // Stop after finding the first use in this block
          }
        }
        if (find) {
          break;
        }
      }

      // If no suitable location found, place back in entry block
      if (!placed) {
        entry_inst_list.push_front(alloca_ins);
      }
    }
  }
}
