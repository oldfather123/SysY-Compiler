#include "../include/backend/MacroExpansion.h"

/**
 * @file MacroExpansion.cpp
 *
 * @brief 定义宏扩展的源文件
 *
 */

#include <sstream>

namespace macroExpansion {
auto MacroExpansion::getParamPhyReg(bool isInt, unsigned index) -> riscv::PhyRegister * {
  if (isInt) {
    switch (index) {
      case 0:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
      case 1:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a1);
      case 2:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a2);
      case 3:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a3);
      case 4:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a4);
      case 5:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a5);
      case 6:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a6);
      case 7:
        return riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a7);
      default:
        assert(false);
    }
  } else {
    switch (index) {
      case 0:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa0);
      case 1:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa1);
      case 2:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa2);
      case 3:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa3);
      case 4:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa4);
      case 5:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa5);
      case 6:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa6);
      case 7:
        return riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa7);
      default:
        assert(false);
    }
  }
}
auto MacroExpansion::getTmpRegister(bool isInt, bool is64) -> riscv::SymRegister * {
  std::stringstream ss;
  riscv::SymRegister *result;
  ss << ".tmp%" << tmpIndex;
  if (isInt) {
    result = new riscv::IntSymRegister(is64, ss.str());
  } else {
    result = new riscv::FloatSymRegister(ss.str());
  }
  symbolRegisterList.emplace_back(result);
  tmpIndex++;
  return result;
}
auto MacroExpansion::getOrEmplaceBasicBlock(riscv::Function *parent, sysy::BasicBlock *block) -> riscv::BasicBlock * {
  riscv::BasicBlock *result;
  auto iter = basicBlockMap.find(block);
  if (iter != basicBlockMap.end()) {
    result = iter->second;
  } else {
    result = parent->addBasicBlock();
    basicBlockMap.emplace(block, result);
  }
  return result;
}
void MacroExpansion::Li32(riscv::IntRegister *rd, int val) {
  auto uval = static_cast<unsigned>(val);
  builder.createLiInst(rd, static_cast<uint64_t>(uval));
}
void MacroExpansion::Li64(riscv::IntRegister *rd, int val) {
  auto val64 = static_cast<int64_t>(val);
  builder.createLiInst(rd, static_cast<uint64_t>(val64));
}
auto MacroExpansion::getOrEmplaceSymRegister(sysy::User *variable, mid2end::StackTable *stackTable)
    -> riscv::SymRegister * {
  auto iter = variableMap.find(variable);
  riscv::SymRegister *result;
  if (iter != variableMap.end()) {
    result = iter->second;
  } else {
    auto global = dynamic_cast<sysy::GlobalValue *>(variable);
    auto constVar = dynamic_cast<sysy::ConstantVariable *>(variable);
    auto local = dynamic_cast<sysy::AllocaInst *>(variable);
    auto la = dynamic_cast<sysy::LaInst *>(variable);
    if (global != nullptr || (constVar != nullptr && constVar->getNumDims() != 0)) {
      result = getTmpRegister(true, true);
      builder.createLlaInst(dynamic_cast<riscv::IntSymRegister *>(result), pModule->getGlobal(variable->getName()));
    } else if (local != nullptr && local->getNumDims() != 0) {
      result = getTmpRegister(true, true);
      auto fp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::fp);
      auto imm = stackTable->getLocalArrayAddr(localArrayMap.find(local)->second.get());
      if (imm > 2048) {
        builder.createLiInst(dynamic_cast<riscv::IntSymRegister *>(result), imm);
        builder.createSubInst(dynamic_cast<riscv::IntSymRegister *>(result), fp,
                              dynamic_cast<riscv::IntSymRegister *>(result));
      } else {
        builder.createAddiInst(dynamic_cast<riscv::IntSymRegister *>(result), fp, -imm);
      }
    } else if (la != nullptr) {
      result = new riscv::IntSymRegister(true, la->getName());
      variableMap.emplace(variable, result);
      symbolRegisterList.emplace_back(result);
    } else if (variable == nullptr) {
      assert(false);
    } else if (variable->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) ||
               variable->getType() == sysy::Type::getIntType()) {
      result = new riscv::IntSymRegister(false, variable->getName());
      variableMap.emplace(variable, result);
      symbolRegisterList.emplace_back(result);
    } else {
      result = new riscv::FloatSymRegister(variable->getName());
      variableMap.emplace(variable, result);
      symbolRegisterList.emplace_back(result);
    }
  }
  return result;
}
auto MacroExpansion::getOrEmplaceFloatConstantReg(sysy::ConstantValue *floatConstant) -> riscv::FloatSymRegister * {
  auto iter = constantMap.find(floatConstant);
  auto addr = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, true));
  auto result = dynamic_cast<riscv::FloatSymRegister *>(getTmpRegister(false));
  if (iter == constantMap.end()) {
    ConstantCounter init;
    init.push_back(floatConstant->getFloat());
    std::stringstream ss;
    ss << ".F" << floatGlobalIndex;
    auto global = pModule->createGlobal(false, std::vector<unsigned>{}, init, ss.str());
    constantMap.emplace(floatConstant, global);
    builder.createLlaInst(addr, global);
    floatGlobalIndex++;
  } else {
    builder.createLlaInst(addr, iter->second);
  }
  builder.createFlwInst(result, addr, 0);
  return result;
}
void MacroExpansion::interpreteAdd(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto addInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = addInst->getLhs();
  auto rhs = addInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(addInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    auto imm = constLhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li32(rd, imm);
      builder.createAddwInst(rd, rd, rs2);
    } else {
      builder.createAddiwInst(rd, rs2, imm);
    }
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto imm = constRhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li32(rd, imm);
      builder.createAddwInst(rd, rs1, rd);
    } else {
      builder.createAddiwInst(rd, rs1, imm);
    }
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createAddwInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteSub(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto subInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = subInst->getLhs();
  auto rhs = subInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(subInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    auto imm = constLhs->getInt();
    Li32(rd, imm);
    builder.createSubwInst(rd, rd, rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto imm = constRhs->getInt();
    Li32(rd, imm);
    builder.createSubwInst(rd, rs1, rd);
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSubwInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteMul(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto mulInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = mulInst->getLhs();
  auto rhs = mulInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(mulInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    auto imm = constLhs->getInt();
    Li32(rd, imm);
    builder.createMulwInst(rd, rd, rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto imm = constRhs->getInt();
    Li32(rd, imm);
    builder.createMulwInst(rd, rs1, rd);
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createMulwInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteDiv(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto divInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = divInst->getLhs();
  auto rhs = divInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(divInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    auto imm = constLhs->getInt();
    Li32(rd, imm);
    builder.createDivwInst(rd, rd, rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto imm = constRhs->getInt();
    Li32(rd, imm);
    builder.createDivwInst(rd, rs1, rd);
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createDivwInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteRem(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto remInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = remInst->getLhs();
  auto rhs = remInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(remInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    auto imm = constLhs->getInt();
    Li32(rd, imm);
    builder.createRemwInst(rd, rd, rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto imm = constRhs->getInt();
    Li32(rd, imm);
    builder.createRemwInst(rd, rs1, rd);
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createRemwInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteAnd(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto andInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = andInst->getLhs();
  auto rhs = andInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(andInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    auto imm = constLhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li32(rd, imm);
      builder.createAndInst(rd, rd, rs2);
    } else {
      builder.createAndiInst(rd, rs2, imm);
    }
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto imm = constRhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li32(rd, imm);
      builder.createAndInst(rd, rd, rs1);
    } else {
      builder.createAndiInst(rd, rs1, imm);
    }
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createAndInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteOr(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto orInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = orInst->getLhs();
  auto rhs = orInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(orInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    auto imm = constLhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li32(rd, imm);
      builder.createOrInst(rd, rd, rs2);
    } else {
      builder.createOriInst(rd, rs2, imm);
    }
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto imm = constRhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li32(rd, imm);
      builder.createOrInst(rd, rd, rs1);
    } else {
      builder.createOriInst(rd, rs1, imm);
    }
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createOrInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteICmpEQ(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto iCmpEQInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = iCmpEQInst->getLhs();
  auto rhs = iCmpEQInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(iCmpEQInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs2, rs2);
    auto imm = constLhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li64(rd, imm);
      builder.createXorInst(rd, rd, rs2);
    } else {
      builder.createXoriInst(rd, rs2, imm);
    }
    builder.createSltiuInst(rd, rd, 1);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    auto imm = constRhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li64(rd, imm);
      builder.createXorInst(rd, rs1, rd);
    } else {
      builder.createXoriInst(rd, rs1, imm);
    }
    builder.createSltiuInst(rd, rd, 1);
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    builder.createSext_wInst(rs2, rs2);
    builder.createXorInst(rd, rs1, rs2);
    builder.createSltiuInst(rd, rd, 1);
  }
}
void MacroExpansion::interpreteICmpNE(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::zero);
  auto iCmpNEInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = iCmpNEInst->getLhs();
  auto rhs = iCmpNEInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(iCmpNEInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs2, rs2);
    auto imm = constLhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li64(rd, imm);
      builder.createXorInst(rd, rd, rs2);
    } else {
      builder.createXoriInst(rd, rs2, imm);
    }
    builder.createSltuInst(rd, zero, rd);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    auto imm = constRhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li64(rd, imm);
      builder.createXorInst(rd, rs1, rd);
    } else {
      builder.createXoriInst(rd, rs1, imm);
    }
    builder.createSltuInst(rd, zero, rd);
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    builder.createSext_wInst(rs2, rs2);
    builder.createSubInst(rd, rs1, rs2);
    builder.createSltuInst(rd, zero, rd);
  }
}
void MacroExpansion::interpreteICmpLT(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto iCmpLTInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = iCmpLTInst->getLhs();
  auto rhs = iCmpLTInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(iCmpLTInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs2, rs2);
    auto imm = constLhs->getInt();
    Li64(rd, imm);
    builder.createSltInst(rd, rd, rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    auto imm = constRhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li64(rd, imm);
      builder.createSltInst(rd, rs1, rd);
    } else {
      builder.createSltiInst(rd, rs1, imm);
    }
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    builder.createSext_wInst(rs2, rs2);
    builder.createSltInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteICmpGT(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto iCmpGTInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = iCmpGTInst->getLhs();
  auto rhs = iCmpGTInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(iCmpGTInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs2, rs2);
    auto imm = constLhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li64(rd, imm);
      builder.createSltInst(rd, rs2, rd);
    } else {
      builder.createSltiInst(rd, rs2, imm);
    }
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    auto imm = constRhs->getInt();
    Li64(rd, imm);
    builder.createSltInst(rd, rd, rs1);
  } else {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    builder.createSext_wInst(rs2, rs2);
    builder.createSltInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteICmpLE(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto iCmpLEInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = iCmpLEInst->getLhs();
  auto rhs = iCmpLEInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(iCmpLEInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs2, rs2);
    auto imm = constLhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li64(rd, imm);
      builder.createSltInst(rd, rs2, rd);
    } else {
      builder.createSltiInst(rd, rs2, imm);
    }
    builder.createXoriInst(rd, rd, 1);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    auto imm = constRhs->getInt();
    Li64(rd, imm);
    builder.createSltInst(rd, rd, rs1);
    builder.createXoriInst(rd, rd, 1);
  } else {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    builder.createSext_wInst(rs2, rs2);
    builder.createSltInst(rd, rs1, rs2);
    builder.createXoriInst(rd, rd, 1);
  }
}
void MacroExpansion::interpreteICmpGE(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto iCmpGEInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = iCmpGEInst->getLhs();
  auto rhs = iCmpGEInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(iCmpGEInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs2, rs2);
    auto imm = constLhs->getInt();
    Li64(rd, imm);
    builder.createSltInst(rd, rd, rs2);
    builder.createXoriInst(rd, rd, 1);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    auto imm = constRhs->getInt();
    if (imm < -2048 || imm > 2047) {
      Li64(rd, imm);
      builder.createSltInst(rd, rs1, rd);
    } else {
      builder.createSltiInst(rd, rs1, imm);
    }
    builder.createXoriInst(rd, rd, 1);
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createSext_wInst(rs1, rs1);
    builder.createSext_wInst(rs2, rs2);
    builder.createSltInst(rd, rs1, rs2);
    builder.createXoriInst(rd, rd, 1);
  }
}
void MacroExpansion::interpreteFAdd(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fAddInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = fAddInst->getLhs();
  auto rhs = fAddInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(fAddInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFadd_sInst(rd, getOrEmplaceFloatConstantReg(constLhs), rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createFadd_sInst(rd, rs1, getOrEmplaceFloatConstantReg(constRhs));
  } else {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFadd_sInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteFSub(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fSubInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = fSubInst->getLhs();
  auto rhs = fSubInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(fSubInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFsub_sInst(rd, getOrEmplaceFloatConstantReg(constLhs), rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createFsub_sInst(rd, rs1, getOrEmplaceFloatConstantReg(constRhs));
  } else {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFsub_sInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteFMul(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fMulInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = fMulInst->getLhs();
  auto rhs = fMulInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(fMulInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFmul_sInst(rd, getOrEmplaceFloatConstantReg(constLhs), rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createFmul_sInst(rd, rs1, getOrEmplaceFloatConstantReg(constRhs));
  } else {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFmul_sInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteFDiv(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fDivInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = fDivInst->getLhs();
  auto rhs = fDivInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(fDivInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFdiv_sInst(rd, getOrEmplaceFloatConstantReg(constLhs), rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createFdiv_sInst(rd, rs1, getOrEmplaceFloatConstantReg(constRhs));
  } else {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFdiv_sInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteFCmpEQ(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fCmpEQInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = fCmpEQInst->getLhs();
  auto rhs = fCmpEQInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(fCmpEQInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFeq_sInst(rd, getOrEmplaceFloatConstantReg(constLhs), rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createFeq_sInst(rd, rs1, getOrEmplaceFloatConstantReg(constRhs));
  } else {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFeq_sInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteFCmpNE(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fCmpNEInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = fCmpNEInst->getLhs();
  auto rhs = fCmpNEInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(fCmpNEInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFeq_sInst(rd, getOrEmplaceFloatConstantReg(constLhs), rs2);
    builder.createXoriInst(rd, rd, 1);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createFeq_sInst(rd, rs1, getOrEmplaceFloatConstantReg(constRhs));
    builder.createXoriInst(rd, rd, 1);
  } else {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFeq_sInst(rd, rs1, rs2);
    builder.createXoriInst(rd, rd, 1);
  }
}
void MacroExpansion::interpreteFCmpLT(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fCmpLTInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = fCmpLTInst->getLhs();
  auto rhs = fCmpLTInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(fCmpLTInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFlt_sInst(rd, getOrEmplaceFloatConstantReg(constLhs), rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createFlt_sInst(rd, rs1, getOrEmplaceFloatConstantReg(constRhs));
  } else {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFlt_sInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteFCmpGT(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fCmpGTInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = fCmpGTInst->getLhs();
  auto rhs = fCmpGTInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(fCmpGTInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFlt_sInst(rd, rs2, getOrEmplaceFloatConstantReg(constLhs));
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createFlt_sInst(rd, getOrEmplaceFloatConstantReg(constRhs), rs1);
  } else {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFlt_sInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteFCmpLE(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fCmpLEInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = fCmpLEInst->getLhs();
  auto rhs = fCmpLEInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(fCmpLEInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFle_sInst(rd, getOrEmplaceFloatConstantReg(constLhs), rs2);
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createFle_sInst(rd, rs1, getOrEmplaceFloatConstantReg(constRhs));
  } else {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFle_sInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteFCmpGE(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fCmpGEInst = dynamic_cast<sysy::BinaryInst *>(inst);
  auto lhs = fCmpGEInst->getLhs();
  auto rhs = fCmpGEInst->getRhs();
  auto constLhs = dynamic_cast<sysy::ConstantValue *>(lhs);
  auto constRhs = dynamic_cast<sysy::ConstantValue *>(rhs);
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(fCmpGEInst, stackTable));
  if (constLhs != nullptr) {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFle_sInst(rd, rs2, getOrEmplaceFloatConstantReg(constLhs));
  } else if (constRhs != nullptr) {
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    builder.createFle_sInst(rd, getOrEmplaceFloatConstantReg(constRhs), rs1);
  } else {
    auto rs2 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(lhs), stackTable));
    auto rs1 =
        dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(rhs), stackTable));
    builder.createFle_sInst(rd, rs1, rs2);
  }
}
void MacroExpansion::interpreteNeg(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::zero);
  auto negInst = dynamic_cast<sysy::UnaryInst *>(inst);
  auto hs = negInst->getOperand();
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(negInst, stackTable));
  auto rs2 = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(hs), stackTable));
  builder.createSubwInst(rd, zero, rs2);
}
void MacroExpansion::interpreteNot(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto notInst = dynamic_cast<sysy::UnaryInst *>(inst);
  auto hs = notInst->getOperand();
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(notInst, stackTable));
  auto rs1 = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(hs), stackTable));
  builder.createSext_wInst(rs1, rs1);
  builder.createSltiuInst(rd, rs1, 1);
}
void MacroExpansion::interpreteFNeg(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto fNegInst = dynamic_cast<sysy::UnaryInst *>(inst);
  auto hs = fNegInst->getOperand();
  auto rd = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(fNegInst, stackTable));
  auto rs1 =
      dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(hs), stackTable));
  builder.createFsgnjn_sInst(rd, rs1, rs1);
}
void MacroExpansion::interpreteFNot(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::zero);
  auto fNotInst = dynamic_cast<sysy::UnaryInst *>(inst);
  auto hs = fNotInst->getOperand();
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(fNotInst, stackTable));
  auto rs1 =
      dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(hs), stackTable));
  builder.createFclass_sInst(rd, rs1);
  builder.createAndiInst(rd, rd, 3 << 3);
  builder.createSltuInst(rd, zero, rd);
}
void MacroExpansion::interpreteFtoI(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto ftoIInst = dynamic_cast<sysy::UnaryInst *>(inst);
  auto hs = ftoIInst->getOperand();
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(ftoIInst, stackTable));
  auto rs1 =
      dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(hs), stackTable));
  builder.createFcvt_w_sInst(rd, rs1);
}
void MacroExpansion::interpreteItoF(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto itoFInst = dynamic_cast<sysy::UnaryInst *>(inst);
  auto hs = itoFInst->getOperand();
  auto rd = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(itoFInst, stackTable));
  auto rs1 = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(hs), stackTable));
  builder.createFcvt_s_wInst(rd, rs1);
}
// 可能有长跳转
void MacroExpansion::interpreteCondBr(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::zero);
  auto condBrInst = dynamic_cast<sysy::CondBrInst *>(inst);
  auto thenBlock = basicBlockMap.find(condBrInst->getThenBlock())->second;
  auto elseBlock = basicBlockMap.find(condBrInst->getElseBlock())->second;
  auto curBlock = builder.getCurBlock();
  auto cond = condBrInst->getCondition();
  auto constCond = dynamic_cast<sysy::ConstantValue *>(cond);
  if (constCond != nullptr) {
    if ((constCond->isInt() && constCond->getInt() != 0) || (constCond->isFloat() && constCond->getFloat() != 0.0F)) {
      builder.createJalInst(zero, thenBlock);
      if (thenBlock != elseBlock) {
        curBlock->removeSuccessor(elseBlock);
        elseBlock->removePredecessor(curBlock);
      }
    } else {
      builder.createJalInst(zero, elseBlock);
      if (thenBlock != elseBlock) {
        curBlock->removeSuccessor(thenBlock);
        thenBlock->removePredecessor(curBlock);
      }
    }
  } else {
    auto rs1 =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(cond), stackTable));
    builder.createBne(rs1, zero, thenBlock);
    auto newBlock = curBlock->getParent()->addBasicBlock();
    builder.setPosition(newBlock);
    builder.createJalInst(zero, elseBlock);

    targetBlocks.insert(thenBlock);
    targetBlocks.insert(elseBlock);

    if (thenBlock != elseBlock) {
      curBlock->removeSuccessor(elseBlock);
      elseBlock->removePredecessor(curBlock);
    }
    curBlock->addSuccessor(newBlock);
    newBlock->addPredecessor(curBlock);
    newBlock->addSuccessor(elseBlock);
    elseBlock->addPredecessor(newBlock);
  }
}
// 可能有长跳转
void MacroExpansion::interpreteBr(sysy::Instruction *inst) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::zero);
  auto brInst = dynamic_cast<sysy::UncondBrInst *>(inst);
  auto thenBlock = basicBlockMap.find(brInst->getBlock())->second;
  builder.createJalInst(zero, thenBlock);

  targetBlocks.insert(thenBlock);
}
void MacroExpansion::interpreteLoad(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::zero);
  auto loadInst = dynamic_cast<sysy::LoadInst *>(inst);
  auto pointer = loadInst->getPointer();
  auto global = dynamic_cast<sysy::GlobalValue *>(pointer);
  auto local = dynamic_cast<sysy::AllocaInst *>(pointer);
  auto constVariable = dynamic_cast<sysy::ConstantVariable *>(pointer);
  std::vector<sysy::Value *> dims;
  if (global != nullptr) {
    dims.reserve(global->getNumDims());
    for (const auto &dim : global->getDims()) {
      dims.push_back(dim->getValue());
    }
  } else if (local != nullptr) {
    dims.reserve(local->getNumDims());
    for (const auto &dim : local->getDims()) {
      dims.push_back(dim->getValue());
    }
  } else if (constVariable != nullptr) {
    dims.reserve(constVariable->getNumDims());
    for (const auto &dim : constVariable->getDims()) {
      dims.push_back(dim->getValue());
    }
  } else {
    assert(false);
  }
  if (dims.empty()) {
    if (inst->getType() == sysy::Type::getIntType()) {
      // 获取对应值
      auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(inst, stackTable));
      if (dynamic_cast<sysy::GlobalValue *>(pointer) != nullptr) {
        auto addrReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
        builder.createLwInst(rd, addrReg, 0);
      } else {
        auto rs1 = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
        builder.createAddwInst(rd, rs1, zero);
      }
    } else {
      auto rd = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(inst, stackTable));
      if (dynamic_cast<sysy::GlobalValue *>(pointer) != nullptr) {
        auto addrReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
        builder.createFlwInst(rd, addrReg, 0);
      } else {
        auto rs1 = dynamic_cast<riscv::FloatSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
        builder.createFsgnj_sInst(rd, rs1, rs1);
      }
    }
  } else {
    auto posReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, true));
    auto dimReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, true));
    auto index = loadInst->getIndex(0);
    auto constIndex = dynamic_cast<sysy::ConstantValue *>(index);
    if (constIndex != nullptr) {
      Li64(posReg, constIndex->getInt());
    } else {
      builder.createSext_wInst(posReg, dynamic_cast<riscv::IntSymRegister *>(
                                           getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(index), stackTable)));
    }
    for (unsigned i = 1; i < dims.size(); i++) {
      auto dim = dynamic_cast<sysy::ConstantValue *>(dims[i])->getInt();
      Li64(dimReg, dim);
      builder.createMulInst(posReg, posReg, dimReg);
      index = loadInst->getIndex(i);
      constIndex = dynamic_cast<sysy::ConstantValue *>(index);
      if (constIndex != nullptr) {
        if (constIndex->getInt() < -2048 || constIndex->getInt() > 2047) {
          Li64(dimReg, constIndex->getInt());
          builder.createAddInst(posReg, posReg, dimReg);
        } else {
          builder.createAddiInst(posReg, posReg, constIndex->getInt());
        }
      } else {
        builder.createSext_wInst(dimReg, dynamic_cast<riscv::IntSymRegister *>(
                                             getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(index), stackTable)));
        builder.createAddInst(posReg, posReg, dimReg);
      }
    }
    builder.createAddiInst(dimReg, zero, 4);
    builder.createMulInst(posReg, posReg, dimReg);
    // 获取起始地址
    auto pointerReg =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
    builder.createAddInst(posReg, pointerReg, posReg);
    if (inst->getType() == sysy::Type::getIntType()) {
      // 获取对应值
      auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(inst, stackTable));
      builder.createLwInst(rd, posReg, 0);
    } else {
      // 获取对应值
      auto rd = dynamic_cast<riscv::FloatSymRegister *>(getOrEmplaceSymRegister(inst, stackTable));
      builder.createFlwInst(rd, posReg, 0);
    }
  }
}
void MacroExpansion::interpreteLa(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::zero);
  auto laInst = dynamic_cast<sysy::LaInst *>(inst);
  auto pointer = laInst->getPointer();
  auto global = dynamic_cast<sysy::GlobalValue *>(pointer);
  auto local = dynamic_cast<sysy::AllocaInst *>(pointer);
  auto constVariable = dynamic_cast<sysy::ConstantVariable *>(pointer);
  std::vector<sysy::Value *> dims;
  if (global != nullptr) {
    dims.reserve(global->getNumDims());
    for (const auto &dim : global->getDims()) {
      dims.push_back(dim->getValue());
    }
  } else if (local != nullptr) {
    dims.reserve(local->getNumDims());
    for (const auto &dim : local->getDims()) {
      dims.push_back(dim->getValue());
    }
  } else if (constVariable != nullptr) {
    dims.reserve(constVariable->getNumDims());
    for (const auto &dim : constVariable->getDims()) {
      dims.push_back(dim->getValue());
    }
  } else {
    assert(false);
  }
  auto posReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, true));
  auto dimReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, true));
  if (laInst->getNumIndices() > 0) {
    auto index = laInst->getIndex(0);
    auto constIndex = dynamic_cast<sysy::ConstantValue *>(index);
    if (constIndex != nullptr) {
      Li64(posReg, constIndex->getInt());
    } else {
      builder.createSext_wInst(posReg, dynamic_cast<riscv::IntSymRegister *>(
                                           getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(index), stackTable)));
    }
  } else {
    builder.createAddiInst(posReg, zero, 0);
  }
  for (unsigned i = 1; i < dims.size(); i++) {
    auto dim = dynamic_cast<sysy::ConstantValue *>(dims[i])->getInt();
    Li64(dimReg, dim);
    builder.createMulInst(posReg, posReg, dimReg);
    if (i < laInst->getNumIndices()) {
      auto index = laInst->getIndex(i);
      auto constIndex = dynamic_cast<sysy::ConstantValue *>(index);
      if (constIndex != nullptr) {
        if (constIndex->getInt() < -2048 || constIndex->getInt() > 2047) {
          Li64(dimReg, constIndex->getInt());
          builder.createAddInst(posReg, posReg, dimReg);
        } else {
          builder.createAddiInst(posReg, posReg, constIndex->getInt());
        }
      } else {
        builder.createSext_wInst(dimReg, dynamic_cast<riscv::IntSymRegister *>(
                                             getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(index), stackTable)));
        builder.createAddInst(posReg, posReg, dimReg);
      }
    }
  }
  builder.createAddiInst(dimReg, zero, 4);
  builder.createMulInst(posReg, posReg, dimReg);
  // 获取起始地址
  auto pointerReg =
      dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
  auto rd = dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(inst, stackTable));
  builder.createAddInst(rd, pointerReg, posReg);
}
void MacroExpansion::interpreteStore(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::zero);
  auto storeInst = dynamic_cast<sysy::StoreInst *>(inst);
  auto pointer = storeInst->getPointer();
  auto value = storeInst->getValue();
  auto constValue = dynamic_cast<sysy::ConstantValue *>(value);
  auto global = dynamic_cast<sysy::GlobalValue *>(pointer);
  auto local = dynamic_cast<sysy::AllocaInst *>(pointer);
  std::vector<sysy::Value *> dims;
  if (global != nullptr) {
    dims.reserve(global->getNumDims());
    for (const auto &dim : global->getDims()) {
      dims.push_back(dim->getValue());
    }
  } else if (local != nullptr) {
    dims.reserve(local->getNumDims());
    for (const auto &dim : local->getDims()) {
      dims.push_back(dim->getValue());
    }
  } else {
    assert(false);
  }
  if (dims.empty()) {
    if (value->getType() == sysy::Type::getIntType()) {
      // 写入对应值
      if (dynamic_cast<sysy::GlobalValue *>(pointer) != nullptr) {
        auto addrReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
        if (constValue != nullptr) {
          auto tmpReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, false));
          Li32(tmpReg, constValue->getInt());
          builder.createSwInst(tmpReg, addrReg, 0);
        } else {
          auto rs1 = dynamic_cast<riscv::IntSymRegister *>(
              getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(value), stackTable));
          builder.createSwInst(rs1, addrReg, 0);
        }
      } else {
        auto rd = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
        if (constValue != nullptr) {
          Li32(rd, constValue->getInt());
        } else {
          auto rs1 = dynamic_cast<riscv::IntSymRegister *>(
              getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(value), stackTable));
          builder.createAddiwInst(rd, rs1, 0);
        }
      }
    } else {
      // 写入对应值
      if (dynamic_cast<sysy::GlobalValue *>(pointer) != nullptr) {
        auto addrReg = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
        if (constValue != nullptr) {
          auto tmpReg = getOrEmplaceFloatConstantReg(constValue);
          builder.createFswInst(tmpReg, addrReg, 0);
        } else {
          auto rs1 = dynamic_cast<riscv::FloatSymRegister *>(
              getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(value), stackTable));
          builder.createFswInst(rs1, addrReg, 0);
        }

      } else {
        auto rd = dynamic_cast<riscv::FloatSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
        if (constValue != nullptr) {
          auto tmpReg = getOrEmplaceFloatConstantReg(constValue);
          builder.createFsgnj_sInst(rd, tmpReg, tmpReg);
        } else {
          auto rs1 = dynamic_cast<riscv::FloatSymRegister *>(
              getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(value), stackTable));
          builder.createFsgnj_sInst(rd, rs1, rs1);
        }
      }
    }
  } else {
    auto posReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, true));
    auto dimReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, true));
    auto index = storeInst->getIndex(0);
    auto constIndex = dynamic_cast<sysy::ConstantValue *>(index);
    if (constIndex != nullptr) {
      Li64(posReg, constIndex->getInt());
    } else {
      builder.createSext_wInst(posReg, dynamic_cast<riscv::IntSymRegister *>(
                                           getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(index), stackTable)));
    }
    for (unsigned i = 1; i < dims.size(); i++) {
      auto dim = dynamic_cast<sysy::ConstantValue *>(dims[i])->getInt();
      Li64(dimReg, dim);
      builder.createMulInst(posReg, posReg, dimReg);
      index = storeInst->getIndex(i);
      constIndex = dynamic_cast<sysy::ConstantValue *>(index);
      if (constIndex != nullptr) {
        if (constIndex->getInt() < -2048 || constIndex->getInt() > 2047) {
          Li64(dimReg, constIndex->getInt());
          builder.createAddInst(posReg, posReg, dimReg);
        } else {
          builder.createAddiInst(posReg, posReg, constIndex->getInt());
        }
      } else {
        builder.createSext_wInst(dimReg, dynamic_cast<riscv::IntSymRegister *>(
                                             getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(index), stackTable)));
        builder.createAddInst(posReg, posReg, dimReg);
      }
    }
    builder.createAddiInst(dimReg, zero, 4);
    builder.createMulInst(posReg, posReg, dimReg);
    // 获取起始地址
    auto pointerReg =
        dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
    builder.createAddInst(posReg, pointerReg, posReg);
    if (value->getType() == sysy::Type::getIntType()) {
      // 写入对应值
      if (constValue != nullptr) {
        auto rs1 = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true));
        Li32(rs1, constValue->getInt());
        builder.createSwInst(rs1, posReg, 0);
      } else {
        auto rs1 = dynamic_cast<riscv::IntSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(value), stackTable));
        builder.createSwInst(rs1, posReg, 0);
      }
    } else {
      // 写入对应值
      if (constValue != nullptr) {
        builder.createFswInst(getOrEmplaceFloatConstantReg(constValue), posReg, 0);
      } else {
        auto rs1 = dynamic_cast<riscv::FloatSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(value), stackTable));
        builder.createFswInst(rs1, posReg, 0);
      }
    }
  }
}
void MacroExpansion::interpreteMemset(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::zero);
  auto memsetInst = dynamic_cast<sysy::MemsetInst *>(inst);
  auto begin = dynamic_cast<sysy::ConstantValue *>(memsetInst->getBegin())->getInt();
  auto len = dynamic_cast<sysy::ConstantValue *>(memsetInst->getSize())->getInt();
  auto pointer = memsetInst->getPointer();
  auto value = memsetInst->getValue();
  auto constValue = dynamic_cast<sysy::ConstantValue *>(value);

  // 获取起始地址
  auto posReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, true));
  auto tmpReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, true));
  auto pointerReg =
      dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(pointer), stackTable));
  Li64(posReg, begin);
  builder.createAddiInst(tmpReg, zero, 4);
  builder.createMulInst(posReg, posReg, tmpReg);
  builder.createAddInst(posReg, pointerReg, posReg);
  if (value->getType() == sysy::Type::getIntType()) {
    // 写入对应值
    if (constValue != nullptr) {
      auto rs1 = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true));
      Li32(rs1, constValue->getInt());
      for (unsigned i = 0; i < static_cast<unsigned>(len); i++) {
        builder.createSwInst(rs1, posReg, 0);
        builder.createAddiInst(posReg, posReg, 4);
      }
    } else {
      auto rs1 =
          dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(value), stackTable));
      for (unsigned i = 0; i < static_cast<unsigned>(len); i++) {
        builder.createSwInst(rs1, posReg, 0);
        builder.createAddiInst(posReg, posReg, 4);
      }
    }
  } else {
    // 写入对应值
    if (constValue != nullptr) {
      auto rs1 = dynamic_cast<riscv::FloatSymRegister *>(getTmpRegister(false));
      auto tmpReg = getOrEmplaceFloatConstantReg(constValue);
      builder.createFsgnj_sInst(rs1, tmpReg, tmpReg);
      for (unsigned i = 0; i < static_cast<unsigned>(len); i++) {
        builder.createFswInst(rs1, posReg, 0);
        builder.createAddiInst(posReg, posReg, 4);
      }

    } else {
      auto rs1 = dynamic_cast<riscv::FloatSymRegister *>(
          getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(value), stackTable));
      for (unsigned i = 0; i < static_cast<unsigned>(len); i++) {
        builder.createFswInst(rs1, posReg, 0);
        builder.createAddiInst(posReg, posReg, 4);
      }
    }
  }
}
// 需要规范化传参：先压入溢出参数，再传递寄存器参数
void MacroExpansion::interpreteCall(sysy::Function *function, sysy::Instruction *inst,
                                    mid2end::StackTable *stackTable) {
  auto sp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::sp);
  auto a0 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
  auto fa0 = riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa0);
  auto callInst = dynamic_cast<sysy::CallInst *>(inst);
  auto callee = callInst->getCallee();
  auto arguments = callInst->getArguments();
  auto params = callee->getEntryBlock()->getArguments();
  auto calleeStackTable = functionStackMap.find(pModule->getFunction(callee->getName()))->second.get();
  auto callerStackTable = functionStackMap.find(pModule->getFunction(function->getName()))->second.get();

  unsigned numIntParams = 0;
  unsigned numFloatParams = 0;
  std::list<sysy::AllocaInst *> notSpillParams;
  std::list<sysy::Value *> notSpillArgs;
  std::list<sysy::AllocaInst *> spillParams;
  std::list<sysy::Value *> spillArgs;
  for (unsigned i = 0; i < params.size(); i++) {
    auto param = params[i];
    auto arg = (arguments.begin() + i)->get()->getValue();
    if (param->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) || param->getNumDims() != 0) {
      if (numIntParams < 8) {
        notSpillParams.emplace_back(param);
        notSpillArgs.emplace_back(arg);
      } else {
        spillParams.emplace_back(param);
        spillArgs.emplace_back(arg);
      }
      numIntParams++;
    } else {
      if (numFloatParams < 8) {
        notSpillParams.emplace_back(param);
        notSpillArgs.emplace_back(arg);
      } else {
        spillParams.emplace_back(param);
        spillArgs.emplace_back(arg);
      }
      numFloatParams++;
    }
  }
  if (!spillParams.empty()) {
    auto addrReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true));
    unsigned lastAddr = 0;
    builder.createAddiInst(addrReg, sp, 0);
    auto paramIter = spillParams.begin();
    auto argIter = spillArgs.begin();
    for (unsigned i = 0; i < spillParams.size(); i++) {
      auto param = *paramIter;
      auto arg = *argIter;
      auto offset =
          calleeStackTable->getSpillParamAddr(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(param), stackTable));
      auto constArg = dynamic_cast<sysy::ConstantValue *>(arg);
      builder.createAddiInst(addrReg, addrReg, offset - lastAddr);
      if (param->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) || param->getNumDims() != 0) {
        if (constArg != nullptr) {
          auto tmpReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, false));
          Li32(tmpReg, constArg->getInt());
          builder.createSwInst(tmpReg, addrReg, 0);
        } else {
          auto argumentReg = dynamic_cast<riscv::IntSymRegister *>(
              getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(arg), stackTable));
          if (param->getNumDims() != 0) {
            builder.createSdInst(argumentReg, addrReg, 0);
          } else {
            builder.createSwInst(argumentReg, addrReg, 0);
          }
        }
      } else {
        if (constArg != nullptr) {
          builder.createFswInst(getOrEmplaceFloatConstantReg(constArg), addrReg, 0);
        } else {
          auto argumentReg = dynamic_cast<riscv::FloatSymRegister *>(
              getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(arg), stackTable));
          builder.createFswInst(argumentReg, addrReg, 0);
        }
      }
      lastAddr = offset;
      paramIter++;
      argIter++;
    }
  }
  std::map<sysy::AllocaInst *, riscv::SymRegister *> paramSymMap;
  std::map<sysy::AllocaInst *, int> paramConstMap;
  auto paramIter = notSpillParams.begin();
  auto argIter = notSpillArgs.begin();
  for (unsigned i = 0; i < notSpillParams.size(); i++) {
    auto param = *paramIter;
    auto arg = *argIter;
    if (param->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) || param->getNumDims() != 0) {
      auto constArg = dynamic_cast<sysy::ConstantValue *>(arg);
      if (constArg != nullptr) {
        paramConstMap.emplace(param, constArg->getInt());
      } else {
        auto rs1 =
            dynamic_cast<riscv::IntSymRegister *>(getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(arg), stackTable));
        paramSymMap.emplace(param, rs1);
      }
    } else {
      auto constArg = dynamic_cast<sysy::ConstantValue *>(arg);
      if (constArg != nullptr) {
        auto tmpReg = getOrEmplaceFloatConstantReg(constArg);
        paramSymMap.emplace(param, tmpReg);
      } else {
        auto rs1 = dynamic_cast<riscv::FloatSymRegister *>(
            getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(arg), stackTable));
        paramSymMap.emplace(param, rs1);
      }
    }
    paramIter++;
    argIter++;
  }
  numIntParams = 0;
  numFloatParams = 0;
  paramIter = notSpillParams.begin();
  for (unsigned i = 0; i < notSpillParams.size(); i++) {
    auto param = *paramIter;
    if (param->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) || param->getNumDims() != 0) {
      auto targetReg = dynamic_cast<riscv::IntPhyRegister *>(getParamPhyReg(true, numIntParams));
      calleeStackTable->addNotSpillParamReg(targetReg);
      auto constIter = paramConstMap.find(param);
      if (constIter != paramConstMap.end()) {
        Li32(targetReg, constIter->second);
      } else {
        if (param->getNumDims() != 0) {
          builder.createAddiInst(targetReg, dynamic_cast<riscv::IntSymRegister *>(paramSymMap.at(param)), 0);
        } else {
          builder.createAddiwInst(targetReg, dynamic_cast<riscv::IntSymRegister *>(paramSymMap.at(param)), 0);
        }
      }
      numIntParams++;
    } else {
      auto targetReg = dynamic_cast<riscv::FloatPhyRegister *>(getParamPhyReg(false, numFloatParams));
      calleeStackTable->addNotSpillParamReg(targetReg);
      auto rs1 = dynamic_cast<riscv::FloatSymRegister *>(paramSymMap.at(param));
      builder.createFsgnj_sInst(targetReg, rs1, rs1);
      numFloatParams++;
    }
    paramIter++;
  }

  auto backendCallInst = builder.createCallInst(pModule->getFunction(callee->getName()));
  callerStackTable->addCalleeStackTable(backendCallInst, calleeStackTable);
  auto callSymbol = getOrEmplaceSymRegister(callInst, stackTable);
  if (callInst->getType() == sysy::Type::getIntType()) {
    builder.createAddiwInst(dynamic_cast<riscv::IntSymRegister *>(callSymbol), a0, 0);
  } else if (callInst->getType() == sysy::Type::getFloatType()) {
    builder.createFsgnj_sInst(dynamic_cast<riscv::FloatSymRegister *>(callSymbol), fa0, fa0);
  } else if (callInst->getType() == sysy::Type::getVoidType()) {
  } else {
    assert(false);
  }
}
void MacroExpansion::interpreteReturn(sysy::Instruction *inst, mid2end::StackTable *stackTable) {
  auto zero = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::zero);
  auto sp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::sp);
  auto fp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::IntRegisterKind::fp);
  auto ra = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::ra);
  auto a0 = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::a0);
  auto fa0 = riscv::FloatPhyRegister::getFloatPhyRegister(riscv::FloatPhyRegister::fa0);
  auto returnInst = dynamic_cast<sysy::ReturnInst *>(inst);
  auto returnValue = returnInst->getReturnValue();
  auto constReturnValue = dynamic_cast<sysy::ConstantValue *>(returnValue);
  if (returnValue != nullptr) {
    if (constReturnValue != nullptr) {
      if (constReturnValue->isInt()) {
        if (constReturnValue->getInt() < -2048 || constReturnValue->getInt() > 2047) {
          Li32(a0, constReturnValue->getInt());
        } else {
          builder.createAddiwInst(a0, zero, constReturnValue->getInt());
        }
      } else {
        auto tmpReg = getOrEmplaceFloatConstantReg(constReturnValue);
        builder.createFsgnj_sInst(fa0, tmpReg, tmpReg);
      }
    } else {
      auto returnValueReg = getOrEmplaceSymRegister(dynamic_cast<sysy::User *>(returnValue), stackTable);
      auto intReturnValueReg = dynamic_cast<riscv::IntSymRegister *>(returnValueReg);
      auto floatReturnValueReg = dynamic_cast<riscv::FloatSymRegister *>(returnValueReg);
      if (intReturnValueReg != nullptr) {
        builder.createAddiwInst(a0, intReturnValueReg, 0);
      } else {
        builder.createFsgnj_sInst(fa0, floatReturnValueReg, floatReturnValueReg);
      }
    }
  }
  builder.createLdInst(ra, fp, -8);
  builder.createLdInst(fp, fp, -16);
  builder.createAddiInst(sp, sp, 0);
  builder.createJalrInst(zero, ra, 0);
}
void MacroExpansion::interpreteAlloca(sysy::Instruction *inst) {}
void MacroExpansion::initBasicElements() {
  using IntRegKind = riscv::IntPhyRegister::IntRegisterKind;
  pModule = std::make_unique<riscv::Module>();
  auto fp = riscv::IntPhyRegister::getIntPhyRegister(riscv::IntPhyRegister::fp);
  // 初始化全局数据
  for (const auto &global : midModule->getGlobals()) {
    bool isInt = global->getType() == sysy::Type::getPointerType(sysy::Type::getIntType());
    std::vector<unsigned> dims;
    for (const auto &dim : global->getDims()) {
      dims.push_back(dynamic_cast<sysy::ConstantValue *>(dim->getValue())->getInt());
    }
    ConstantCounter init;
    auto valueCounter = global->getInitValues();
    auto &values = valueCounter.getValues();
    auto &nums = valueCounter.getNumbers();
    for (unsigned i = 0; i < values.size(); i++) {
      auto constValue = dynamic_cast<sysy::ConstantValue *>(values[i]);
      if (isInt) {
        init.push_back(constValue->getInt(), nums[i]);
      } else {
        init.push_back(constValue->getFloat(), nums[i]);
      }
    }
    pModule->createGlobal(isInt, dims, init, global->getName());
  }
  // 注意，多维常量数组可看成全局变量
  for (const auto &constVar : midModule->getConsts()) {
    if (constVar->getNumDims() != 0) {
      bool isInt = constVar->getType() == sysy::Type::getPointerType(sysy::Type::getIntType());
      std::vector<unsigned> dims;
      for (const auto &dim : constVar->getDims()) {
        dims.push_back(dynamic_cast<sysy::ConstantValue *>(dim->getValue())->getInt());
      }
      ConstantCounter init;
      auto valueCounter = constVar->getInitValues();
      auto &values = valueCounter.getValues();
      auto &nums = valueCounter.getNumbers();
      for (unsigned i = 0; i < values.size(); i++) {
        auto constValue = dynamic_cast<sysy::ConstantValue *>(values[i]);
        if (isInt) {
          init.push_back(constValue->getInt(), nums[i]);
        } else {
          init.push_back(constValue->getFloat(), nums[i]);
        }
      }
      pModule->createGlobal(isInt, dims, init, constVar->getName());
    }
  }

  // 初始化函数
  for (const auto &functionItem : midModule->getFunctions()) {
    using ReturnType = riscv::Function::ReturnType;
    ReturnType returnType;
    if (functionItem.second->getReturnType() == sysy::Type::getVoidType()) {
      returnType = ReturnType::VOID;
    } else if (functionItem.second->getReturnType() == sysy::Type::getIntType()) {
      returnType = ReturnType::INT;
    } else if (functionItem.second->getReturnType() == sysy::Type::getFloatType()) {
      returnType = ReturnType::FLOAT;
    } else {
      assert(false);
    }
    auto curFunction = pModule->createFunction(functionItem.first, returnType);
    auto curStackTable = new mid2end::StackTable;
    functionStackMap.emplace(curFunction, curStackTable);
  }
  // 初始化外部函数
  for (const auto &functionItem : midModule->getExternalFunctions()) {
    using ReturnType = riscv::Function::ReturnType;
    ReturnType returnType;
    if (functionItem.second->getReturnType() == sysy::Type::getVoidType()) {
      returnType = ReturnType::VOID;
    } else if (functionItem.second->getReturnType() == sysy::Type::getIntType()) {
      returnType = ReturnType::INT;
    } else if (functionItem.second->getReturnType() == sysy::Type::getFloatType()) {
      returnType = ReturnType::FLOAT;
    } else {
      assert(false);
    }
    auto curFunction = pModule->createFunction(functionItem.first, returnType, true);
    auto curStackTable = new mid2end::StackTable;
    functionStackMap.emplace(curFunction, curStackTable);
  }

  // 头部块工作(需要溢出后修改)
  // 初始化基本块以及控制流图关系
  // 初始化局部数组
  for (const auto &functionItem : midModule->getFunctions()) {
    auto curFunction = pModule->getFunction(functionItem.first);
    auto curStackTable = functionStackMap.find(curFunction)->second.get();
    auto entryBlock = functionItem.second->getEntryBlock();
    auto curBlock = getOrEmplaceBasicBlock(curFunction, entryBlock);
    auto sp = riscv::IntPhyRegister::getIntPhyRegister(IntRegKind::sp);
    auto ra = riscv::IntPhyRegister::getIntPhyRegister(IntRegKind::ra);
    builder.setPosition(curBlock);
    builder.createSdInst(ra, sp, -8);
    builder.createSdInst(fp, sp, -16);
    builder.createAddiInst(fp, sp, 0);
    builder.createAddiInst(sp, sp, 0);
    for (const auto &block : functionItem.second->getBasicBlocks()) {
      auto curBlock = getOrEmplaceBasicBlock(curFunction, block.get());
      for (const auto &pred : block->getPredecessors()) {
        curBlock->addPredecessor(getOrEmplaceBasicBlock(curFunction, pred));
      }
      for (const auto &succ : block->getSuccessors()) {
        curBlock->addSuccessor(getOrEmplaceBasicBlock(curFunction, succ));
      }
      for (const auto &inst : block->getInstructions()) {
        auto allocaInst = dynamic_cast<sysy::AllocaInst *>(inst.get());
        if (allocaInst != nullptr && allocaInst->getNumDims() != 0 &&
            dynamic_cast<sysy::ConstantValue *>(allocaInst->getDim(0))->getInt() != -1) {
          std::vector<unsigned> dims;
          for (const auto &dim : allocaInst->getDims()) {
            dims.push_back(dynamic_cast<sysy::ConstantValue *>(dim->getValue())->getInt());
          }
          bool isInt = allocaInst->getType() == sysy::Type::getPointerType(sysy::Type::getIntType());
          localArrayMap.emplace(allocaInst, new mid2end::LocalArray(isInt, dims, allocaInst->getName()));
          curStackTable->addLocalArray(localArrayMap.find(allocaInst)->second.get());
        }
      }
    }
  }
  // 初始化外部函数的头部块
  for (const auto &functionItem : midModule->getExternalFunctions()) {
    auto curFunction = pModule->getFunction(functionItem.first);
    auto entryBlock = functionItem.second->getEntryBlock();
    getOrEmplaceBasicBlock(curFunction, entryBlock);
  }

  // 初始化溢出形式参数以及未溢出形式参数
  for (const auto &functionItem : midModule->getFunctions()) {
    auto curFunction = pModule->getFunction(functionItem.first);
    auto curStackTable = functionStackMap.find(curFunction)->second.get();
    auto paramList = functionItem.second->getEntryBlock()->getArguments();
    std::list<sysy::AllocaInst *> notSpillParams;
    std::list<sysy::AllocaInst *> spillParams;

    unsigned numIntParams = 0;
    unsigned numFloatParams = 0;
    for (const auto &param : paramList) {
      if (param->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) || param->getNumDims() != 0) {
        if (numIntParams < 8) {
          notSpillParams.emplace_back(param);
        } else {
          spillParams.emplace_back(param);
        }
        numIntParams++;
      } else {
        if (numFloatParams < 8) {
          notSpillParams.emplace_back(param);
        } else {
          spillParams.emplace_back(param);
        }
        numFloatParams++;
      }
    }

    builder.setPosition(curFunction->getEntryBlock());
    numIntParams = 0;
    numFloatParams = 0;
    for (const auto &param : notSpillParams) {
      if (param->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) || param->getNumDims() != 0) {
        auto rs1 = dynamic_cast<riscv::IntPhyRegister *>(getParamPhyReg(true, numIntParams));
        if (param->getNumDims() != 0) {
          auto result = new riscv::IntSymRegister(true, param->getName());
          builder.createAddiInst(result, rs1, 0);
          variableMap.emplace(param, result);
          symbolRegisterList.emplace_back(result);
        } else {
          auto result = new riscv::IntSymRegister(false, param->getName());
          builder.createAddiwInst(result, rs1, 0);
          variableMap.emplace(param, result);
          symbolRegisterList.emplace_back(result);
        }
        numIntParams++;
      } else {
        auto result = new riscv::FloatSymRegister(param->getName());
        auto rs1 = dynamic_cast<riscv::FloatPhyRegister *>(getParamPhyReg(false, numFloatParams));
        builder.createFsgnj_sInst(dynamic_cast<riscv::FloatSymRegister *>(result), rs1, rs1);
        variableMap.emplace(param, result);
        symbolRegisterList.emplace_back(result);
        numFloatParams++;
      }
    }

    if (!spillParams.empty()) {
      auto addrReg = dynamic_cast<riscv::IntSymRegister *>(getTmpRegister(true, true));
      builder.createAddiInst(addrReg, fp, 0);
      unsigned lastAddr = 0;
      for (const auto &param : spillParams) {
        if (param->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) || param->getNumDims() != 0) {
          if (param->getNumDims() != 0) {
            auto result = new riscv::IntSymRegister(true, param->getName());
            curStackTable->addSpillParam(result);
            builder.createAddiInst(addrReg, addrReg, curStackTable->getSpillParamAddr(result) - lastAddr);
            builder.createLdInst(result, addrReg, 0);
            lastAddr = curStackTable->getSpillParamAddr(result);
            variableMap.emplace(param, result);
            symbolRegisterList.emplace_back(result);
          } else {
            auto result = new riscv::IntSymRegister(false, param->getName());
            curStackTable->addSpillParam(result);
            builder.createAddiInst(addrReg, addrReg, curStackTable->getSpillParamAddr(result) - lastAddr);
            builder.createLwInst(result, addrReg, 0);
            lastAddr = curStackTable->getSpillParamAddr(result);
            variableMap.emplace(param, result);
            symbolRegisterList.emplace_back(result);
          }
        } else {
          auto result = new riscv::FloatSymRegister(param->getName());
          curStackTable->addSpillParam(result);
          builder.createAddiInst(addrReg, addrReg, curStackTable->getSpillParamAddr(result) - lastAddr);
          builder.createFlwInst(result, addrReg, 0);
          lastAddr = curStackTable->getSpillParamAddr(result);
          variableMap.emplace(param, result);
          symbolRegisterList.emplace_back(result);
        }
      }
    }
  }
  for (const auto &functionItem : midModule->getExternalFunctions()) {
    auto curFunction = pModule->getFunction(functionItem.first);
    auto curStackTable = functionStackMap.find(curFunction)->second.get();
    auto paramList = functionItem.second->getEntryBlock()->getArguments();
    std::list<sysy::AllocaInst *> notSpillParams;
    std::list<sysy::AllocaInst *> spillParams;

    unsigned numIntParams = 0;
    unsigned numFloatParams = 0;
    for (const auto &param : paramList) {
      if (param->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) || param->getNumDims() != 0) {
        if (numIntParams < 8) {
          notSpillParams.emplace_back(param);
        } else {
          spillParams.emplace_back(param);
        }
        numIntParams++;
      } else {
        if (numFloatParams < 8) {
          notSpillParams.emplace_back(param);
        } else {
          spillParams.emplace_back(param);
        }
        numFloatParams++;
      }
    }

    numIntParams = 0;
    numFloatParams = 0;
    for (const auto &param : notSpillParams) {
      if (param->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) || param->getNumDims() != 0) {
        if (param->getNumDims() != 0) {
          auto result = new riscv::IntSymRegister(true, param->getName());
          variableMap.emplace(param, result);
          symbolRegisterList.emplace_back(result);
        } else {
          auto result = new riscv::IntSymRegister(false, param->getName());
          variableMap.emplace(param, result);
          symbolRegisterList.emplace_back(result);
        }
        numIntParams++;
      } else {
        auto result = new riscv::FloatSymRegister(param->getName());
        variableMap.emplace(param, result);
        symbolRegisterList.emplace_back(result);
        numFloatParams++;
      }
    }

    if (!spillParams.empty()) {
      for (const auto &param : spillParams) {
        if (param->getType() == sysy::Type::getPointerType(sysy::Type::getIntType()) || param->getNumDims() != 0) {
          if (param->getNumDims() != 0) {
            auto result = new riscv::IntSymRegister(true, param->getName());
            curStackTable->addSpillParam(result);
            variableMap.emplace(param, result);
            symbolRegisterList.emplace_back(result);
          } else {
            auto result = new riscv::IntSymRegister(false, param->getName());
            curStackTable->addSpillParam(result);
            variableMap.emplace(param, result);
            symbolRegisterList.emplace_back(result);
          }
        } else {
          auto result = new riscv::FloatSymRegister(param->getName());
          curStackTable->addSpillParam(result);
          variableMap.emplace(param, result);
          symbolRegisterList.emplace_back(result);
        }
      }
    }
  }
}
// 跳转地址的限制(须寄存器分配插入溢出代码后处理)
// 条件跳转的标签未处理
void MacroExpansion::interpreteOneIR(sysy::Function *function, sysy::Instruction *inst,
                                     mid2end::StackTable *stackTable) {
  using Kind = sysy::Instruction::Kind;
  switch (inst->getKind()) {
    case Kind::kAdd: {
      interpreteAdd(inst, stackTable);
      break;
    }
    case Kind::kSub: {
      interpreteSub(inst, stackTable);
      break;
    }
    case Kind::kMul: {
      interpreteMul(inst, stackTable);
      break;
    }
    case Kind::kDiv: {
      interpreteDiv(inst, stackTable);
      break;
    }
    case Kind::kRem: {
      interpreteRem(inst, stackTable);
      break;
    }
    case Kind::kAnd: {
      interpreteAnd(inst, stackTable);
      break;
    }
    case Kind::kOr: {
      interpreteOr(inst, stackTable);
      break;
    }
    case Kind::kICmpEQ: {
      interpreteICmpEQ(inst, stackTable);
      break;
    }
    case Kind::kICmpNE: {
      interpreteICmpNE(inst, stackTable);
      break;
    }
    case Kind::kICmpLT: {
      interpreteICmpLT(inst, stackTable);
      break;
    }
    case Kind::kICmpGT: {
      interpreteICmpGT(inst, stackTable);
      break;
    }
    case Kind::kICmpLE: {
      interpreteICmpLE(inst, stackTable);
      break;
    }
    case Kind::kICmpGE: {
      interpreteICmpGE(inst, stackTable);
      break;
    }
    case Kind::kFAdd: {
      interpreteFAdd(inst, stackTable);
      break;
    }
    case Kind::kFSub: {
      interpreteFSub(inst, stackTable);
      break;
    }
    case Kind::kFMul: {
      interpreteFMul(inst, stackTable);
      break;
    }
    case Kind::kFDiv: {
      interpreteFDiv(inst, stackTable);
      break;
    }
    case Kind::kFCmpEQ: {
      interpreteFCmpEQ(inst, stackTable);
      break;
    }
    case Kind::kFCmpNE: {
      interpreteFCmpNE(inst, stackTable);
      break;
    }
    case Kind::kFCmpLT: {
      interpreteFCmpLT(inst, stackTable);
      break;
    }
    case Kind::kFCmpGT: {
      interpreteFCmpGT(inst, stackTable);
      break;
    }
    case Kind::kFCmpLE: {
      interpreteFCmpLE(inst, stackTable);
      break;
    }
    case Kind::kFCmpGE: {
      interpreteFCmpGE(inst, stackTable);
      break;
    }
    case Kind::kNeg: {
      interpreteNeg(inst, stackTable);
      break;
    }
    case Kind::kNot: {
      interpreteNot(inst, stackTable);
      break;
    }
    case Kind::kFNeg: {
      interpreteFNeg(inst, stackTable);
      break;
    }
    case Kind::kFNot: {
      interpreteFNot(inst, stackTable);
      break;
    }
    case Kind::kFtoI: {
      interpreteFtoI(inst, stackTable);
      break;
    }
    case Kind::kItoF: {
      interpreteItoF(inst, stackTable);
      break;
    }
    case Kind::kCondBr: {
      interpreteCondBr(inst, stackTable);
      break;
    }
    case Kind::kBr: {
      interpreteBr(inst);
      break;
    }
    case Kind::kLoad: {
      interpreteLoad(inst, stackTable);
      break;
    }
    case Kind::kLa: {
      interpreteLa(inst, stackTable);
      break;
    }
    case Kind::kStore: {
      interpreteStore(inst, stackTable);
      break;
    }
    case Kind::kMemset: {
      interpreteMemset(inst, stackTable);
      break;
    }
    case Kind::kCall: {
      interpreteCall(function, inst, stackTable);
      break;
    }
    case Kind::kReturn: {
      interpreteReturn(inst, stackTable);
      break;
    }
    case Kind::kAlloca: {
      interpreteAlloca(inst);
      break;
    }
    default:
      assert(false);
  }
}

void MacroExpansion::generateInstrution() {
  for (const auto &functionItem : midModule->getFunctions()) {
    auto curFunction = pModule->getFunction(functionItem.first);
    auto curStackTable = functionStackMap.find(curFunction)->second.get();
    for (const auto &block : functionItem.second->getBasicBlocks()) {
      builder.setPosition(basicBlockMap.at(block.get()));
      for (const auto &inst : block->getInstructions()) {
        interpreteOneIR(functionItem.second.get(), inst.get(), curStackTable);
      }
    }

    curFunction->sortBlocks();
  }
}

void MacroExpansion::select() {
  initBasicElements();
  generateInstrution();
}
}  // namespace macroExpansion