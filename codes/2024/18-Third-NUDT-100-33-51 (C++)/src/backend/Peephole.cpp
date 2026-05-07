#include "../include/backend/Peephole.h"

namespace riscv {
void Peephole::run(Module *pModule) {
  using Kind = riscv::Instruction::InstKind;
  using iteratorInst = std::list<std::unique_ptr<Instruction>>::iterator;

  for (auto &func : pModule->getFunctions()) {
    for (auto &block : func.second.get()->getBlocks()) {
      // auto &blockInstructions = block.get()->getInstructions();
      curBlock = block.get();
      toDelete.clear();
      for (auto &inst : curBlock->getInstructions()) {
        if (this->getTableInstNum() < this->getHoleSize()) {  //< 更新SlideTable, 装填指令
          slideTable.push_back(inst.get());
          this->setTableInstNum(this->getTableInstNum() + 1);
        } else if (this->getTableInstNum() == this->getHoleSize()) {  //< 更新SlideTable, 更新指令
          slideTable.pop_front();
          slideTable.push_back(inst.get());
        } else {
          throw std::runtime_error("Slide Window Size error");
        }

        if (slideTable.size() < this->getHoleSize()) {
          continue;
        }
        // 开始匹配模板,块内模板
        runAsPattern();
      }
      delUnuseInst();
    }
  }
}

void Peephole::runAsPattern() {
  //< 除化乘
  // div2Mul();

  //< 消除无用指令
  matchUnuseIinst();

  //< loadStore冗余
  delLSInst();

  //< math冗余
  delMathInst();
}

void Peephole::matchUnuseIinst() {
  for (auto inst = slideTable.begin(); inst != slideTable.end(); inst++) {
    using kd = riscv::Instruction::InstKind;
    auto kind = (*inst)->getKind();
    if ((*inst)->isIInst()) {
      auto iInst = dynamic_cast<IInst *>(*inst);
      if (iInst == nullptr) {
        continue;
      }
      auto desReg = iInst->getDestination();
      auto sourReg = iInst->getSource(0);
      auto desRegPhy = dynamic_cast<IntPhyRegister *>(desReg);
      auto sourRegPhy = dynamic_cast<IntPhyRegister *>(sourReg);
      if (desRegPhy != nullptr) {
        if (desRegPhy->getIntPhyRegisterKind() == IntPhyRegister::sp) {
          continue;
        }
        // continue;
      } else if (sourRegPhy != nullptr) {
        if (sourRegPhy->getIntPhyRegisterKind() == IntPhyRegister::sp) {
          continue;
        }
      }

      if (iInst->isAddrInst()) {
        continue;
      } else if (kind == kd::kAddi || kind == kd::kAddiw || kind == kd::kSlli || kind == kd::kSlliw ||
                 kind == kd::kSrli || kind == kd::kSrliw || kind == kd::kSrai || kind == kd::kSraiw ||
                 kind == kd::kOri) {  // 0 delete
        if (iInst->getDestination() == iInst->getSource(0) && iInst->getImm() == 0) {
          auto iter2del = getIterByInst(iInst);
          toDelete.emplace(iInst, true);
          // curBlock->removeInst(iter2del);
          // iter2del = curBlock->getInstructions().erase(iter2del);
        }
      } else if (kind == kd::kAndi) {
        if (iInst->getDestination() == iInst->getSource(0) && iInst->getImm() == 1) {
          auto iter2del = getIterByInst(iInst);
          toDelete.emplace(iInst, true);
          // curBlock->removeInst(iter2del);
          // iter2del = curBlock->getInstructions().erase(iter2del);
        }
      }
    }
  }
}
void Peephole::delUnuseInst() {
  auto &instructions = curBlock->getInstructions();
  for (auto inst = instructions.begin(); inst != instructions.end();) {
    // std::cout<< "deleted" <<std::endl;
    if (toDelete.find(inst->get()) != toDelete.end()) {
      inst = curBlock->removeInst(inst);
      // toDelete.erase(inst);
    } else {
      inst++;
    }
  }
  toDelete.clear();
}
void Peephole::delLSInst() {
  auto inst1 = *slideTable.begin();
  auto inst2 = *(std::next(slideTable.begin()));
  if ((!inst1->isFSInst() || !inst1->isFLInst()) && (!inst2->isFLInst() && !inst2->isFSInst()))
    if (inst1->isAddrInst() && inst2->isAddrInst()) {
      if (inst1->isLoadInst() && inst2->isLoadInst()) {
        if (inst1->getDestination() == inst2->getDestination()) {
          // inst1 无用，可删
          toDelete.emplace(inst1, true);
        }
      } else if (inst1->isLoadInst() && inst2->isStoreInst()) {  // 存存指令not use
        auto iInst1 = dynamic_cast<IInst *>(inst1);
        auto sInst2 = dynamic_cast<SInst *>(inst2);
        if (iInst1->getDestination() == sInst2->getSource(0)) {
          if (iInst1->getImm() == sInst2->getImm() && iInst1->getSource(0) == sInst2->getSource(1)) {
            toDelete.emplace(inst2, true);
          }
        }
      } else if (inst1->isStoreInst() && inst2->isLoadInst()) {
        auto sInst1 = dynamic_cast<SInst *>(inst1);
        auto iInst2 = dynamic_cast<IInst *>(inst2);
        if (sInst1->getSource(0) == iInst2->getDestination()) {
          if (sInst1->getImm() == iInst2->getImm() && sInst1->getSource(1) == iInst2->getSource(0)) {
            toDelete.emplace(inst2, true);
          }
        }
      }
    }
}
void Peephole::delMathInst() {
  auto inst1 = *slideTable.begin();
  auto inst2 = *(std::next(slideTable.begin()));
  auto kind1 = inst1->getKind();
  auto kind2 = inst2->getKind();
  if ((kind1 == Instruction::InstKind::kAddi || kind1 == Instruction::InstKind::kAddiw) &&
      (kind2 == Instruction::InstKind::kAddi || kind2 == Instruction::InstKind::kAddiw)) {
    auto iInst1 = dynamic_cast<IInst *>(inst1);
    auto iInst2 = dynamic_cast<IInst *>(inst2);
    if (iInst1->getDestination() == iInst2->getDestination() && iInst1->getSource(0) == iInst2->getSource(1)) {
      if ((iInst1->getImm() + iInst2->getImm()) == 0) {
        toDelete.emplace(inst1, true);
        toDelete.emplace(inst2, true);
      }
    }
  }
}

auto Peephole::getBeforeImm(Instruction *inst, Register *reg, int beforeNums) -> std::tuple<bool, int> {
  auto &instructions = curBlock->getInstructions();
  auto curIter = std::find_if(instructions.begin(), instructions.end(),
                              [inst](const std::unique_ptr<Instruction> &ptr) { return ptr.get() == inst; });
  int index = 0;
  auto resultBool = false;
  int result = 0;
  if (curIter != instructions.end()) {
    while (curIter != instructions.begin() && index < beforeNums) {
      index++;
      --curIter;
      auto beforInstKind = curIter->get()->getKind();
      if (beforInstKind == Instruction::InstKind::kLi) {
        auto beforeInst = curIter->get();
        if (reg == beforeInst->getDestination()) {
          resultBool = true;
        }
      }
    }
  }
}

auto Peephole::isDivInst(Instruction *inst) -> bool {
  using InstKind = riscv::Instruction::InstKind;
  return inst->getKind() == InstKind::kDiv || inst->getKind() == InstKind::kDivw;
}
auto Peephole::isDivuInst(Instruction *inst) -> bool {
  using InstKind = riscv::Instruction::InstKind;
  return inst->getKind() == InstKind::kDivu;
}
auto Peephole::getIterByInst(Instruction *inst) -> iteratorInst {
  auto &instructions = curBlock->getInstructions();
  auto curIter = std::find_if(instructions.begin(), instructions.end(),
                              [inst](std::unique_ptr<Instruction> &ptr) { return ptr.get() == inst; });
  if (curIter != instructions.end()) {
    return curIter;
  }
}
}  // namespace riscv
