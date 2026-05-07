#include "../include/backend.h"
#include "../include/global.h"
#include <variant>

namespace aaac {
namespace backend {

namespace {
bool isFusibleInsn(OpCode op) {
  switch (op) {
  case OpCode::Add32:
  case OpCode::Add64:
  case OpCode::Sub:
  case OpCode::Mul32:
  case OpCode::Mul64:
  case OpCode::Div:
  case OpCode::Mod:
  case OpCode::LShift:
  case OpCode::RShift:
  case OpCode::BitAnd:
  case OpCode::BitOr:
  case OpCode::BitXor:
  case OpCode::LogAnd:
  case OpCode::LogOr:
  case OpCode::LogNot:
  case OpCode::Negate:
  case OpCode::Load:
  case OpCode::GlobalLoad:
  case OpCode::LoadDataAddress:
    return true;
  default:
    return false;
  }
}

bool tryFusion(InsnList &insns, InsnNode it) {
  auto next_it = std::next(it);
  if (next_it == insns.end())
    return false;

  auto &cur = *it;
  auto &next = *next_it;

  // {ALU rn, rs1, rs2; mv rd, rn;} -> {ALU rd, rs1, rs2;}
  if (next->getOpCode() == OpCode::Assign && isFusibleInsn(cur->getOpCode())) {
    auto dest = cur->getDest();
    auto src0 = next->getSrc0();
    if (dest && std::holds_alternative<std::shared_ptr<sym::Var>>(src0)) {
      auto srcVar = std::get<std::shared_ptr<sym::Var>>(src0);
      if (srcVar == dest) {
        for (auto scan_it = std::next(next_it); scan_it != insns.end();
             ++scan_it) {
          auto &scan_insn = *scan_it;
          if (scan_insn->getDest() && scan_insn->getDest() == dest)
            break;
          auto op = scan_insn->getSrc0();
          if (std::holds_alternative<std::shared_ptr<sym::Var>>(op) &&
              std::get<std::shared_ptr<sym::Var>>(op) == dest)
            return false;
          op = scan_insn->getSrc1();
          if (std::holds_alternative<std::shared_ptr<sym::Var>>(op) &&
              std::get<std::shared_ptr<sym::Var>>(op) == dest)
            return false;
          op = scan_insn->getSrcPseudo();
          if (std::holds_alternative<std::shared_ptr<sym::Var>>(op) &&
              std::get<std::shared_ptr<sym::Var>>(op) == dest)
            return false;
        }
        if (auto common = std::dynamic_pointer_cast<CommonInsn>(cur)) {
          common->setDestination(next->getDest());
          insns.erase(next_it);
          return true;
        }
      }
    }
  }

  if (cur->getOpCode() == OpCode::LoadConstant) {
    auto constInsn = std::dynamic_pointer_cast<ConstantLoadInsn>(cur);
    if (!constInsn || !constInsn->isInt())
      return false;
    int val = constInsn->getInt();
    auto nextCommon = std::dynamic_pointer_cast<CommonInsn>(next);
    if (!nextCommon)
      return false;
    auto curDest = constInsn->getDestination();

    // {li rn, 0; add rd, rs1, rn;} or {li rn, 0; add rd, rn, rs1;} -> {mv rd, rs1;}
    if (val == 0 &&
        (nextCommon->getOpcode() == OpCode::Add32 ||
         nextCommon->getOpcode() == OpCode::Add64)) {
      if ((std::holds_alternative<std::shared_ptr<sym::Var>>(nextCommon->getSrc0()) &&
           nextCommon->getSrcVar(0) == curDest) ||
          (std::holds_alternative<std::shared_ptr<sym::Var>>(nextCommon->getSrc1()) &&
           nextCommon->getSrcVar(1) == curDest)) {
        std::shared_ptr<sym::Var> nonZeroSrc;
        if (std::holds_alternative<std::shared_ptr<sym::Var>>(nextCommon->getSrc0()) &&
            nextCommon->getSrcVar(0) == curDest)
          nonZeroSrc = nextCommon->getSrcVar(1);
        else
          nonZeroSrc = nextCommon->getSrcVar(0);
        auto newInsn =
            CommonInsn::create(OpCode::Assign, nextCommon->getDest(),
                                frontend::Operand(nonZeroSrc), frontend::Operand{});
        *it = newInsn;
        insns.erase(next_it);
        return true;
      }
    }

    // {li rn, 0; sub rd, rs1, rn;} -> {mv rd, rs1;}
    // {li rn, 0; sub rd, rn, rs1;} -> {negate rd, rs1;}
    if (val == 0 && nextCommon->getOpcode() == OpCode::Sub) {
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(nextCommon->getSrc1()) &&
          nextCommon->getSrcVar(1) == curDest) {
        auto newInsn = CommonInsn::create(
            OpCode::Assign, nextCommon->getDest(),
            frontend::Operand(nextCommon->getSrcVar(0)), frontend::Operand{});
        *it = newInsn;
        insns.erase(next_it);
        return true;
      }
      if (std::holds_alternative<std::shared_ptr<sym::Var>>(nextCommon->getSrc0()) &&
          nextCommon->getSrcVar(0) == curDest) {
        auto newInsn = CommonInsn::create(
            OpCode::Negate, nextCommon->getDest(),
            frontend::Operand(nextCommon->getSrcVar(1)), frontend::Operand{});
        *it = newInsn;
        insns.erase(next_it);
        return true;
      }
    }

    // {li rn, 0; mul rd, rs1, rn;} -> {li rd, 0;}
    if (val == 0 &&
        (nextCommon->getOpcode() == OpCode::Mul32 ||
         nextCommon->getOpcode() == OpCode::Mul64)) {
      if ((std::holds_alternative<std::shared_ptr<sym::Var>>(nextCommon->getSrc0()) &&
           nextCommon->getSrcVar(0) == curDest) ||
          (std::holds_alternative<std::shared_ptr<sym::Var>>(nextCommon->getSrc1()) &&
           nextCommon->getSrcVar(1) == curDest)) {
        auto newInsn =
            ConstantLoadInsn::create(nextCommon->getDest(), 0);
        *it = newInsn;
        insns.erase(next_it);
        return true;
      }
    }

    // {li rn, 1; mul rd, rs1, rn;} -> {mv rd, rs1;}
    if (val == 1 &&
        (nextCommon->getOpcode() == OpCode::Mul32 ||
         nextCommon->getOpcode() == OpCode::Mul64)) {
      if ((std::holds_alternative<std::shared_ptr<sym::Var>>(nextCommon->getSrc0()) &&
           nextCommon->getSrcVar(0) == curDest) ||
          (std::holds_alternative<std::shared_ptr<sym::Var>>(nextCommon->getSrc1()) &&
           nextCommon->getSrcVar(1) == curDest)) {
        std::shared_ptr<sym::Var> nonOneSrc;
        if (std::holds_alternative<std::shared_ptr<sym::Var>>(nextCommon->getSrc0()) &&
            nextCommon->getSrcVar(0) == curDest)
          nonOneSrc = nextCommon->getSrcVar(1);
        else
          nonOneSrc = nextCommon->getSrcVar(0);
        auto newInsn =
            CommonInsn::create(OpCode::Assign, nextCommon->getDest(),
                                frontend::Operand(nonOneSrc), frontend::Operand{});
        *it = newInsn;
        insns.erase(next_it);
        return true;
      }
    }
  }

  return false;
}

void optimizeList(InsnList &insns) {
  bool changed;
  do {
    changed = false;
    for (auto it = insns.begin(); it != insns.end();) {
      auto &insn = *it;
      if (insn->getOpCode() == OpCode::Assign) {
        auto dest = insn->getDest();
        auto src0 = insn->getSrc0();
        if (dest &&
            std::holds_alternative<std::shared_ptr<sym::Var>>(src0) &&
            dest == std::get<std::shared_ptr<sym::Var>>(src0)) {
          it = insns.erase(it);
          changed = true;
          continue;
        }
      }

      if (tryFusion(insns, it)) {
        changed = true;
        continue;
      }

      ++it;
    }
  } while (changed);
}
} // namespace

void runOnFunction(const std::shared_ptr<Function> &func) {
  if (!func)
    return;
  for (auto bb = func->getStartBasicBlock(); bb; bb = bb->getNext()) {
    auto &info = bb->prop<backend::BasicBlockInfo>();
    if (info.insn_list)
      optimizeList(*info.insn_list);
  }
}

void peephole() {
  for (auto &func : globals.functions) {
    runOnFunction(func);
  }
}

} // namespace backend
} // namespace aaac
