#include "../include/backend/RiscvPrinter.h"
#include "../include/backend/Riscv.h"
#include "../runtime/functionCache.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>

/**
 * @file RiscvPrinter.h
 *
 * @brief 打印Riscv后端汇编指令的源文件
 */

// 局部数组不用全部初始化
namespace riscv {
auto RiscvCodePrinter::getOperandName(Register *reg) -> std::string {
  using IntKind = IntPhyRegister::IntRegisterKind;
  using FloatKind = FloatPhyRegister::FloatRegisterKind;
  std::string result;
  auto symbolReg = dynamic_cast<SymRegister *>(reg);
  auto phyReg = dynamic_cast<PhyRegister *>(reg);
  if (symbolReg != nullptr) {
    result = symbolReg->getName();
  } else {
    auto intPhyreg = dynamic_cast<IntPhyRegister *>(phyReg);
    auto floatPhyreg = dynamic_cast<FloatPhyRegister *>(phyReg);
    if (intPhyreg != nullptr) {
      switch (intPhyreg->getIntPhyRegisterKind()) {
      case IntKind::zero:
        result = "zero";
        break;
      case IntKind::ra:
        result = "ra";
        break;
      case IntKind::sp:
        result = "sp";
        break;
      case IntKind::gp:
        result = "gp";
        break;
      case IntKind::tp:
        result = "tp";
        break;
      case IntKind::t0:
        result = "t0";
        break;
      case IntKind::t1:
        result = "t1";
        break;
      case IntKind::t2:
        result = "t2";
        break;
      case IntKind::fp:
        result = "fp";
        break;
      case IntKind::s1:
        result = "s1";
        break;
      case IntKind::a0:
        result = "a0";
        break;
      case IntKind::a1:
        result = "a1";
        break;
      case IntKind::a2:
        result = "a2";
        break;
      case IntKind::a3:
        result = "a3";
        break;
      case IntKind::a4:
        result = "a4";
        break;
      case IntKind::a5:
        result = "a5";
        break;
      case IntKind::a6:
        result = "a6";
        break;
      case IntKind::a7:
        result = "a7";
        break;
      case IntKind::s2:
        result = "s2";
        break;
      case IntKind::s3:
        result = "s3";
        break;
      case IntKind::s4:
        result = "s4";
        break;
      case IntKind::s5:
        result = "s5";
        break;
      case IntKind::s6:
        result = "s6";
        break;
      case IntKind::s7:
        result = "s7";
        break;
      case IntKind::s8:
        result = "s8";
        break;
      case IntKind::s9:
        result = "s9";
        break;
      case IntKind::s10:
        result = "s10";
        break;
      case IntKind::s11:
        result = "s11";
        break;
      case IntKind::t3:
        result = "t3";
        break;
      case IntKind::t4:
        result = "t4";
        break;
      case IntKind::t5:
        result = "t5";
        break;
      case IntKind::t6:
        result = "t6";
        break;
      default:
        assert(false);
      }
    } else {
      switch (floatPhyreg->getFloatRegisterKind()) {
      case FloatKind::ft0:
        result = "ft0";
        break;
      case FloatKind::ft1:
        result = "ft1";
        break;
      case FloatKind::ft2:
        result = "ft2";
        break;
      case FloatKind::ft3:
        result = "ft3";
        break;
      case FloatKind::ft4:
        result = "ft4";
        break;
      case FloatKind::ft5:
        result = "ft5";
        break;
      case FloatKind::ft6:
        result = "ft6";
        break;
      case FloatKind::ft7:
        result = "ft7";
        break;
      case FloatKind::fs0:
        result = "fs0";
        break;
      case FloatKind::fs1:
        result = "fs1";
        break;
      case FloatKind::fa0:
        result = "fa0";
        break;
      case FloatKind::fa1:
        result = "fa1";
        break;
      case FloatKind::fa2:
        result = "fa2";
        break;
      case FloatKind::fa3:
        result = "fa3";
        break;
      case FloatKind::fa4:
        result = "fa4";
        break;
      case FloatKind::fa5:
        result = "fa5";
        break;
      case FloatKind::fa6:
        result = "fa6";
        break;
      case FloatKind::fa7:
        result = "fa7";
        break;
      case FloatKind::fs2:
        result = "fs2";
        break;
      case FloatKind::fs3:
        result = "fs3";
        break;
      case FloatKind::fs4:
        result = "fs4";
        break;
      case FloatKind::fs5:
        result = "fs5";
        break;
      case FloatKind::fs6:
        result = "fs6";
        break;
      case FloatKind::fs7:
        result = "fs7";
        break;
      case FloatKind::fs8:
        result = "fs8";
        break;
      case FloatKind::fs9:
        result = "fs9";
        break;
      case FloatKind::fs10:
        result = "fs10";
        break;
      case FloatKind::fs11:
        result = "fs11";
        break;
      case FloatKind::ft8:
        result = "ft8";
        break;
      case FloatKind::ft9:
        result = "ft9";
        break;
      case FloatKind::ft10:
        result = "ft10";
        break;
      case FloatKind::ft11:
        result = "ft11";
        break;
      default:
        assert(false);
      }
    }
  }

  return result;
}
auto RiscvCodePrinter::getInstName(Instruction *inst) -> std::string {
  std::string result;
  switch (inst->getKind()) {
  case Instruction::kSll:
    result = "sll";
    break;
  case Instruction::kSlli:
    result = "slli";
    break;
  case Instruction::kSrl:
    result = "srl";
    break;
  case Instruction::kSrli:
    result = "srli";
    break;
  case Instruction::kSra:
    result = "sra";
    break;
  case Instruction::kSrai:
    result = "srai";
    break;
  case Instruction::kSllw:
    result = "sllw";
    break;
  case Instruction::kSlliw:
    result = "slliw";
    break;
  case Instruction::kSrlw:
    result = "srlw";
    break;
  case Instruction::kSrliw:
    result = "srliw";
    break;
  case Instruction::kSraw:
    result = "sraw";
    break;
  case Instruction::kSraiw:
    result = "sraiw";
    break;
  case Instruction::kAdd:
    result = "add";
    break;
  case Instruction::kAddi:
    result = "addi";
    break;
  case Instruction::kSub:
    result = "sub";
    break;
  case Instruction::kAddw:
    result = "addw";
    break;
  case Instruction::kAddiw:
    result = "addiw";
    break;
  case Instruction::kSubw:
    result = "subw";
    break;
  case Instruction::kLui:
    result = "lui";
    break;
  case Instruction::kAuipc:
    result = "auipc";
    break;
  case Instruction::kXor:
    result = "xor";
    break;
  case Instruction::kXori:
    result = "xori";
    break;
  case Instruction::kOr:
    result = "or";
    break;
  case Instruction::kOri:
    result = "ori";
    break;
  case Instruction::kAnd:
    result = "and";
    break;
  case Instruction::kAndi:
    result = "andi";
    break;
  case Instruction::kSlt:
    result = "slt";
    break;
  case Instruction::kSlti:
    result = "slti";
    break;
  case Instruction::kSltu:
    result = "sltu";
    break;
  case Instruction::kSltiu:
    result = "sltiu";
    break;
  case Instruction::kBeq:
    result = "beq";
    break;
  case Instruction::kBne:
    result = "bne";
    break;
  case Instruction::kBlt:
    result = "blt";
    break;
  case Instruction::kBge:
    result = "bge";
    break;
  case Instruction::kBltu:
    result = "bltu";
    break;
  case Instruction::kBgeu:
    result = "bgeu";
    break;
  case Instruction::kJal:
    result = "jal";
    break;
  case Instruction::kJalr:
    result = "jalr";
    break;
  case Instruction::kLb:
    result = "lb";
    break;
  case Instruction::kLh:
    result = "lh";
    break;
  case Instruction::kLbu:
    result = "lbu";
    break;
  case Instruction::kLhu:
    result = "lhu";
    break;
  case Instruction::kLw:
    result = "lw";
    break;
  case Instruction::kLwu:
    result = "lwu";
    break;
  case Instruction::kLd:
    result = "ld";
    break;
  case Instruction::kSb:
    result = "sb";
    break;
  case Instruction::kSh:
    result = "sh";
    break;
  case Instruction::kSw:
    result = "sw";
    break;
  case Instruction::kSd:
    result = "sd";
    break;
  case Instruction::kMul:
    result = "mul";
    break;
  case Instruction::kMulh:
    result = "mulh";
    break;
  case Instruction::kMulhsu:
    result = "mulhsu";
    break;
  case Instruction::kMulhu:
    result = "mulhu";
    break;
  case Instruction::kMulw:
    result = "mulw";
    break;
  case Instruction::kDiv:
    result = "div";
    break;
  case Instruction::kDivu:
    result = "divu";
    break;
  case Instruction::kDivw:
    result = "divw";
    break;
  case Instruction::kRem:
    result = "rem";
    break;
  case Instruction::kRemu:
    result = "remu";
    break;
  case Instruction::kRemw:
    result = "remw";
    break;
  case Instruction::kRemuw:
    result = "remuw";
    break;
  case Instruction::kFmv_w_x:
    result = "fmv.w.x";
    break;
  case Instruction::kFmv_x_w:
    result = "fmv.x.w";
    break;
  case Instruction::kFmv_d_x:
    result = "fmv.d.x";
    break;
  case Instruction::kFmv_x_d:
    result = "fmv.x.d";
    break;
  case Instruction::kFcvt_s_w:
    result = "fcvt.s.w";
    break;
  case Instruction::kFcvt_s_wu:
    result = "fcvt.s.wu";
    break;
  case Instruction::kFcvt_w_s:
    result = "fcvt.w.s";
    break;
  case Instruction::kFcvt_wu_s:
    result = "fcvt.wu.s";
    break;
  case Instruction::kFcvt_s_l:
    result = "fcvt.s.l";
    break;
  case Instruction::kFcvt_s_lu:
    result = "fcvt.s.lu";
    break;
  case Instruction::kFcvt_l_s:
    result = "fcvt.l.s";
    break;
  case Instruction::kFcvt_lu_s:
    result = "fcvt.lu.s";
    break;
  case Instruction::kFlw:
    result = "flw";
    break;
  case Instruction::kFsw:
    result = "fsw";
    break;
  case Instruction::kFadd_s:
    result = "fadd.s";
    break;
  case Instruction::kFsub_s:
    result = "fsub.s";
    break;
  case Instruction::kFmul_s:
    result = "fmul.s";
    break;
  case Instruction::kFdiv_s:
    result = "fdiv.s";
    break;
  case Instruction::kFmadd_s:
    result = "fmadd.s";
    break;
  case Instruction::kFmsub_s:
    result = "fmsub.s";
    break;
  case Instruction::kFnmsub_s:
    result = "fnmsub.s";
    break;
  case Instruction::kFnmadd_s:
    result = "fnmadd.s";
    break;
  case Instruction::kFsgnj_s:
    result = "fsgnj.s";
    break;
  case Instruction::kFsgnjn_s:
    result = "fsgnjn.s";
    break;
  case Instruction::kFsgnjx_s:
    result = "fsgnjx.s";
    break;
  case Instruction::kFmin_s:
    result = "fmin.s";
    break;
  case Instruction::kFmax_s:
    result = "fmax.s";
    break;
  case Instruction::kFeq_s:
    result = "feq.s";
    break;
  case Instruction::kFlt_s:
    result = "flt.s";
    break;
  case Instruction::kFle_s:
    result = "fle.s";
    break;
  case Instruction::kFclass_s:
    result = "fclass.s";
    break;
  case Instruction::kLla:
    result = "lla";
    break;
  case Instruction::kLi:
    result = "li";
    break;
  case Instruction::kSext_w:
    result = "sext.w";
    break;
  case Instruction::kCall:
    result = "call";
    break;
  default:
    assert(false);
  }
  return result;
}
void RiscvCodePrinter::printRInst(RInst *inst) {
  std::cout << getInstName(inst) << " "
            << getOperandName(inst->getDestination()) << ","
            << getOperandName(inst->getSource(0)) << ","
            << getOperandName(inst->getSource(1)) << std::endl;
}
void RiscvCodePrinter::printIInst(IInst *inst) {
  if (inst->getKind() != IInst::kLb && inst->getKind() != IInst::kLbu &&
      inst->getKind() != IInst::kLh && inst->getKind() != IInst::kLhu &&
      inst->getKind() != IInst::kLw && inst->getKind() != IInst::kLwu &&
      inst->getKind() != IInst::kLd) {
    std::cout << getInstName(inst) << " "
              << getOperandName(inst->getDestination()) << ","
              << getOperandName(inst->getSource(0)) << "," << inst->getImm()
              << std::endl;
  } else {
    std::cout << getInstName(inst) << " "
              << getOperandName(inst->getDestination()) << "," << inst->getImm()
              << "(" << getOperandName(inst->getSource(0)) << ")" << std::endl;
  }
}
void RiscvCodePrinter::printUInst(UInst *inst) {
  std::cout << getInstName(inst) << " "
            << getOperandName(inst->getDestination()) << "," << inst->getImm()
            << std::endl;
}
void RiscvCodePrinter::printSInst(SInst *inst) {
  std::cout << getInstName(inst) << " " << getOperandName(inst->getSource(0))
            << "," << inst->getImm() << "("
            << getOperandName(inst->getSource(1)) << ")" << std::endl;
}
void RiscvCodePrinter::printBInst(BInst *inst) {
  std::cout << getInstName(inst) << " " << getOperandName(inst->getSource(0))
            << "," << getOperandName(inst->getSource(1)) << ","
            << inst->getThenBlock()->getName() << std::endl;
}
void RiscvCodePrinter::printJInst(JInst *inst) {
  std::cout << getInstName(inst) << " "
            << getOperandName(inst->getDestination()) << ","
            << inst->getLabel()->getName() << std::endl;
}
void RiscvCodePrinter::printI2FInst(I2FInst *inst) {
  if (inst->getKind() == riscv::I2FInst::kFcvt_s_w ||
      inst->getKind() == riscv::I2FInst::kFcvt_s_wu ||
      inst->getKind() == riscv::I2FInst::kFcvt_s_l ||
      inst->getKind() == riscv::I2FInst::kFcvt_s_lu) {
    std::cout << getInstName(inst) << " "
              << getOperandName(inst->getDestination()) << ","
              << getOperandName(inst->getSource(0)) << ","
              << "rne" << std::endl;
  } else {
    std::cout << getInstName(inst) << " "
              << getOperandName(inst->getDestination()) << ","
              << getOperandName(inst->getSource(0)) << std::endl;
  }
}
void RiscvCodePrinter::printF2IInst(F2IInst *inst) {
  if (inst->getKind() == riscv::F2IInst::kFcvt_w_s ||
      inst->getKind() == riscv::F2IInst::kFcvt_wu_s ||
      inst->getKind() == riscv::F2IInst::kFcvt_l_s ||
      inst->getKind() == riscv::F2IInst::kFcvt_lu_s) {
    std::cout << getInstName(inst) << " "
              << getOperandName(inst->getDestination()) << ","
              << getOperandName(inst->getSource(0)) << ","
              << "rtz" << std::endl;
  } else {
    std::cout << getInstName(inst) << " "
              << getOperandName(inst->getDestination()) << ","
              << getOperandName(inst->getSource(0)) << std::endl;
  }
}
void RiscvCodePrinter::printFRInst(FRInst *inst) {
  if (inst->getKind() == riscv::FRInst::kFadd_s ||
      inst->getKind() == riscv::FRInst::kFsub_s ||
      inst->getKind() == riscv::FRInst::kFmul_s ||
      inst->getKind() == riscv::FRInst::kFdiv_s) {
    std::cout << getInstName(inst) << " "
              << getOperandName(inst->getDestination()) << ","
              << getOperandName(inst->getSource(0)) << ","
              << getOperandName(inst->getSource(1)) << ","
              << "rne" << std::endl;
  } else {
    std::cout << getInstName(inst) << " "
              << getOperandName(inst->getDestination()) << ","
              << getOperandName(inst->getSource(0)) << ","
              << getOperandName(inst->getSource(1)) << std::endl;
  }
}
void RiscvCodePrinter::printFR2IInst(FR2IInst *inst) {
  std::cout << getInstName(inst) << " "
            << getOperandName(inst->getDestination()) << ","
            << getOperandName(inst->getSource(0)) << ","
            << getOperandName(inst->getSource(1)) << std::endl;
}
void RiscvCodePrinter::printFLongInst(FLongInst *inst) {
  std::cout << getInstName(inst) << " "
            << getOperandName(inst->getDestination()) << ","
            << getOperandName(inst->getSource(0)) << ","
            << getOperandName(inst->getSource(1)) << ","
            << getOperandName(inst->getSource(2)) << ","
            << "rne" << std::endl;
}
void RiscvCodePrinter::printFLInst(FLInst *inst) {
  std::cout << getInstName(inst) << " "
            << getOperandName(inst->getDestination()) << "," << inst->getImm()
            << "(" << getOperandName(inst->getSource(0)) << ")" << std::endl;
}
void RiscvCodePrinter::printFSInst(FSInst *inst) {
  std::cout << getInstName(inst) << " " << getOperandName(inst->getSource(0))
            << "," << inst->getImm() << "("
            << getOperandName(inst->getSource(1)) << ")" << std::endl;
}
void RiscvCodePrinter::printLLaInst(LlaInst *inst) {
  std::cout << getInstName(inst) << " "
            << getOperandName(inst->getDestination()) << ","
            << inst->getSymbol()->getName() << std::endl;
}
void RiscvCodePrinter::printLiInst(LiInst *inst) {
  std::cout << getInstName(inst) << " "
            << getOperandName(inst->getDestination()) << "," << inst->getImm()
            << std::endl;
}
void RiscvCodePrinter::printSext_wInst(Sext_wInst *inst) {
  std::cout << getInstName(inst) << " "
            << getOperandName(inst->getDestination()) << ","
            << getOperandName(inst->getSource(0)) << std::endl;
}
void RiscvCodePrinter::printCallInst(CallInst *inst) {
  std::cout << getInstName(inst) << " ";
  auto function = dynamic_cast<Function *>(inst->getLabel());
  if (function->isExternalFucntion()) {
    std::cout << inst->getLabel()->getName() << "@plt" << std::endl;
  } else {
    std::cout << inst->getLabel()->getName() << std::endl;
  }
}
void RiscvCodePrinter::printInst(Instruction *inst) {
  auto rInst = dynamic_cast<RInst *>(inst);
  auto iInst = dynamic_cast<IInst *>(inst);
  auto uInst = dynamic_cast<UInst *>(inst);
  auto sInst = dynamic_cast<SInst *>(inst);
  auto bInst = dynamic_cast<BInst *>(inst);
  auto jInst = dynamic_cast<JInst *>(inst);
  auto i2FInst = dynamic_cast<I2FInst *>(inst);
  auto f2IInst = dynamic_cast<F2IInst *>(inst);
  auto fRInst = dynamic_cast<FRInst *>(inst);
  auto fr2IInst = dynamic_cast<FR2IInst *>(inst);
  auto fLongInst = dynamic_cast<FLongInst *>(inst);
  auto fLinst = dynamic_cast<FLInst *>(inst);
  auto fSInst = dynamic_cast<FSInst *>(inst);
  auto lLaInst = dynamic_cast<LlaInst *>(inst);
  auto liInst = dynamic_cast<LiInst *>(inst);
  auto setx_wInst = dynamic_cast<Sext_wInst *>(inst);
  auto callInst = dynamic_cast<CallInst *>(inst);

  if (rInst != nullptr) {
    printRInst(rInst);
  } else if (iInst != nullptr) {
    printIInst(iInst);
  } else if (uInst != nullptr) {
    printUInst(uInst);
  } else if (sInst != nullptr) {
    printSInst(sInst);
  } else if (bInst != nullptr) {
    printBInst(bInst);
  } else if (jInst != nullptr) {
    printJInst(jInst);
  } else if (i2FInst != nullptr) {
    printI2FInst(i2FInst);
  } else if (f2IInst != nullptr) {
    printF2IInst(f2IInst);
  } else if (fRInst != nullptr) {
    printFRInst(fRInst);
  } else if (fr2IInst != nullptr) {
    printFR2IInst(fr2IInst);
  } else if (fLongInst != nullptr) {
    printFLongInst(fLongInst);
  } else if (fLinst != nullptr) {
    printFLInst(fLinst);
  } else if (fSInst != nullptr) {
    printFSInst(fSInst);
  } else if (lLaInst != nullptr) {
    printLLaInst(lLaInst);
  } else if (liInst != nullptr) {
    printLiInst(liInst);
  } else if (setx_wInst != nullptr) {
    printSext_wInst(setx_wInst);
  } else if (callInst != nullptr) {
    printCallInst(callInst);
  } else {
    assert(false);
  }
}
void RiscvCodePrinter::printFunction(Function *function) {
  std::cout << "  "
            << ".align 1" << std::endl;
  std::cout << "  "
            << ".global " << function->getName() << std::endl;
  std::cout << "  "
            << ".type " << function->getName() << ", "
            << "@function" << std::endl;
  std::cout << function->getName() << ": " << std::endl;
  for (const auto &block : function->getBlocks()) {
    if (!block->getName().empty()) {
      std::cout << block->getName() << ":" << std::endl;
    }
    for (const auto &inst : block->getInstructions()) {
      std::cout << "  ";
      printInst(inst.get());
    }
  }
  std::cout << "  "
            << ".size " << function->getName() << ", "
            << ".-" << function->getName() << std::endl;
}
void RiscvCodePrinter::printGlobal(Module *pModule) {
  std::list<Global *> dataSection;
  std::list<Global *> rodataSection;
  std::list<Global *> bssSection;
  for (const auto &globalItem : pModule->getGlobals()) {
    if (globalItem.first[0] == '.') {
      rodataSection.push_back(globalItem.second.get());
    } else {
      auto &init = globalItem.second->getInit();
      auto numbers = init.getNumbers();
      if (numbers.size() == 1 &&
          ((globalItem.second->isIntType() && init.getValue(0).intValue == 0) ||
           (!globalItem.second->isIntType() &&
            init.getValue(0).floatValue == 0.0))) {
        bssSection.push_back(globalItem.second.get());
      } else {
        dataSection.push_back(globalItem.second.get());
      }
    }
  }
  std::cout << "  "
            << ".section .data" << std::endl;
  for (const auto &global : dataSection) {
    unsigned size = 1;
    for (const auto &dim : global->getDims()) {
      size *= dim;
    }
    std::cout << "  "
              << ".global " << global->getName() << std::endl;
    std::cout << "  "
              << ".align  2" << std::endl;
    std::cout << "  "
              << ".type " << global->getName() << ", @object" << std::endl;
    std::cout << "  "
              << ".size " << global->getName() << ", " << 4 * size << std::endl;
    std::cout << global->getName() << ":" << std::endl;

    auto &init = global->getInit();
    auto values = init.getValues();
    auto numbers = init.getNumbers();
    if (global->isIntType()) {
      for (unsigned i = 0; i < numbers.size(); i++) {
        if (values[i].intValue == 0) {
          std::cout << "  "
                    << ".zero " << 4 * numbers[i] << std::endl;
        } else {
          for (unsigned j = 0; j < numbers[i]; j++) {
            std::cout << "  "
                      << ".word " << values[i].intValue << std::endl;
          }
        }
      }
    } else {
      for (unsigned i = 0; i < numbers.size(); i++) {
        for (unsigned j = 0; j < numbers[i]; j++) {
          auto floatbit = reinterpret_cast<unsigned *>(&(values[i].floatValue));
          std::cout << "  "
                    << ".word " << *floatbit << std::endl;
        }
      }
    }
  }

  std::cout << "  "
            << ".section .bss" << std::endl;
  for (const auto &global : bssSection) {
    unsigned size = 1;
    for (const auto &dim : global->getDims()) {
      size *= dim;
    }
    std::cout << "  "
              << ".global " << global->getName() << std::endl;
    std::cout << "  "
              << ".align  2" << std::endl;
    std::cout << "  "
              << ".type " << global->getName() << ", @object" << std::endl;
    std::cout << "  "
              << ".size " << global->getName() << ", " << 4 * size << std::endl;
    std::cout << global->getName() << ":" << std::endl;
    std::cout << "  "
              << ".zero " << 4 * size << std::endl;
  }

  std::cout << "  "
            << ".section .rodata" << std::endl;
  for (const auto &global : rodataSection) {
    std::cout << "  "
              << ".align  2" << std::endl;
    std::cout << global->getName() << ":" << std::endl;

    auto &init = global->getInit();
    auto values = init.getValues();
    auto numbers = init.getNumbers();
    if (global->isIntType()) {
      for (unsigned i = 0; i < numbers.size(); i++) {
        if (values[i].intValue == 0) {
          std::cout << "  "
                    << ".zero " << 4 * numbers[i] << std::endl;
        } else {
          for (unsigned j = 0; j < numbers[i]; j++) {
            std::cout << "  "
                      << ".word " << values[i].intValue << std::endl;
          }
        }
      }
    } else {
      for (unsigned i = 0; i < numbers.size(); i++) {
        for (unsigned j = 0; j < numbers[i]; j++) {
          auto floatbit = reinterpret_cast<unsigned *>(&(values[i].floatValue));
          std::cout << "  "
                    << ".word " << *floatbit << std::endl;
        }
      }
    }
  }
}
void RiscvCodePrinter::printRiscv(const std::string &fileName,
                                  Module *pModule) {
  std::streambuf *oldCoutBuf;
  std::ofstream fon;

  if (!fileName.empty()) {
    oldCoutBuf = std::cout.rdbuf();
    fon.open(fileName);

    if (!fon.is_open()) {
      assert(false);
    }

    std::cout.rdbuf(fon.rdbuf());
  }

  std::cout << "  "
            << ".file " << '"' << fileName << '"' << std::endl;
  std::cout << "  "
            << ".option "
            << "pic" << std::endl;
  printGlobal(pModule);

  if (pModule->getGlobals().find("functionCacheArray(0)") !=
      pModule->getGlobals().end()) {
    std::cout << libFunction << std::endl;
  }

  std::cout << "  "
            << ".text" << std::endl;
  for (const auto &functionItem : pModule->getFunctions()) {
    if (!functionItem.second->isExternalFucntion()) {
      printFunction(functionItem.second.get());
    }
  }

  if (!fileName.empty()) {
    std::cout.rdbuf(oldCoutBuf);
    fon.close();
  }
}
} // namespace riscv
