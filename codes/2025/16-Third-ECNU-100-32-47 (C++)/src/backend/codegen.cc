#include "../include/backend.h"
#include "../include/global.h"
#include "../include/sym.h"
#include "riscv.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <variant>
#include <vector>

namespace aaac {

namespace backend {
static std::vector<RISCV_Reg> pushed_args;
static int int_counter = 0, float_counter = 0;
static std::mt19937 nop_rng(std::random_device{}());
static std::uniform_int_distribution<int> nop_dist(0, 1000000);

// forward declaration
static void emit_function(Function *func, std::ostream &ofs);
void emit_builtin(std::ostream &ofs);
static void emit_insn(std::shared_ptr<Insn> insn, const std::string &exit_label,
                      std::ostream &ofs, int frame_size);
static std::map<size_t, int> zip_init_list(const std::vector<int> &list) {
  std::map<size_t, int> result;
  auto cur=list.begin();
  while (1) {
    cur =
        std::find_if(cur, list.end(), [](int i) { return i != 0; });
    if(cur==list.end())
        break;
    size_t idx=std::distance(list.begin(), cur);
    result[idx] = *cur;
    ++cur;
  }
  return result;
}

static std::map<size_t, float> zip_init_list(const std::vector<float> &list) {
  std::map<size_t, float> result;
  auto cur=list.begin();
  while (1) {
    cur =
        std::find_if(cur, list.end(), [](float i) { return i != 0.0; });
    if(cur==list.end())
        break;
    size_t idx=std::distance(list.begin(), cur);
    result[idx] = *cur;
    ++cur;
  }
  return result;
}

static bool in_range(long long int imm) { return -2048 <= imm && imm <= 2047; }

static void emit_sd(RISCV_Reg r, int off, std::ostream &ofs) {
  if (in_range(off)) {
    ofs << "\tsd " << reg_to_string(r) << ", " << off << "(sp)\n";
  } else {
    ofs << "\tli t0, " << off << "\n";
    ofs << "\tadd t0, t0, sp\n";
    ofs << "\tsd " << reg_to_string(r) << ", 0(t0)\n";
  }
}

static void emit_ld(RISCV_Reg r, int off, std::ostream &ofs) {
  if (in_range(off)) {
    ofs << "\tld " << reg_to_string(r) << ", " << off << "(sp)\n";
  } else {
    ofs << "\tli t0, " << off << "\n";
    ofs << "\tadd t0, t0, sp\n";
    ofs << "\tld " << reg_to_string(r) << ", 0(t0)\n";
  }
}

static void emit_fsd(RISCV_Reg r, int off, std::ostream &ofs) {
  if (in_range(off)) {
    ofs << "\tfsd " << reg_to_string(r) << ", " << off << "(sp)\n";
  } else {
    ofs << "\tli t0, " << off << "\n";
    ofs << "\tadd t0, t0, sp\n";
    ofs << "\tfsd " << reg_to_string(r) << ", 0(t0)\n";
  }
}

static void emit_fld(RISCV_Reg r, int off, std::ostream &ofs) {
  if (in_range(off)) {
    ofs << "\tfld " << reg_to_string(r) << ", " << off << "(sp)\n";
  } else {
    ofs << "\tli t0, " << off << "\n";
    ofs << "\tadd t0, t0, sp\n";
    ofs << "\tfld " << reg_to_string(r) << ", 0(t0)\n";
  }
}
// int stack_offset(Function* func) {
//     int max_offset = 0;
//     for (const auto &var : func->var_list) {
//         auto &info = var->prop<sym::RegAllocInfo>();
//         if (info.stack_offset > 0)
//             max_offset = std::max(max_offset, info.stack_offset);
//     }
//     max_offset += 16; // ra/s0

//     return ((max_offset + 15) / 16) * 16; // align(16)
// }

int stack_offset(Function *func) {
  int gpr_cnt = 0;
  for (auto r : CALLEE_SAVED_GPRS)
    if (func->uses(r))
      ++gpr_cnt;
  int fpr_cnt = 0;
  for (auto r : CALLEE_SAVED_FPRS)
    if (func->uses(r))
      ++fpr_cnt;

  int frame_size = (2 + gpr_cnt + fpr_cnt) * 8; // ra, s0 plus callee-saved
  for (auto esc : func->getEscape()) {
    frame_size += esc->getType()->getTypeSize();
    esc->prop<sym::RegAllocInfo>().fp_offset = -frame_size;
    std::cout << "alloca " << esc->getName() << " " << *esc->getType() << " "
              << esc->getType()->getTypeSize() << " " << -frame_size << "\n";
  }
  frame_size += func->getStackSize();
  frame_size = (frame_size + 15) / 16 * 16; // 向上16字节对齐
  return frame_size;
}

static void emit_globals(std::ostream &ofs) {
  if (globals.global_vars.empty())
    return;
  std::string cur_sec;
  for (auto &v : globals.global_vars) {
    auto it = globals.g_registry.find(v->getName());
    if (it != globals.g_registry.end()) {
      if (it->second.get_type() == INT_LIST) {
        auto zipped_list = zip_init_list(it->second.get<std::vector<int>>());
        if (zipped_list.size() == 0) {
          cur_sec = ".bss";
          ofs << "\t.bss\n";
          ofs << "\t.globl " << v->getName() << "\n" << v->getName() << ":\n";
          ofs << "\t.zero " << (it->second.get<std::vector<int>>().size())*4 << "\n";
          continue;
        }
      } else if (it->second.get_type() == FLOAT_LIST) {
        auto zipped_list = zip_init_list(it->second.get<std::vector<float>>());
        if (zipped_list.size() == 0) {
          cur_sec = ".bss";
          ofs << "\t.bss\n";
          ofs << "\t.globl " << v->getName() << "\n" << v->getName() << ":\n";
          ofs << "\t.zero " << (it->second.get<std::vector<float>>().size())*4 << "\n";
          continue;
        }
      }
    }
    bool is_const = v->isConst();
    std::string sec = is_const ? ".rodata" : ".data";
    if (sec != cur_sec) {
      ofs << "\t" << sec << "\n";
      cur_sec = sec;
    }
    ofs << "\t.globl " << v->getName() << "\n" << v->getName() << ":\n";
    if (it != globals.g_registry.end()) {
      switch (it->second.get_type()) {
      case INT_T:
        ofs << "\t.long " << it->second.get<int>() << "\n";
        break;
      case INT_LIST:{
        auto zipped_list=zip_init_list(it->second.get<std::vector<int>>());
        size_t last_idx=0;
        for(auto [idx,val]:zipped_list){
            if(idx!=last_idx){
                ofs << "\t.zero " << (idx-last_idx)*4 << "\n";
            }
            ofs<<"\t.long "<<val<<"\n";
            last_idx=idx+1;
        }
        if(last_idx!=it->second.get<std::vector<int>>().size()){
            ofs << "\t.zero " << (it->second.get<std::vector<int>>().size()-last_idx)*4 << "\n";
        }
        break;}
      case FLOAT_T:{
          float f_val = it->second.get<float>();
          int i_val = 0;
          memcpy(&i_val,&f_val,sizeof(int));
          // ofs << "\t.float " << it->second.get<float>() << " # imm = " << i_val << "\n";
          ofs << "\t.word " << i_val << " # imm = " << f_val << "\n";
          break;
      }
      case FLOAT_LIST:{
        auto zipped_list=zip_init_list(it->second.get<std::vector<float>>());
        size_t last_idx=0;
        for(auto [idx,val]:zipped_list){
            if(idx!=last_idx){
                ofs << "\t.zero " << (idx-last_idx)*4 << "\n";
            }
            ofs<<"\t.long "<<val<<"\n";
            last_idx=idx+1;
        }
        if(last_idx!=it->second.get<std::vector<float>>().size()){
            ofs << "\t.zero " << (it->second.get<std::vector<float>>().size()-last_idx)*4 << "\n";
        }
        break;}
      }
    } else {
      ofs << "\t.zero " << v->getType()->getTypeSize() << "\n";
    }
  }
}

void codegen(std::ostream &ofs) {
  emit_globals(ofs);

  for (auto &func : globals.functions) {
      emit_function(func.get(), ofs);
  }
  emit_builtin(ofs);
}

#ifndef EXPORT_TO_FILE
void codegen() { codegen(std::cout); }
#else
void codegen() { codegen("out.s"); }
#endif

void codegen(const std::string &filename) {
  std::ofstream ofs(filename);
  if (!ofs.is_open()) {
    std::cerr << "Failed to open output file: " << filename << '\n';
    return;
  }
  codegen(ofs);
}

void emit_builtin(std::ostream &ofs) {
  ofs << R"(
    .text
    .align 2
    .globl __builtin_memset
    .type __builtin_memset, @function
__builtin_memset:
    mv t0, a0
    li t1, 0
.Lloop:
    beq t1, a2, .Lend
    add t2, t0, t1
    sb a1, 0(t2)
    addi t1, t1, 1
    j .Lloop
.Lend:
    ret
)";

  ofs << R"(
    .text
    .comm	_sysy_start,16,8
    .comm	_sysy_end,16,8
    .comm	_sysy_l1,4096,8
    .comm	_sysy_l2,4096,8
    .comm	_sysy_h,4096,8
    .comm	_sysy_m,4096,8
    .comm	_sysy_s,4096,8
    .comm	_sysy_us,4096,8
    .comm	_sysy_idx,4,4
)";
}

void emit_function(Function *func, std::ostream &ofs) {
  const std::string &name = func->name;
  int gpr_cnt = 0;
  for (auto r : CALLEE_SAVED_GPRS)
    if (func->uses(r))
      ++gpr_cnt;
  int fpr_cnt = 0;
  for (auto r : CALLEE_SAVED_FPRS)
    if (func->uses(r))
      ++fpr_cnt;

  int frame_size = stack_offset(func);
  auto in_range = [](int imm) -> bool { return -2048 <= imm && imm <= 2047; };
  if (name == "main") {
    ofs << "\t.text\n\t.align 2\n\t.globl " << name << "\n";
  } else {
    ofs << "\t.text\n\t.align 2\n" << name << ":\n";
  }
  ofs << "\t.type " << name << ", @function\n" << name << ":\n";
//   if (name == "main") {
//     ofs << "\tli t0, 0b001\n";
//     ofs << "\tfsrm t0\n";
//   }
  int off_ra = frame_size - 8;
  int off_s0 = frame_size - 16;
  int off_fs0 = off_s0 - gpr_cnt * 8;
  if (in_range(frame_size)) {
    ofs << "\taddi sp, sp, -" << frame_size << '\n';
  } else {
    ofs << "\tli t0, -" << frame_size << "\n";
    ofs << "\tadd sp, sp, t0\n";
  }

  emit_sd(RISCV_Reg::__ra, off_ra, ofs);
  emit_sd(RISCV_Reg::__s0, off_s0, ofs);

  int offset = off_s0;
  for (auto r : CALLEE_SAVED_GPRS) {
    if (!func->uses(r))
      continue;
    offset -= 8;
    emit_sd(r, offset, ofs);
  }

  offset = off_fs0;
  for (auto r : CALLEE_SAVED_FPRS) {
    if (!func->uses(r))
      continue;
    offset -= 8;
    emit_fsd(r, offset, ofs);
  }

  if (in_range(frame_size)) {
    ofs << "\taddi s0, sp, " << frame_size << '\n';
  } else {
    ofs << "\tli t0, " << frame_size << "\n";
    ofs << "\tadd s0, sp, t0\n";
  }
  std::string exit_label = ".Lret_" + name;

  for (auto bb = func->bbs; bb; bb = bb->getNext()) {
    auto insn_list = bb->getInsnList();
    for (auto &insn : insn_list) {
      emit_insn(insn, exit_label, ofs, frame_size);
    }
  }

  // auto dfs = [&](auto self, std::shared_ptr<BasicBlock> cur) -> void {
  //     if(cur == nullptr) return;
  //     for (auto &insn : cur->getInsnList()) {
  //         emit_insn(insn, exit_label, ofs, frame_size);
  //     }
  //     self(self,cur->getNext());
  //     self(self,cur->getThen());
  //     self(self,cur->getElse());
  // };
  // dfs(dfs,func->bbs);

  ofs << exit_label << ":\n";

  offset = off_fs0 - fpr_cnt * 8;
  for (auto it = CALLEE_SAVED_FPRS.rbegin(); it != CALLEE_SAVED_FPRS.rend(); ++it) {
    if (!func->uses(*it))
      continue;
    emit_fld(*it, offset, ofs);
    offset += 8;
  }

  offset = off_s0 - gpr_cnt * 8;
  for (auto it = CALLEE_SAVED_GPRS.rbegin(); it != CALLEE_SAVED_GPRS.rend(); ++it) {
    if (!func->uses(*it))
      continue;
    emit_ld(*it, offset, ofs);
    offset += 8;
  }

  emit_ld(RISCV_Reg::__s0, off_s0, ofs);
  emit_ld(RISCV_Reg::__ra, off_ra, ofs);

  if (in_range(frame_size)) {
    ofs << "\taddi sp, sp, " << frame_size << '\n';
  } else {
    ofs << "\tli t0, " << frame_size << "\n";
    ofs << "\tadd sp, sp, t0\n";
  }
  ofs << "\tret\n";
}

  static const char *get_reg(std::shared_ptr<sym::Var> var) {
    auto &info = var->prop<sym::RegAllocInfo>();
    return reg_to_string(index_to_reg(info.consumed_registers));
  }

  static std::string operand_to_str(frontend::Operand op) {
        if (std::holds_alternative<std::shared_ptr<sym::Var>>(op)) {
            auto v = std::get<std::shared_ptr<sym::Var>>(op);
            return get_reg(v);
        } else if (std::holds_alternative<int>(op)) {
            return std::to_string(std::get<int>(op));
        } else if (std::holds_alternative<float>(op)) {
            return std::to_string(std::get<float>(op));
        }
        return "";
    }

    // 这个函数将常量或寄存器搬运到临时浮点寄存器
    // 为了处理整数 (imm/reg) 和浮点数的混合运算而存在
    // TODO: fcvt.s.w 会带来不小的额外开销
    // 若op是整数或者浮点，则会用到t0寄存器
    static void load_to_ft(const frontend::Operand &op, const char *tmp, std::ostream &ofs) {
        if (std::holds_alternative<float>(op)) {
            float  v = std::get<float>(op);
            uint32_t bits;
            static_assert(sizeof(bits) == sizeof(v));
            std::memcpy(&bits, &v, sizeof(bits));
            ofs << "\tli        t0, " << bits << " # imm = " << v << "\n";
            ofs << "\tfmv.w.x   " << tmp << ", t0\n";
        } else if (std::holds_alternative<int>(op)) {
            int imm = std::get<int>(op);
            ofs << "\tli        t0, " << imm    << "\n";
            ofs << "\tfcvt.s.w  " << tmp << ", t0\n";
        } else {
            auto opr = operand_to_str(op);
            if (!opr.empty() && opr[0] == 'f') {
              ofs << "\tfmv.s     " << tmp << ", " << opr << "\n";
            } else {
              ofs << "\tfcvt.s.w  " << tmp << ", " << opr << "\n";
            }
        }
    }

  static bool operandIsFloat(const frontend::Operand &op) {
    if (std::holds_alternative<float>(op)) {
      return true;
    }
    if (auto pv = std::get_if<std::shared_ptr<sym::Var>>(&op)) {
      return (*pv)->getType() == frontend::TypeFactory::FloatTy;
    }
    return false;
  }

void emit_insn(std::shared_ptr<Insn> insn, const std::string &exit_label,
               std::ostream &ofs, int frame_size) {
  auto opcode = insn->getOpCode();

  auto common = std::dynamic_pointer_cast<CommonInsn>(insn);
  auto label = std::dynamic_pointer_cast<LabelledInsn>(insn);
  auto branch = std::dynamic_pointer_cast<BranchInsn>(insn);
  auto cst = std::dynamic_pointer_cast<ConstantLoadInsn>(insn);

        switch (opcode) {
        case OpCode::Alloca: {
            auto dest = get_reg(common->getDest());
            int size = std::get<int>(common->getSrc0());
            ofs << "\t# alloca " << dest << ", " << size << "\n";
            break; // handled by frame layout
        }
        // aaac 的 cast 处理本质上就是一条 mv 语句
        case OpCode::Int2Float:
        case OpCode::Float2Int:
        case OpCode::Assign: {
            auto src0 = common->getSrc0();
            auto destVar = common->getDestination();
            auto &info = destVar->prop<sym::RegAllocInfo>();
            RISCV_Reg destReg = index_to_reg(info.consumed_registers);
            bool destFloat = isFloatReg(destReg);
            // case 1: 整型立即数
            if (std::holds_alternative<int>(src0)) {
              int imm = std::get<int>(src0);
              if (!destFloat) {
                ofs << "\tli " << get_reg(destVar) << ", " << imm << " # imm = " << imm << "\n";
              } else {
                ofs << "\tli t0, " << imm << " # imm = " << imm << "\n";
                ofs << "\tfcvt.s.w " << get_reg(destVar) << ", t0\n";
              }
            // case 2: 浮点立即数
            } else if (std::holds_alternative<float>(src0)) {
              float vf = std::get<float>(src0);
              uint32_t bits;
              static_assert(sizeof(bits)==sizeof(vf));
              std::memcpy(&bits, &vf, sizeof(bits));
              // 整寄存器 + 浮常量（无）

              // 浮寄存器 + 浮常量
              ofs << "\tli t0, " << bits << " # imm = " << vf << "\n";
              ofs << "\tfmv.w.x " << get_reg(destVar) << ", t0\n";
            // case 3: 寄存器
            } else {
              auto srcVar  = std::get<std::shared_ptr<sym::Var>>(src0);
              auto &srcInfo = srcVar->prop<sym::RegAllocInfo>();
              RISCV_Reg srcReg = index_to_reg(srcInfo.consumed_registers);
              bool srcFloat = backend::isFloatReg(srcReg);

              const char* destName = get_reg(destVar);
              const char* srcName  = get_reg(srcVar);

              if (!srcFloat && !destFloat) {
                if (destName != srcName) {
                  ofs << "\tmv " << destName << ", " << srcName << "\n";
                }
              } else if (!srcFloat && destFloat) {
                ofs << "\tfcvt.s.w " << destName << ", " << srcName << "\n";
              } else if (srcFloat && !destFloat) {
                ofs << "\tfcvt.w.s " << destName << ", " << srcName << ", rtz\n";
              } else {
                ofs << "\tfmv.s " << destName << ", " << srcName << "\n";
              }
            }
            break;
        }
        case OpCode::LoadConstant: {
            if (cst) {
                ofs << "\tli " << get_reg(cst->getDestination()) << ", ";
                if (cst->isInt())
                    ofs << cst->getInt() << "\n";
                else
                    ofs << cst->getFloat() << "\n";
            }
            break;
        }
        case OpCode::LoadDataAddress: {
            ofs << "\tla " << get_reg(common->getDestination()) << ", .data+"
                << operand_to_str(common->getSrc0()) << "\n";
            break;
        }
        case OpCode::LoadLabelAddress: {
            std::string name = std::get<std::shared_ptr<sym::Var>>(common->getSrcPseudo())->getName();
            std::string reg = get_reg(common->getDestination());
            ofs << "\tla " << get_reg(common->getDestination()) << ", " << name << "\n";
            // char buf[512];
            // sprintf(buf,"\tlui %s, %%hi(%s)\n",reg.c_str(),name.c_str());
            // ofs << buf;
            // sprintf(buf,"\taddi %s, %s, %%lo(%s)\n",reg.c_str(),reg.c_str(),name.c_str());
            // ofs << buf;
            break;
        }
        case OpCode::LoadEscapeAddress: {
            auto var = std::get<std::shared_ptr<sym::Var>>(common->getSrcPseudo());
            int fp_offset = var->prop<sym::RegAllocInfo>().fp_offset;
            std::cout << "codegen " << var->getName() << " " << *var->getType() << " " << fp_offset << "\n";
            if (in_range(fp_offset)) {
                ofs << "\taddi " << get_reg(common->getDestination()) << ", s0, " << fp_offset << "\n";
            } else {
                ofs << "\tli t2, " << fp_offset << "\n";
                ofs << "\tadd " << get_reg(common->getDestination()) << ", s0, t2\n";
            }
            break;
        }
        case OpCode::AddressOf: {
            int off = std::get<int>(common->getSrc0()) - frame_size;
            ofs << "\taddi " << get_reg(common->getDestination()) << ", s0, "
                << off << "\n";
            break;
        }
        case OpCode::GlobalAddressOf: {
            ofs << "\taddi " << get_reg(common->getDestination()) << ", gp, "
                << operand_to_str(common->getSrc0()) << "\n";
            break;
        }
        case OpCode::Label: {
            ofs << label->getLabel() << ":\n";
            break;
        }
        case OpCode::Push: {
            auto srcOp = common->getSrc0();
            std::visit(
                [&](auto srcOp) {
                  using T = std::decay_t<decltype(srcOp)>;
                  if constexpr (std::is_same_v<T, std::shared_ptr<sym::Var>>) {
                    auto varPtr = srcOp;
                    // 从它的 RegAllocInfo 里读取分配到的寄存器索引
                    auto &info = varPtr->template prop<sym::RegAllocInfo>();
                    RISCV_Reg srcReg = index_to_reg(info.consumed_registers);
                    if (isFloatReg(srcReg)) {
                      int float_idx = float_counter++;
                      if (float_idx <= 7) {
                        RISCV_Reg dstReg = static_cast<RISCV_Reg>(
                            static_cast<int>(RISCV_Reg::__fa0) + float_idx);
                        ofs << "\tfmv.s " << reg_to_string(dstReg) << ", "
                            << reg_to_string(srcReg) << "\n";
                        return;
                      }
                    } else {
                      int int_idx = int_counter++;
                      if (int_idx <= 7) {
                        RISCV_Reg dstReg = static_cast<RISCV_Reg>(
                            static_cast<int>(RISCV_Reg::__a0) + int_idx);
                        ofs << "\tmv " << reg_to_string(dstReg) << ", "
                            << reg_to_string(srcReg) << "\n";
                        return;
                      }
                      // TODO 暂无spill
                    }
                    int spill_off = std::max(0, int_counter - 8);
                    spill_off += std::max(0, float_counter - 8);
                    spill_off *= -8;
                    if (spill_off >= -2048 && spill_off <= 2047) {
                      if (isFloatReg(srcReg)) {
                        ofs << "\tfsw " << reg_to_string(srcReg) << ", "
                            << spill_off << "(sp)\n";
                      } else {
                        ofs << "\tsd " << reg_to_string(srcReg) << ", "
                            << spill_off << "(sp)\n";
                      }
                    } else {
                      ofs << "\tli t0, " << spill_off << "\n";
                      ofs << "\tadd t0, sp, t0\n";
                      if (isFloatReg(srcReg)) {
                        ofs << "\tfsw " << reg_to_string(srcReg) << ", 0(t0)\n";
                      } else {
                        ofs << "\tsd " << reg_to_string(srcReg) << ", 0(t0)\n";
                      }
                    }
                  }else if constexpr(std::is_same_v<T, int>){
                      int int_idx = int_counter++;
                      if (int_idx <= 7) {
                        RISCV_Reg dstReg = static_cast<RISCV_Reg>(
                            static_cast<int>(RISCV_Reg::__a0) + int_idx);
                        ofs << "\tli " << reg_to_string(dstReg) << ", "
                            << srcOp << "\n";
                        return;
                      }
                      int spill_off = std::max(0, int_counter - 8);
                      spill_off += std::max(0, float_counter - 8);
                      spill_off *= -8;
                      if (spill_off >= -2048 && spill_off <= 2047) {
                        ofs << "\tli t0, " << srcOp << "\n";
                        ofs << "\tsd t0, " << spill_off << "(sp)\n";
                      } else {
                        ofs << "\tli t0, " << spill_off << "\n";
                        ofs << "\tadd t0, sp, t0\n";
                        ofs << "\tli t1, " << srcOp << "\n";
                        ofs << "\tsd t1, 0(t0)\n";
                      }
                  }else{
                      uint32_t bits;
                      static_assert(sizeof(bits)==sizeof(srcOp));
                      std::memcpy(&bits, &srcOp, sizeof(bits));
                      int float_idx = float_counter++;
                      if (float_idx <= 7) {
                        RISCV_Reg dstReg = static_cast<RISCV_Reg>(
                            static_cast<int>(RISCV_Reg::__fa0) + float_idx);
                        ofs << "\tli t0, "<< bits << " # imm = " << srcOp << "\n";
                        ofs << "\tfmv.w.x " << reg_to_string(dstReg) << ", t0\n";
                        return;
                      }
                      int spill_off = std::max(0, int_counter - 8);
                      spill_off += std::max(0, float_counter - 8);
                      spill_off *= -8;
                      if (spill_off >= -2048 && spill_off <= 2047) {
                        load_to_ft(frontend::Operand(srcOp), "ft0",ofs);
                        ofs << "\tfsw ft0, " << spill_off << "(sp)\n";
                      } else {
                        load_to_ft(frontend::Operand(srcOp), "ft0",ofs);
                        ofs << "\tli t0, " << spill_off << "\n";
                        ofs << "\tadd t0, sp, t0\n";
                        ofs << "\tfsw ft0, 0(t0)\n";
                      }
                  }
                },
                srcOp);
            // int int_idx = int_counter++;
            // if (int_idx <= 7) {
            //   if (isFloatReg(srcReg)) {
            //     RISCV_Reg dstReg = static_cast<RISCV_Reg>(static_cast<int>(RISCV_Reg::__fa0) + int_idx);
            //     ofs << "\tfmv.s " << reg_to_string(dstReg) << ", " << reg_to_string(srcReg) << "\n";
            //   } else {
            //     RISCV_Reg dstReg = static_cast<RISCV_Reg>(static_cast<int>(RISCV_Reg::__a0) + int_idx);
            //     ofs << "\tmv " << reg_to_string(dstReg) << ", " << reg_to_string(srcReg) << "\n";
            //   }
            //   //ofs << "\tmv a" << idx << ", " << reg_to_string(reg) << "\n";
            //   pushed_args.push_back(srcReg);
            // } else {
            //   // TODO Spill
            //   int spill_off = 8 * (7 - int_idx);
            //    if (spill_off >= -2048 && spill_off <= 2047) {
            //      if (isFloatReg(srcReg)) {
            //        ofs << "\tfsw " << reg_to_string(srcReg)
            //            << ", " << spill_off << "(sp)\n";
            //      } else {
            //        ofs << "\tsd " << reg_to_string(srcReg)
            //            << ", " << spill_off << "(sp)\n";
            //      }
            //    } else {
            //      ofs << "\tli t0, " << spill_off << "\n";
            //      ofs << "\tadd t0, sp, t0\n";
            //      if (isFloatReg(srcReg)) {
            //        ofs << "\tfsw " << reg_to_string(srcReg) << ", 0(t0)\n";
            //      } else {
            //        ofs << "\tsd " << reg_to_string(srcReg) << ", 0(t0)\n";
            //      }
            //    }
            //   ofs << "\t#push args " << operand_to_str(common->getSrc0()) << "\n"; // just for debug
              //naive: ofs << "\tsd "<< reg_to_string(srcReg) << ", " << 8 * (7 - idx) << "(sp)\n";
            // }
            break;
        }
        case OpCode::Branch: {
            if (branch) {
                ofs << "\tbnez " << get_reg(branch->getCond()) << ", "
                    << branch->getTrueLabel() << "\n";
                ofs << "\tj " << branch->getFalseLabel() << "\n";
            }
            break;
        }
        case OpCode::Jump: {
            ofs << "\tj " << label->getLabel() << "\n";
            break;
        }
        case OpCode::Call: {
            int size = std::max(int_counter - 8, 0);
            size += std::max(float_counter - 8, 0);
            size *= 8;
            if (size > 0) {
                // int size = -8 * (int_counter - 8 + float_counter - 8);
                if (in_range(size)) {
                    ofs << "\tadd sp, sp, " << -size << "\n";
                } else {
                    ofs << "\tli t0, " << -size << "\n";
                    ofs << "\tadd sp, sp, t0\n";
                }
            }
            ofs << "\tcall " << label->getLabel() << "\n";
            if (size > 0) {
                // int size = 8 * (int_counter - 8 + float_counter - 8);
                if (in_range(size)) {
                    ofs << "\tadd sp, sp, " << size << "\n";
                } else {
                    ofs << "\tli t0, " << size << "\n";
                    ofs << "\tadd sp, sp, t0\n";
                }
            }
            // for (int i = 0; i < counter; ++i) {
            //     auto orig = pushed_args[i];
            //     ofs << "\tmv " << reg_to_string(orig) << ", t" << i << "\n";
            // }
            // State reset
            int_counter = 0, float_counter = 0;
            pushed_args.clear();
            break;
        }
        case OpCode::Return: {
            auto src0 = common->getSrc0();
            // 变量返回，可能为 int / float
            if (std::holds_alternative<std::shared_ptr<sym::Var>>(src0)) {
              auto v = std::get<std::shared_ptr<sym::Var>>(src0);
              auto &info = v->prop<sym::RegAllocInfo>();
              RISCV_Reg r = index_to_reg(info.consumed_registers);
              const char* name = get_reg(v);
              if (isFloatReg(r)) {
                ofs << "\tfmv.s fa0, " << name << "\n";
              } else {
                ofs << "\tmv a0, " << name << "\n";
              }
            // 整型返回
            } else if (std::holds_alternative<int>(src0)) {
              int imm = std::get<int>(src0);
              ofs << "\tli a0, " << imm << "\n";
            // 浮点返回
            } else {
              float f = std::get<float>(src0);
              uint32_t bits;
              static_assert(sizeof(bits) == sizeof(f));
              std::memcpy(&bits, &f, sizeof(bits));
              ofs << "\tli t0, " << bits << "\n";
              ofs << "\tfmv.w.x ft0, t0\n";
              ofs << "\tfmv.s fa0, ft0\n";
            }
            ofs << "\tj " << exit_label << "\n";
            break;
        }
        case OpCode::FuncRet: {
          // TODO Fix fmv.w.x f0,a0
          // ofs << "\tmv " << get_reg(common->getDestination()) << ", a0\n";
          auto destVar = common->getDestination();
          auto &destInfo = destVar->prop<sym::RegAllocInfo>();
          RISCV_Reg destReg = index_to_reg(destInfo.consumed_registers);
          const char* reg = get_reg(destVar);
          if (isFloatReg(destReg)) {
            // 浮点函数返回值在 fa0
            // GNU as 伪指令为 fmv.s
            ofs << "\tfmv.s " << reg << ", fa0\n";
          } else {
            ofs << "\tmv " << reg << ", a0\n";
          }
          break;
        }
        case OpCode::Load: {
            // 感觉这里的frame_size没必要减去？
            // int off = std::get<int>(common->getSrc0()) - frame_size;
            int off = std::get<int>(common->getSrc0());
            auto destVar = common->getDestination();
            const char* rd = get_reg(destVar);
            // 合法的小偏移
            if (off >= -2048 && off <= 2047) {
              if (destVar->isFloatVar()) {
                ofs << "\tflw  " << rd << ", " << off << "(s0)\n";
              } else {
                ofs << "\tld   " << rd << ", " << off << "(s0)\n";
              }
            } else {
              ofs << "\tli t0, " << off << "\n";
              ofs << "\tadd t0, s0, t0\n";
              if (destVar->isFloatVar()) {
                ofs << "\tflw " << rd << ", 0(t0)\n";
              } else {
                ofs << "\tld " << rd << ", 0(t0)\n";
              }
            }
            break;
        }
        case OpCode::Load_sp: {
            int off = std::get<int>(common->getSrc0());
            auto destVar = common->getDestination();
            std::string destReg = get_reg(destVar);
            bool isFloat = destVar->getType() == frontend::TypeFactory::FloatTy;

            if (-2048 <= off && off <= 2047) {
                if (isFloat) {
                    ofs << "\tfld  " << destReg << ", " << off << "(sp)\n";
                } else {
                    ofs << "\tld   " << destReg << ", " << off << "(sp)\n";
                }
            } else {
                ofs << "\tli   t0, " << off << "\n";
                ofs << "\tadd  t0, t0, sp\n";
                if (isFloat) {
                    ofs << "\tfld  " << destReg << ", 0(t0)\n";
                } else {
                    ofs << "\tld   " << destReg << ", 0(t0)\n";
                }
            }
            break;
        }
        case OpCode::Store: {
            // auto src0 = common->getSrc0();
            // int off = std::get<int>(common->getSrc1()) - frame_size;
            // if (std::holds_alternative<int>(src0)) {
            //     ofs << "\tli   t0, " << std::get<int>(src0) << "\n";
            //     ofs << "\tsd   t0, " << off << "(s0)\n";
            // } else if (std::holds_alternative<float>(src0)) {
            //     ofs << "\tli   t0, " << std::get<float>(src0) << "\n";
            //     ofs << "\tsd   t0, " << off << "(s0)\n";
            // } else {
            //     ofs << "\tsd   " << operand_to_str(src0)
            //         << ", " << off << "(s0)\n";
            // }
            int off = std::get<int>(common->getSrc1()) - frame_size;
            ofs << "\tsd " << operand_to_str(common->getSrc0()) << ", " << off
                << "(s0)\n";
            break;
        }
        case OpCode::Store_sp: {
            int off = std::get<int>(common->getSrc1());
            bool isFloat = false;
            if (std::holds_alternative<std::shared_ptr<sym::Var>>(common->getSrc0())) {
                auto v = std::get<std::shared_ptr<sym::Var>>(common->getSrc0());
                isFloat = (v->getType() == frontend::TypeFactory::FloatTy);
            }
            if (-2048 <= off && off <= 2047) {
                if (isFloat) {
                    ofs << "\tfsd " << operand_to_str(common->getSrc0())
                        << ", " << off << "(sp)\n";
                } else {
                    ofs << "\tsd " << operand_to_str(common->getSrc0())
                        << ", " << off << "(sp)\n";
                }
            }
            else {
                ofs << "\tli t1, " << off << "\n";
                ofs << "\tadd t1, t1, sp\n";
                if (isFloat) {
                    ofs << "\tfsd " << operand_to_str(common->getSrc0())
                        << ", 0(t1)\n";
                } else {
                    ofs << "\tsd " << operand_to_str(common->getSrc0())
                        << ", 0(t1)\n";
                }
            }
            break;
        }
        case OpCode::GlobalLoad: {
            auto dest  = get_reg(common->getDestination());
            auto var   = std::get<std::shared_ptr<sym::Var>>(common->getSrc0());
            auto name  = var->getName();
            ofs << "\tla t0, " << name << "\n";
            ofs << "\tld " << dest << ", 0(t0)\n";
            break;
        }
        case OpCode::GlobalStore: {
            auto src0 = common->getSrc0();
            auto var  = std::get<std::shared_ptr<sym::Var>>(common->getSrc1());
            auto name = var->getName();
            if (std::holds_alternative<int>(src0)) {
                ofs << "\tli t0, " << std::get<int>(src0) << "\n";
            }
            else if (std::holds_alternative<float>(src0)) {
                ofs << "\tli t0, " << std::get<float>(src0) << "\n";
            }
            ofs << "\tla t1, " << name << "\n";
            if (std::holds_alternative<int>(src0) || std::holds_alternative<float>(src0)) {
                ofs << "\tsd t0, 0(t1)\n";
            } else {
                ofs << "\tsd " << operand_to_str(src0) << ", 0(t1)\n";
            }
            break;
        }
        case OpCode::Read: {
            auto destVar = common->getDestination();
            std::string destReg = get_reg(destVar);
            std::string addr   = operand_to_str(common->getSrc0());

            if (destVar->getType() == frontend::TypeFactory::FloatTy) {
                ofs << "\tflw " << destReg << ", 0(" << addr << ")\n";
            } else {
                ofs << "\tlw  " << destReg << ", 0(" << addr << ")\n";
            }
            break;
        }
        case OpCode::Write: {
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1(); //reg
            auto addr = operand_to_str(src1);
            if (std::holds_alternative<int>(src0)) {
                ofs << "\tli t0, " << std::get<int>(src0) << "\n";
                ofs << "\tsw t0, 0(" << addr << ")\n";
            } else if (std::holds_alternative<float>(src0)) {
                float fv = std::get<float>(src0);
                uint32_t bits;
                static_assert(sizeof(bits) == sizeof(fv));
                std::memcpy(&bits, &fv, sizeof(bits));
                ofs << "\tli t0, " << bits   << "\n";
                ofs << "\tfmv.w.x ft1, t0\n";
                ofs << "\tfsw ft1, 0(" << addr << ")\n";
            } else {
                auto v = std::get<std::shared_ptr<sym::Var>>(src0);
                auto regname = operand_to_str(src0);
                if (v->getType() == frontend::TypeFactory::FloatTy) {
                    ofs << "\tfsw " << regname << ", 0(" << addr << ")\n";
                } else {
                    ofs << "\tsw " << regname << ", 0(" << addr << ")\n";
                }
            }
            break;
        }
        case OpCode::AddressOfFunc: {
            ofs << "\tla " << get_reg(common->getDestination()) << ", "
                << label->getLabel() << "\n";
            break;
        }
        case OpCode::LoadFunc: {
            int off = std::get<int>(common->getSrc0()) - frame_size;
            ofs << "\tld t0, " << off << "(s0)\n";
            break;
        }
        case OpCode::GlobalLoadFunc: {
            ofs << "\tld t0, " << operand_to_str(common->getSrc0()) << "(gp)\n";
            break;
        }
        case OpCode::Indirect: {
            ofs << "\tjalr ra, t0, 0\n";
            break;
        }
        case OpCode::Negate: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            if (std::holds_alternative<int>(src0)) {
                int v = -std::get<int>(src0);
                ofs << "\tli   " << dest << ", " << v << "\n";
            } else if (std::holds_alternative<float>(src0)) { //just in case
                ofs << "\tli   t0, " << std::get<float>(src0) << "\n";
                ofs << "\tfneg.s " << dest << ", t0\n";
            } else {
                ofs << "\tneg  " << dest << ", " << operand_to_str(src0) << "\n";
            }
            break;
        }
        case OpCode::FloatNegate: {
            ofs << "\tfneg.s " << get_reg(common->getDestination()) << ", "
                << operand_to_str(common->getSrc0()) << "\n";
            break;
        }
        // 64bit add
        case OpCode::Add64: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();
            if (std::holds_alternative<int>(src0) && std::holds_alternative<int>(src1)) {
                int sum = std::get<int>(src0) + std::get<int>(src1);
                ofs << "\tli   " << dest << ", " << sum << "\n";
            } else if (std::holds_alternative<int>(src1)) {
                int imm = std::get<int>(src1);
                if(in_range(imm)) {
                    ofs << "\taddi " << dest << ", "
                        << operand_to_str(src0) << ", "
                        << imm << "\n";
                } else {
                    ofs << "\tli t2, " << imm << "\n";
                    ofs << "\tadd " << dest << ", " << operand_to_str(src0) << ", t2\n";
                }
            } else if (std::holds_alternative<int>(src0)) {
                int imm = std::get<int>(src0);
                if(in_range(imm)) {
                    ofs << "\taddi " << dest << ", "
                        << operand_to_str(src1) << ", "
                        << imm << "\n";
                } else {
                    ofs << "\tli t2, " << imm << "\n";
                    ofs << "\tadd " << dest << ", " << operand_to_str(src0) << ", t2\n";
                }
            } else {
                ofs << "\tadd  " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << operand_to_str(src1) << "\n";
            }
            break;
        }
        case OpCode::Add32: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();
            if (std::holds_alternative<int>(src0) && std::holds_alternative<int>(src1)) {
                int sum = std::get<int>(src0) + std::get<int>(src1);
                ofs << "\tli   " << dest << ", " << sum << "\n";
            } else if (std::holds_alternative<int>(src1)) {
                int imm = std::get<int>(src1);
                if(in_range(imm)) {
                    ofs << "\taddi " << dest << ", "
                        << operand_to_str(src0) << ", "
                        << imm << "\n";
                } else {
                    ofs << "\tli t2, " << imm << "\n";
                    ofs << "\tadd " << dest << ", " << operand_to_str(src0) << ", t2\n";
                }
            } else if (std::holds_alternative<int>(src0)) {
                int imm = std::get<int>(src0);
                if(in_range(imm)) {
                    ofs << "\taddi " << dest << ", "
                        << operand_to_str(src1) << ", "
                        << imm << "\n";
                } else {
                    ofs << "\tli t2, " << imm << "\n";
                    ofs << "\tadd " << dest << ", " << operand_to_str(src0) << ", t2\n";
                }
            } else {
                ofs << "\tadd  " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << operand_to_str(src1) << "\n";
            }
            ofs << "\tsext.w " << dest << ", " << dest << "\n";
            break;
        }
        case OpCode::Sub: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();
            if (std::holds_alternative<int>(src0) && std::holds_alternative<int>(src1)) {
                int diff = std::get<int>(src0) - std::get<int>(src1);
                ofs << "\tli   " << dest << ", " << diff << "\n";
            } else if (std::holds_alternative<int>(src1)) {
                // int imm = std::get<int>(src1);
                // ofs << "\taddi " << dest << ", "
                //     << operand_to_str(src0) << ", " << -imm << "\n";
                int imm = std::get<int>(src1);
                if(in_range(-imm)) {
                    ofs << "\taddi " << dest << ", "
                        << operand_to_str(src0) << ", "
                        << -imm << "\n";
                } else {
                    ofs << "\tli t2, " << -imm << "\n";
                    ofs << "\tadd " << dest << ", " << operand_to_str(src0) << ", t2\n";
                }
            } else if (std::holds_alternative<int>(src0)) {
                int imm0 = std::get<int>(src0);
                ofs << "\tli   t1, " << imm0 << "\n";
                ofs << "\tsub  " << dest << ", t1, " << operand_to_str(src1) << "\n";
                // int imm = std::get<int>(src0);
                // if(in_range(-imm)) {
                //     ofs << "\taddi " << dest << ", "
                //         << operand_to_str(src1) << ", "
                //         << -imm << "\n";
                // } else {
                //     ofs << "\tli t2, " << -imm << "\n";
                //     ofs << "\tadd " << dest << ", " << operand_to_str(src0) << ", t2\n";
                // }
            } else {
                ofs << "\tsub " << dest << ", "
                << operand_to_str(src0) << ", "
                << operand_to_str(src1) << "\n";
            }
            break;
        }
        case OpCode::Mul64: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();
            if (std::holds_alternative<int>(src0) && std::holds_alternative<int>(src1)) {
                int prod = std::get<int>(src0) * std::get<int>(src1);
                ofs << "\tli   " << dest << ", " << prod << "\n";
            } else if (std::holds_alternative<int>(src0)) {
                ofs << "\tli   t1, " << std::get<int>(src0) << "\n";
                ofs << "\tmul  " << dest << ", t1, " << operand_to_str(src1) << "\n";
            } else if (std::holds_alternative<int>(src1)) {
                ofs << "\tli   t2, " << std::get<int>(src1) << "\n";
                ofs << "\tmul  " << dest << ", " << operand_to_str(src0) << ", t2\n";
            } else {
                ofs << "\tmul  " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << operand_to_str(src1) << "\n";
            }
            break;
        }
        case OpCode::Mul32: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();
            if (std::holds_alternative<int>(src0) && std::holds_alternative<int>(src1)) {
                int prod = std::get<int>(src0) * std::get<int>(src1);
                ofs << "\tli   " << dest << ", " << prod << "\n";
            } else if (std::holds_alternative<int>(src0)) {
                ofs << "\tli   t1, " << std::get<int>(src0) << "\n";
                ofs << "\tmul  " << dest << ", t1, " << operand_to_str(src1) << "\n";
            } else if (std::holds_alternative<int>(src1)) {
                ofs << "\tli   t2, " << std::get<int>(src1) << "\n";
                ofs << "\tmul  " << dest << ", " << operand_to_str(src0) << ", t2\n";
            } else {
                ofs << "\tmul  " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << operand_to_str(src1) << "\n";
            }
            ofs << "\tsext.w " << dest << ", " << dest << "\n";
            break;
        }
        case OpCode::Div: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();

            if (std::holds_alternative<int>(src0) && std::holds_alternative<int>(src1)) {
                int q = std::get<int>(src0) / std::get<int>(src1);
                ofs << "\tli   " << dest << ", " << q << "\n";
            } else if (std::holds_alternative<int>(src1)) {
                ofs << "\tli   t2, " << std::get<int>(src1) << "\n";
                ofs << "\tdiv  " << dest << ", "
                    << operand_to_str(src0) << ", t2\n";
            } else if (std::holds_alternative<int>(src0)) {
                ofs << "\tli   t1, " << std::get<int>(src0) << "\n";
                ofs << "\tdiv  " << dest << ", t1, "
                    << operand_to_str(src1) << "\n";
            } else {
                ofs << "\tdiv  " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << operand_to_str(src1) << "\n";
            }
            break;
        }
        case OpCode::FloatAdd: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();

            load_to_ft(src0, "ft1", ofs);
            load_to_ft(src1, "ft2", ofs);
            ofs << "\tfadd.s    " << dest << ", ft1, ft2\n";
            break;
        }
        case OpCode::FloatSub: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();

            load_to_ft(src0, "ft1", ofs);
            load_to_ft(src1, "ft2", ofs);
            ofs << "\tfsub.s    " << dest << ", ft1, ft2\n";
            break;
        }
        case OpCode::FloatMul: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();

            load_to_ft(src0, "ft1", ofs);
            load_to_ft(src1, "ft2", ofs);
            ofs << "\tfmul.s    " << dest << ", ft1, ft2\n";
            break;
        }
        case OpCode::FloatDiv: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();

            load_to_ft(src0, "ft1", ofs);
            load_to_ft(src1, "ft2", ofs);
            ofs << "\tfdiv.s    " << dest << ", ft1, ft2\n";
            break;
        }
        case OpCode::Mod: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();
            if (std::holds_alternative<int>(src0) && std::holds_alternative<int>(src1)) {
                int r = std::get<int>(src0) % std::get<int>(src1);
                ofs << "\tli   " << dest << ", " << r << "\n";
            } else if (std::holds_alternative<int>(src1)) {
                ofs << "\tli   t2, " << std::get<int>(src1) << "\n";
                ofs << "\tremw  " << dest << ", "
                    << operand_to_str(src0) << ", t2\n";
            } else if (std::holds_alternative<int>(src0)) {
                ofs << "\tli   t1, " << std::get<int>(src0) << "\n";
                ofs << "\tremw  " << dest << ", t1, "
                    << operand_to_str(src1) << "\n";
            } else {
                ofs << "\tremw  " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << operand_to_str(src1) << "\n";
            }
            break;
        }
        case OpCode::Eq: {
          auto dstVar = common->getDestination();
          const char* rd = get_reg(dstVar);

          auto src0 = common->getSrc0();
          auto src1 = common->getSrc1();

          bool floatCmp = operandIsFloat(src0) || operandIsFloat(src1);

          if (floatCmp) {
            load_to_ft(src0, "ft1", ofs);
            load_to_ft(src1, "ft2", ofs);
            ofs << "\tfeq.s t0, ft1, ft2\n";
            ofs << "\tsnez " << rd << ", t0\n";
          } else {
            std::string op0 = operand_to_str(src0);;
            std::string op1 = operand_to_str(src1);
            if (std::holds_alternative<int>(src0)) {
              ofs << "\tli t1, " << std::get<int>(src0) << "\n";
              op0 = "t1";
            }
            if (std::holds_alternative<int>(src1)) {
              ofs << "\tli t2, " << std::get<int>(src1) << "\n";
              op1 = "t2";
            }
            ofs << "\txor " << rd << ", " << op0 << ", " << op1 << "\n";
            // ofs << "\txor " << rd << ", " << operand_to_str(src0) << ", " << operand_to_str(src1) << "\n";
            ofs << "\tseqz " << rd << ", " << rd << "\n";
          }
          break;
        }
        case OpCode::Neq: {
          auto dstVar = common->getDestination();
          const char* rd = get_reg(dstVar);

          auto src0 = common->getSrc0();
          auto src1 = common->getSrc1();
          std::string op1 = operand_to_str(src0);
          std::string op2 = operand_to_str(src1);

          bool floatCmp = operandIsFloat(src0) || operandIsFloat(src1);

          if (floatCmp) {
            load_to_ft(src0, "ft1", ofs);
            load_to_ft(src1, "ft2", ofs);
            ofs << "\tfeq.s t0, ft1, ft2\n";
            ofs << "\txori " << rd << ", t0, 1\n";
          } else {
            // std::string op1 = operand_to_str(src0);
            // std::string op2 = operand_to_str(src1);
            // ofs << "\txor " << rd << ", " << op1 << ", " << op2 << "\n";
            std::string op0 = operand_to_str(src0);;
            std::string op1 = operand_to_str(src1);
            if (std::holds_alternative<int>(src0)) {
              ofs << "\tli t1, " << std::get<int>(src0) << "\n";
              op0 = "t1";
            }
            if (std::holds_alternative<int>(src1)) {
              ofs << "\tli t2, " << std::get<int>(src1) << "\n";
              op1 = "t2";
            }
            ofs << "\txor " << rd << ", " << op0 << ", " << op1 << "\n";
            ofs << "\tsnez " << rd << ", " << rd   << "\n";
          }
          break;
        }
        case OpCode::Gt: {
          auto dstVar = common->getDestination();
          const char* rd = get_reg(dstVar);

          auto src0 = common->getSrc0();
          auto src1 = common->getSrc1();

          bool floatCmp = operandIsFloat(src0) || operandIsFloat(src1);

          if (floatCmp) {
            load_to_ft(src0, "ft1", ofs);
            load_to_ft(src1, "ft2", ofs);
            ofs << "\tflt.s " << rd << ", ft2, ft1\n";
          } else {
            std::string op0 = operand_to_str(src0);;
            std::string op1 = operand_to_str(src1);
            if (std::holds_alternative<int>(src0)) {
              ofs << "\tli t1, " << std::get<int>(src0) << "\n";
              op0 = "t1";
            }
            if (std::holds_alternative<int>(src1)) {
              ofs << "\tli t2, " << std::get<int>(src1) << "\n";
              op1 = "t2";
            }
            ofs << "\tsgt " << rd << ", " << op0 << ", " << op1 << "\n";
            // ofs << "\tsgt " << rd << ", "
            //     << operand_to_str(src0) << ", "
            //     << operand_to_str(src1) << "\n";
          }
          break;
        }
        case OpCode::Lt: {
          auto dstVar = common->getDestination();
          const char* rd = get_reg(dstVar);

          auto src0 = common->getSrc0();
          auto src1 = common->getSrc1();

          bool floatCmp = operandIsFloat(src0) || operandIsFloat(src1);

          if (floatCmp) {
            load_to_ft(src0, "ft1", ofs);
            load_to_ft(src1, "ft2", ofs);
            ofs << "\tflt.s   " << rd << ", ft1, ft2\n";
          } else {
                        std::string op0 = operand_to_str(src0);;
            std::string op1 = operand_to_str(src1);
            if (std::holds_alternative<int>(src0)) {
              ofs << "\tli t1, " << std::get<int>(src0) << "\n";
              op0 = "t1";
            }
            if (std::holds_alternative<int>(src1)) {
              ofs << "\tli t2, " << std::get<int>(src1) << "\n";
              op1 = "t2";
            }
            ofs << "\tslt " << rd << ", " << op0 << ", " << op1 << "\n";
            // ofs << "\tslt     " << rd << ", "
            //     << operand_to_str(src0) << ", "
            //     << operand_to_str(src1) << "\n";
          }
          break;
        }
        case OpCode::Geq: {
          auto dstVar = common->getDestination();
          const char* rd = get_reg(dstVar);

          auto src0 = common->getSrc0();
          auto src1 = common->getSrc1();

          bool floatCmp = operandIsFloat(src0) || operandIsFloat(src1);

          if (floatCmp) {
            load_to_ft(src0, "ft1", ofs);
            load_to_ft(src1, "ft2", ofs);
            ofs << "\tflt.s t0, ft1, ft2\n";
            ofs << "\txori " << rd << ", t0, 1\n";
          } else {
            std::string op0 = operand_to_str(src0);;
            std::string op1 = operand_to_str(src1);
            if (std::holds_alternative<int>(src0)) {
              ofs << "\tli t1, " << std::get<int>(src0) << "\n";
              op0 = "t1";
            }
            if (std::holds_alternative<int>(src1)) {
              ofs << "\tli t2, " << std::get<int>(src1) << "\n";
              op1 = "t2";
            }
            ofs << "\tslt " << rd << ", " << op0 << ", " << op1 << "\n";
            // ofs << "\tslt " << rd << ", " << operand_to_str(src0) << ", " << operand_to_str(src1) << "\n";
            ofs << "\txori " << rd << ", " << rd << ", 1\n";
          }
          break;
        }

        case OpCode::Leq: {
          auto dstVar = common->getDestination();
          const char* rd = get_reg(dstVar);

          auto src0 = common->getSrc0();
          auto src1 = common->getSrc1();

          bool floatCmp = operandIsFloat(src0) || operandIsFloat(src1);

          if (floatCmp) {
            load_to_ft(src0, "ft1", ofs);
            load_to_ft(src1, "ft2", ofs);
            ofs << "\tfle.s " << rd << ", ft1, ft2\n";
          } else {
            std::string op0 = operand_to_str(src0);;
            std::string op1 = operand_to_str(src1);
            if (std::holds_alternative<int>(src0)) {
              ofs << "\tli t1, " << std::get<int>(src0) << "\n";
              op0 = "t1";
            }
            if (std::holds_alternative<int>(src1)) {
              ofs << "\tli t2, " << std::get<int>(src1) << "\n";
              op1 = "t2";
            }
            ofs << "\tsgt " << rd << ", " << op0 << ", " << op1 << "\n";
            // ofs << "\tsgt " << rd << ", " << operand_to_str(src0) << ", " << operand_to_str(src1) << "\n";
            ofs << "\txori " << rd << ", " << rd << ", 1\n";
          }
          break;
        }

        case OpCode::BitAnd: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();
            if (std::holds_alternative<int>(src0)) {
                int imm = std::get<int>(src0);
                if (in_range(imm)) {
                    ofs << "\tandi " << dest << ", "
                        << operand_to_str(src1) << ", " << imm << "\n";
                } else {
                    ofs << "\tli t2, " << imm << "\n";
                    ofs << "\tand  " << dest << ", t2, "
                        << operand_to_str(src1) << "\n";
                }
            } else if (std::holds_alternative<int>(src1)) {
                int imm = std::get<int>(src1);
                if (in_range(imm)) {
                    ofs << "\tandi " << dest << ", "
                        << operand_to_str(src0) << ", " << imm << "\n";
                } else {
                    ofs << "\tli t2, " << imm << "\n";
                    ofs << "\tand  " << dest << ", "
                        << operand_to_str(src0) << ", t2\n";
                }
            } else {
                ofs << "\tand  " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << operand_to_str(src1) << "\n";
            }
            break;
        }
        case OpCode::BitOr: {
            ofs << "\tor " << get_reg(common->getDestination()) << ", "
                << operand_to_str(common->getSrc0()) << ", "
                << operand_to_str(common->getSrc1()) << "\n";
            break;
        }
        case OpCode::BitNot: {
            ofs << "\txori " << get_reg(common->getDestination()) << ", "
                << operand_to_str(common->getSrc0()) << ", -1\n";
            break;
        }
        case OpCode::BitXor: {
            ofs << "\txor " << get_reg(common->getDestination()) << ", "
                << operand_to_str(common->getSrc0()) << ", "
                << operand_to_str(common->getSrc1()) << "\n";
            break;
        }
        case OpCode::LogNot: {
            ofs << "\tseqz " << get_reg(common->getDestination()) << ", "
                << operand_to_str(common->getSrc0()) << "\n";
            break;
        }
        case OpCode::RShift: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();
            if (std::holds_alternative<int>(src1)) {
                ofs << "\tsrai " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << std::get<int>(src1) << "\n";
            } else {
                ofs << "\tsra  " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << operand_to_str(src1) << "\n";
            }
            break;
        }
        case OpCode::LShift: {
            auto dest = get_reg(common->getDestination());
            auto src0 = common->getSrc0();
            auto src1 = common->getSrc1();
            if (std::holds_alternative<int>(src1)) {
                ofs << "\tslli " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << std::get<int>(src1) << "\n";
            } else {
                ofs << "\tsll  " << dest << ", "
                    << operand_to_str(src0) << ", "
                    << operand_to_str(src1) << "\n";
            }
            break;
        }
        case OpCode::Start: {
            ofs << "\t# <start>\n";
            break;
        }
        case OpCode::Nop: {
            ofs << "\tnop # " << nop_dist(nop_rng) << "\n";
            break;
        }
        default:
            break;
        }
    }
} // namespace backend
} // namespace aaac
