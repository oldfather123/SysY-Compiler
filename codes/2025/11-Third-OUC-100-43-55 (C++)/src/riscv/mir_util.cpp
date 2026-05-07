#include "mir_util.h"

namespace riscv {

MachineInst *loadImmI(MachineBlock *block, int imm) {
  auto inst = block->pushMInst(std::make_unique<LI>(MAKE_I32, imm));
  return inst;
}

MachineInst *loadImmF(MachineBlock *block, int imm) {
  auto tmp = block->pushMInst(std::make_unique<LI>(MAKE_I32, imm));
  auto inst = block->pushMInst(std::make_unique<RR>(RROp::MV, MAKE_F32, tmp));
  return inst;
}

MachineInst *loadImmI(MachineBlock *block, float imm) {
  return loadImmI(block, *reinterpret_cast<int *>(&imm));
}

MachineInst *loadImmF(MachineBlock *block, float imm) {
  return loadImmF(block, *reinterpret_cast<int *>(&imm));
}

int bitcount(int x) { return static_cast<int>(std::bitset<32>(x).count()); }

int leadingZeros(int x) {
  constexpr int bitSize = std::numeric_limits<int>::digits + 1;
  if (x == 0) {
    return bitSize;
  }
  std::bitset<bitSize> bits(x);
  int count = 0;
  for (int i = bitSize - 1; i >= 0; --i) {
    if (bits[i] == 0) {
      count++;
    } else {
      break;
    }
  }
  return count;
}

int trailingZeros(int x) {
  constexpr int bitSize = std::numeric_limits<int>::digits + 1;
  if (x == 0) {
    return bitSize;
  }
  std::bitset<bitSize> bits(x);
  int count = 0;
  for (int i = 0; i < bitSize; ++i) {
    if (bits[i] == 0) {
      count++;
    } else {
      break;
    }
  }
  return count;
}

MachineInst *addRegRegI(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1) {
  auto inst =
      block->pushMInst(std::make_unique<RRR>(RRROp::ADD, MAKE_I32, src0, src1));
  return inst;
}

MachineInst *addRegRegF(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1) {
  auto inst =
      block->pushMInst(std::make_unique<RRR>(RRROp::ADD, MAKE_F32, src0, src1));
  return inst;
}

MachineInst *addRegImmF(MachineBlock *block, MachineInst *src, float imm) {
  auto inst = loadImmF(block, imm);
  return addRegRegF(block, src, inst);
}

MachineInst *addRegImmI(MachineBlock *block, MachineInst *src, int imm) {
  if (std::abs(imm) < 2048) {
    auto inst = block->pushMInst(
        std::make_unique<RRI>(RRIOp::ADDI, MAKE_I32, src, imm));
    return inst;
  }
  auto inst = loadImmI(block, imm);
  return addRegRegI(block, src, inst);
}

MachineInst *divRegRegF(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1) {
  return block->pushMInst(
      std::make_unique<RRR>(RRROp::DIV, MAKE_F32, src0, src1));
}

MachineInst *divRegRegI(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1) {
  return block->pushMInst(
      std::make_unique<RRR>(RRROp::DIVW, MAKE_I32, src0, src1));
}

MachineInst *divImmRegF(MachineBlock *block, float imm, MachineInst *src) {
  auto inst = loadImmF(block, imm);
  return divRegRegF(block, inst, src);
}

MachineInst *divImmRegI(MachineBlock *block, int imm, MachineInst *src) {
  auto inst = loadImmI(block, imm);
  return divRegRegI(block, inst, src);
}

MachineInst *divRegImmF(MachineBlock *block, MachineInst *src, float imm) {
  auto inst = loadImmF(block, imm);
  return divRegRegF(block, src, inst);
}

MachineInst *divRegImmI(MachineBlock *block, MachineInst *src, int imm) {
  if (imm == 1) {
    return src; // No need to divide by 1
  }
  if (imm == -1) {
    // Use NEG
    auto negInst =
        block->pushMInst(std::make_unique<RR>(RROp::NEG, MAKE_F32, src));
    return negInst;
  }
  auto immInst = loadImmI(block, imm);
  return divRegRegI(block, src, immInst);
  // int div = imm;
  // bool isPos = true;
  // if (div < 0) {
  //   isPos = false;
  //   div = -div;
  // }
  // int shift = 0;
  // while (1L << (shift + 32) <= (0x7fffffffL - 0x80000000L % div) *
  //                                  (div - (1L << (shift + 32)) % div)) {
  //   shift++;
  // }
  // int magic = static_cast<int>((1L << (shift + 32)) / div + 1);
  // auto tmp1 = loadImmI(block, magic);
  // MachineInst *mid1, *mid2;
  // if (magic >= 0) {
  //   auto tmp2 = block->pushMInst(
  //       std::make_unique<RRR>(RRROp::MUL, MAKE_I32, src, tmp1));
  //   auto tmp3 = block->pushMInst(
  //       std::make_unique<RRI>(RRIOp::SRLI, MAKE_I32, tmp2, 32));
  //   mid1 = tmp3;
  // } else {
  //   auto tmp2 = block->pushMInst(
  //       std::make_unique<RRR>(RRROp::MUL, MAKE_I32, src, tmp1));
  //   auto tmp3 = block->pushMInst(
  //       std::make_unique<RRI>(RRIOp::SRLI, MAKE_I32, tmp2, 32));
  //   auto tmp4 = block->pushMInst(
  //       std::make_unique<RRR>(RRROp::ADD, MAKE_I32, tmp3, src));
  //   mid1 = tmp4;
  // }
  // if (shift != 0) {
  //   auto tmp2 = block->pushMInst(
  //       std::make_unique<RRI>(RRIOp::SRAIW, MAKE_I32, mid1, shift));
  //   mid2 = tmp2;
  // } else {
  //   mid2 = mid1; // No shift needed
  // }
  // if (isPos) {
  //   auto tmp2 = block->pushMInst(
  //       std::make_unique<RRI>(RRIOp::SRLIW, MAKE_I32, src, 31));
  //   return block->pushMInst(
  //       std::make_unique<RRR>(RRROp::ADDW, MAKE_I32, mid2, tmp2));
  // }
  // auto tmp2 =
  //     block->pushMInst(std::make_unique<RRI>(RRIOp::SRAIW, MAKE_I32, src,
  //     31));
  // return block->pushMInst(
  //     std::make_unique<RRR>(RRROp::SUBW, MAKE_I32, mid2, tmp2));
}

MachineInst *modRegReg(MachineBlock *block, MachineInst *src0,
                       MachineInst *src1) {
  return block->pushMInst(
      std::make_unique<RRR>(RRROp::REMW, MAKE_I32, src0, src1));
}

MachineInst *modImmReg(MachineBlock *block, int imm, MachineInst *src) {
  auto midInst = loadImmI(block, imm);
  return modRegReg(block, midInst, src);
}

MachineInst *modRegImm(MachineBlock *block, MachineInst *src, int imm) {
  if (imm == 1)
    return block->pushMInst(std::make_unique<RR>(RROp::MV, MAKE_I32, src));
  else if (bitcount(imm) == 1) {
    MachineInst *mid;
    if (imm == 2)
      mid = block->pushMInst(
          std::make_unique<RRI>(RRIOp::SRLIW, MAKE_I32, src, 31));
    else {
      mid = block->pushMInst(
          std::make_unique<RRI>(RRIOp::SRAIW, MAKE_I32, src, 31));
      mid = block->pushMInst(std::make_unique<RRI>(RRIOp::SRLIW, MAKE_I32, mid,
                                                   leadingZeros(imm) + 1));
    }
    mid =
        block->pushMInst(std::make_unique<RRR>(RRROp::ADD, MAKE_I32, mid, src));

    if (std::abs(imm) < 2048)
      mid = block->pushMInst(
          std::make_unique<RRI>(RRIOp::ANDI, MAKE_I32, mid, -imm));
    else {
      auto inst = loadImmI(block, -imm);
      mid = block->pushMInst(
          std::make_unique<RRR>(RRROp::AND, MAKE_I32, mid, inst));
    }
    return block->pushMInst(
        std::make_unique<RRR>(RRROp::SUBW, MAKE_I32, src, mid));
  }

  auto mid1 = divRegImmI(block, src, imm);
  auto mid2 = mulRegImmI(block, mid1, imm);
  return block->pushMInst(
      std::make_unique<RRR>(RRROp::SUBW, MAKE_I32, src, mid2));
}

MachineInst *mulRegRegF(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1) {
  return (block->pushMInst(
      std::make_unique<RRR>(RRROp::MUL, MAKE_F32, src0, src1)));
}

MachineInst *mulRegRegI(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1) {
  return (block->pushMInst(
      std::make_unique<RRR>(RRROp::MULW, MAKE_I32, src0, src1)));
}

MachineInst *mulRegImmF(MachineBlock *block, MachineInst *src, float imm) {
  auto midInst = loadImmF(block, imm);
  return mulRegRegF(block, src, midInst);
}

MachineInst *mulRegImmI(MachineBlock *block, MachineInst *src, int imm) {
  if (imm == 0)
    return loadImmI(block, 0);
  else if (imm == 1)
    return block->pushMInst(std::make_unique<RR>(RROp::MV, MAKE_I32, src));
  else if (imm == -1)
    return block->pushMInst(std::make_unique<RR>(RROp::NEG, MAKE_I32, src));
  else if (bitcount(imm) == 1)
    return block->pushMInst(
        std::make_unique<RRI>(RRIOp::SLLIW, MAKE_I32, src, trailingZeros(imm)));
  else if (bitcount(imm) == 2 && imm % 2 == 1) {
    auto midInst = block->pushMInst(std::make_unique<RRI>(
        RRIOp::SLLIW, MAKE_I32, src, 31 - leadingZeros(imm)));
    return block->pushMInst(
        std::make_unique<RRR>(RRROp::ADDW, MAKE_I32, (midInst), src));
  } else if (trailingZeros(imm) == 0 &&
             leadingZeros(imm) + bitcount(imm) == 32) {
    auto midInst = block->pushMInst(std::make_unique<RRI>(
        RRIOp::SLLIW, MAKE_I32, src, 32 - leadingZeros(imm)));
    return block->pushMInst(
        std::make_unique<RRR>(RRROp::SUBW, MAKE_I32, (midInst), src));
  } else {
    auto midInst = loadImmI(block, imm);
    return mulRegRegI(block, src, midInst);
  }
}

MachineInst *subRegRegF(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1) {
  return block->pushMInst(
      std::make_unique<RRR>(RRROp::SUB, MAKE_F32, src0, src1));
}

MachineInst *subRegRegI(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1) {
  return (block->pushMInst(
      std::make_unique<RRR>(RRROp::SUBW, MAKE_I32, src0, src1)));
}

MachineInst *subImmRegF(MachineBlock *block, float imm, MachineInst *src) {
  auto midInst = loadImmF(block, imm);
  return subRegRegF(block, midInst, src);
}
MachineInst *subImmRegI(MachineBlock *block, int imm, MachineInst *src) {
  auto midInst = loadImmI(block, imm);
  return subRegRegI(block, midInst, src);
}
MachineInst *subRegImmF(MachineBlock *block, MachineInst *src, float imm) {
  auto midInst = loadImmF(block, imm);
  return subRegRegF(block, src, midInst);
}
MachineInst *subRegImmI(MachineBlock *block, MachineInst *src, int imm) {
  if (-imm >= -2048 && -imm < 2048)
    return block->pushMInst(
        std::make_unique<RRI>(RRIOp::ADDI, MAKE_I32, src, -imm));

  auto midInst = loadImmI(block, imm);
  return subRegRegI(block, src, midInst);
}

MachineInst *binImmReg(MachineBlock *block, ir::BinaryInst *binInst,
                       ir::ConstantNumber *imm, MachineInst *src) {
  switch (binInst->getOp()) {
  case ir::BinaryOp::ADD:
    return addRegImmI(block, src, imm->intValue());
  case ir::BinaryOp::FADD:
    return addRegImmF(block, src, imm->floatValue());
  case ir::BinaryOp::SDIV:
    return divImmRegI(block, imm->intValue(), src);
  case ir::BinaryOp::FDIV:
    return divImmRegF(block, imm->floatValue(), src);
  case ir::BinaryOp::SREM:
    return modImmReg(block, imm->intValue(), src);
  case ir::BinaryOp::MUL:
    return mulRegImmI(block, src, imm->intValue());
  case ir::BinaryOp::FMUL:
    return mulRegImmF(block, src, imm->floatValue());
  case ir::BinaryOp::SUB:
    return subImmRegI(block, imm->intValue(), src);
  case ir::BinaryOp::FSUB:
    return subImmRegF(block, imm->floatValue(), src);
  default:
    throw std::runtime_error("Unsupported binary operator in riscv::binImmReg");
  }
}
MachineInst *binRegImm(MachineBlock *block, ir::BinaryInst *binInst,
                       MachineInst *src, ir::ConstantNumber *imm) {
  switch (binInst->getOp()) {
  case ir::BinaryOp::ADD:
    return addRegImmI(block, src, imm->intValue());
  case ir::BinaryOp::FADD:
    return addRegImmF(block, src, imm->floatValue());
  case ir::BinaryOp::SDIV:
    return divRegImmI(block, src, imm->intValue());
  case ir::BinaryOp::FDIV:
    return divRegImmF(block, src, imm->floatValue());
  case ir::BinaryOp::SREM:
    return modRegImm(block, src, imm->intValue());
  case ir::BinaryOp::MUL:
    return mulRegImmI(block, src, imm->intValue());
  case ir::BinaryOp::FMUL:
    return mulRegImmF(block, src, imm->floatValue());
  case ir::BinaryOp::SUB:
    return subRegImmI(block, src, imm->intValue());
  case ir::BinaryOp::FSUB:
    return subRegImmF(block, src, imm->floatValue());
  case ir::BinaryOp::XOR: {
    auto inst = loadImmI(block, imm->intValue());
    return block->pushMInst(
        std::make_unique<RRR>(RRROp::XOR, MAKE_I32, src, inst));
  }
  default:
    throw std::runtime_error("Unsupported binary operator in riscv::binRegImm");
  }
}
MachineInst *binRegReg(MachineBlock *block, ir::BinaryInst *binInst,
                       MachineInst *src0, MachineInst *src1) {
  switch (binInst->getOp()) {
  case ir::BinaryOp::ADD:
    return addRegRegI(block, src0, src1);
  case ir::BinaryOp::FADD:
    return addRegRegF(block, src0, src1);
  case ir::BinaryOp::SDIV:
    return divRegRegI(block, src0, src1);
  case ir::BinaryOp::FDIV:
    return divRegRegF(block, src0, src1);
  case ir::BinaryOp::SREM:
    return modRegReg(block, src0, src1);
  case ir::BinaryOp::MUL:
    return mulRegRegI(block, src0, src1);
  case ir::BinaryOp::FMUL:
    return mulRegRegF(block, src0, src1);
  case ir::BinaryOp::SUB:
    return subRegRegI(block, src0, src1);
  case ir::BinaryOp::FSUB:
    return subRegRegF(block, src0, src1);
  default:
    throw std::runtime_error("Unsupported binary operator in riscv::binRegReg");
  }
}
} // namespace riscv
