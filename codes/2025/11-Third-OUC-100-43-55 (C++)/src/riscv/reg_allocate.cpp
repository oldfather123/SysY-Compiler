#include "reg_allocate.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace riscv {

using ir::Reg;
using std::unordered_map;
using std::unordered_set;
using std::vector;

vector<BlockInfo *> FuncRegAlloc::calcBlockInfos() const {
  // 1. Create BlockInfos and map them to MachineBlocks
  vector<BlockInfo *> blocks;
  unordered_map<MachineBlock *, BlockInfo *> blockMap;
  for (const auto &blockPtr : *_mFunc) {
    auto block = static_cast<MachineBlock *>(blockPtr.get());
    BlockInfo *info = new BlockInfo(block);
    blocks.push_back(info);
    blockMap[block] = info;
  }

  // 2. Calculate their successors
  for (auto *info : blocks) {
    MachineBlock *block = info->getMBlock();
    // Unconditional jump
    if (Jump *uncondJump = block->getUncondJump()) {
      MachineBlock *target = uncondJump->getTargetBlock();
      if (blockMap.count(target)) {
        info->addNext(blockMap[target]);
      }
    }
    // Conditional jump
    if (block->hasCondJump()) {
      Jump *condJump = block->getCondJump();
      MachineBlock *target = condJump->getTargetBlock();
      if (blockMap.count(target)) {
        info->addNext(blockMap[target]);
      }
    }
  }

  return blocks;
}

void FuncRegAlloc::calcUseDef(std::vector<BlockInfo *> &blocks) const {
  for (auto blockInfo : blocks) {
    auto mBlock = blockInfo->getMBlock();
    for (const auto &instPtr : *mBlock) {
      auto inst = static_cast<MachineInst *>(instPtr.get());
      for (const auto reg : inst->getRead()) {
        if (!blockInfo->containsInDef(reg)) {
          blockInfo->addUse(reg);
        }
      }
      for (const auto reg : inst->getWrite()) {
        blockInfo->addDef(reg);
      }
    }
  }
}

void FuncRegAlloc::calcInOut(std::vector<BlockInfo *> &blocks) {
  bool changed = true;
  while (changed) {
    changed = false;
    for (int i = blocks.size() - 1; i >= 0; --i) {
      auto blockInfo = blocks[i];
      size_t oldSize = blockInfo->sizeOfInOut();
      blockInfo->calcIn();
      blockInfo->calcOut();
      size_t newSize = blockInfo->sizeOfInOut();
      if (oldSize != newSize) {
        changed = true;
      }
    }
  }
}

unordered_map<ir::Reg *, unordered_set<MachineInst *>>
FuncRegAlloc::calcLifespans() {
  vector<BlockInfo *> blockInfos = calcBlockInfos();
  calcUseDef(blockInfos);
  calcInOut(blockInfos);
  unordered_map<ir::Reg *, unordered_set<MachineInst *>> lifespanMap;
  for (auto blockInfo : blockInfos) {
    auto &regs = blockInfo->getOut();
    auto mBlock = blockInfo->getMBlock();
    for (int i = mBlock->size() - 1; i >= 0; --i) {
      auto inst = mBlock->getMInst(i);
      for (auto reg : regs) {
        // TODO make sure this will work .try_emplace(reg).first->second?
        auto &lifespan = lifespanMap[reg];
        lifespan.insert(inst);
      }
      for (auto reg : inst->getWrite()) {
        auto &lifespan = lifespanMap[reg];
        lifespan.insert(inst);
        regs.erase(reg);
      }
      for (auto reg : inst->getRead()) {
        auto &lifespan = lifespanMap[reg];
        lifespan.insert(inst);
        regs.insert(reg);
      }
    }
  }

  return lifespanMap;
}

void FuncRegAlloc::makeFrameInfo() {
  _funcParamSize = std::max(_mFunc->maxFuncParamNum -
                                static_cast<int>(MReg::iCallerRegs.size()),
                            0) *
                   8;
  _localSize = _mFunc->getLocalSize();
  std::unordered_set<MReg *> usedICalleeRegs;
  std::unordered_set<MReg *> usedFCalleeRegs;
  _callAddrSize = 0;
  for (const auto &blockPtr : *_mFunc) {
    auto mBlock = static_cast<MachineBlock *>(blockPtr.get());
    for (const auto &instPtr : *mBlock) {
      auto inst = static_cast<MachineInst *>(instPtr.get());
      if (dynamic_cast<Call *>(inst)) {
        _callAddrSize = 8;
      }
      for (const auto reg : inst->getRegs()) {
        if (auto const mreg = dynamic_cast<MReg *>(reg)) {
          if (std::find(MReg::iCalleeRegs.begin(), MReg::iCalleeRegs.end(),
                        mreg) != MReg::iCalleeRegs.end()) {
            usedICalleeRegs.insert(mreg);
            continue;
          }
          if (std::find(MReg::fCalleeRegs.begin(), MReg::fCalleeRegs.end(),
                        mreg) != MReg::fCalleeRegs.end()) {
            usedFCalleeRegs.insert(mreg);
          }
        }
      }
    }
  }
  for (const auto reg : MReg::iCalleeRegs) {
    if (usedICalleeRegs.find(reg) != usedICalleeRegs.end()) {
      _iCalleeRegs.push_back(reg);
    }
  }
  for (const auto reg : MReg::fCalleeRegs) {
    if (usedFCalleeRegs.find(reg) != usedFCalleeRegs.end()) {
      _fCalleeRegs.push_back(reg);
    }
  }
  _savedRegSize = (static_cast<int>(_iCalleeRegs.size()) +
                   static_cast<int>(_fCalleeRegs.size())) *
                  8;
  _alignSize = (_funcParamSize + _spillSize + _localSize + _paramInnerSize +
                _savedRegSize + _callAddrSize) %
               8;
}

void FuncRegAlloc::pushFrame() {
  auto entryMBlock = static_cast<MachineBlock *>(_mFunc->getEntryBlock());
  std::vector<std::unique_ptr<MachineInst>> headIRs;
  std::vector<MReg *> toSaveRegs;
  if (_callAddrSize != 0) {
    toSaveRegs.push_back(MReg::ra);
  }
  toSaveRegs.insert(toSaveRegs.end(), _iCallerRegs.begin(), _iCallerRegs.end());
  toSaveRegs.insert(toSaveRegs.end(), _fCallerRegs.begin(), _fCallerRegs.end());
  toSaveRegs.insert(toSaveRegs.end(), _iCalleeRegs.begin(), _iCalleeRegs.end());
  toSaveRegs.insert(toSaveRegs.end(), _fCalleeRegs.begin(), _fCalleeRegs.end());
  int saveRegsSize = static_cast<int>(toSaveRegs.size()) * 8;
  if (saveRegsSize > 0) {
    headIRs.insert(headIRs.begin(),
                   std::make_unique<RRI>(RRIOp::ADDI, MReg::sp, MReg::spInst,
                                         -saveRegsSize));
    for (int i = 0; i < toSaveRegs.size(); i++) {
      MReg *toSaveReg = toSaveRegs[i];
      headIRs.insert(
          headIRs.begin(),
          std::make_unique<Store>(new MachineInst(toSaveReg), MReg::spInst,
                                  saveRegsSize - 8 * (i + 1),
                                  toSaveReg->getRegType()->isI32() ? 8 : 4));
    }
  }

  int totalSize = _funcParamSize + _alignSize + _spillSize + _localSize;
  if (totalSize > 0 && totalSize <= 255) {
    headIRs.insert(
        headIRs.begin(),
        std::make_unique<RRI>(RRIOp::ADDI, MReg::sp, MReg::spInst, -totalSize));
  } else if (totalSize > 255) {
    headIRs.insert(headIRs.begin(), std::make_unique<LI>(MReg::t0, totalSize));
    headIRs.insert(headIRs.begin(),
                   std::make_unique<RRR>(RRROp::SUB, MReg::sp, MReg::spInst,
                                         MReg::t0Inst));
  }
  for (auto &inst : headIRs) {
    entryMBlock->insertInstruction(0, std::move(inst));
  }
}

void FuncRegAlloc::popFrame() {
  auto exitMBlock = static_cast<MachineBlock *>(_mFunc->getExitBlock());
  std::vector<MReg *> toSaveRegs;
  if (_callAddrSize != 0) {
    toSaveRegs.push_back(MReg::ra);
  }
  toSaveRegs.insert(toSaveRegs.end(), _iCallerRegs.begin(), _iCallerRegs.end());
  toSaveRegs.insert(toSaveRegs.end(), _fCallerRegs.begin(), _fCallerRegs.end());
  toSaveRegs.insert(toSaveRegs.end(), _iCalleeRegs.begin(), _iCalleeRegs.end());
  toSaveRegs.insert(toSaveRegs.end(), _fCalleeRegs.begin(), _fCalleeRegs.end());
  int totalSize = static_cast<int>(toSaveRegs.size()) * 8 + _funcParamSize +
                  _alignSize + _spillSize + _localSize;
  if (totalSize > 0) {
    if (totalSize < 2048) {
      exitMBlock->pushMInst(std::make_unique<RRI>(RRIOp::ADDI, MReg::sp,
                                                  MReg::spInst, totalSize));
    } else {
      exitMBlock->pushMInst(std::make_unique<LI>(MReg::t0, totalSize));
      exitMBlock->pushMInst(std::make_unique<RRR>(RRROp::ADD, MReg::sp,
                                                  MReg::spInst, MReg::t0Inst));
    }
  }
  for (int i = 0; i < toSaveRegs.size(); i++) {
    auto toSaveReg = toSaveRegs[i];
    if (std::find(_iCallerRegs.begin(), _iCallerRegs.end(), toSaveReg) !=
            _iCallerRegs.end() ||
        std::find(_fCallerRegs.begin(), _fCallerRegs.end(), toSaveReg) !=
            _fCallerRegs.end()) {
      continue;
    }
    exitMBlock->pushMInst(
        std::make_unique<Load>(toSaveReg, MReg::spInst, -8 * (i + 1),
                               toSaveReg->getRegType()->isI32() ? 8 : 4));
  }
}

void FuncRegAlloc::replaceFakeMIRs() {
  for (const auto &blockPtr : *_mFunc) {
    auto mBlock = static_cast<MachineBlock *>(blockPtr.get());
    for (int i = 0; i < mBlock->size(); i++) {
      auto inst = mBlock->getMInst(i);
      if (inst->getMInstKind() == MInstKind::LEA) {
        auto leaInst = static_cast<LEA *>(inst);
        int totalSize =
            _funcParamSize + _alignSize + _spillSize + leaInst->getImm();
        if (totalSize < 2048) {
          auto newInst = std::make_unique<RRI>(RRIOp::ADDI, leaInst->getDest(),
                                               MReg::spInst, totalSize);
          leaInst->replaceAllUsesWith(newInst.get());
          mBlock->setInstruction(i, std::move(newInst));
        } else {
          auto newInst = std::make_unique<RRR>(RRROp::ADD, leaInst->getDest(),
                                               MReg::spInst, MReg::t0Inst);
          leaInst->replaceAllUsesWith(newInst.get());
          mBlock->setInstruction(i, std::make_unique<LI>(MReg::t0, totalSize));
          mBlock->insertInstruction(i + 1, std::move(newInst));
          i++;
        }
        continue;
      }
      if (inst->getMInstKind() == MInstKind::LoadFrom) {
        auto loadFromInst = static_cast<LoadFrom *>(inst);
        int totalSize = 0, size = 0;
        auto item = loadFromInst->getItem();
        switch (item) {
        case LoadItem::SPILL:
          totalSize = _funcParamSize + _alignSize + loadFromInst->getImm();
          break;
        case LoadItem::LOCAL:
          size = 4;
          totalSize =
              _funcParamSize + _alignSize + _spillSize + loadFromInst->getImm();
          break;
        case LoadItem::INNER:
          totalSize = _funcParamSize + _alignSize + _spillSize + _localSize +
                      _savedRegSize + loadFromInst->getImm();
          break;
        case LoadItem::OUTER:
          totalSize = _funcParamSize + _alignSize + _spillSize + _localSize +
                      _savedRegSize + _paramInnerSize + _callAddrSize +
                      loadFromInst->getImm();
          break;
        }
        if (size == 0) {
          const auto destType = loadFromInst->getDest()->getRegType();
          if (destType->getBasicKind() == ir::BasicKind::F32) {
            size = 4;
          } else if (destType->getBasicKind() == ir::BasicKind::I32) {
            size = 8;
          } else {
            throw std::runtime_error("Invalid dest type in LoadFrom");
          }
        }
        if (totalSize < 2048) {
          auto newInst = std::make_unique<Load>(loadFromInst->getDest(),
                                                MReg::spInst, totalSize, size);
          loadFromInst->replaceAllUsesWith(newInst.get());
          mBlock->setInstruction(i, std::move(newInst));
        } else {
          auto newInst = std::make_unique<Load>(loadFromInst->getDest(),
                                                MReg::t0Inst, 0, size);
          loadFromInst->replaceAllUsesWith(newInst.get());
          mBlock->setInstruction(i, std::make_unique<LI>(MReg::t0, totalSize));
          mBlock->insertInstruction(
              i + 1, std::make_unique<RRR>(RRROp::ADD, MReg::t0, MReg::spInst,
                                           MReg::t0Inst));
          mBlock->insertInstruction(i + 2, std::move(newInst));
          i += 2;
        }
        continue;
      }
      if (inst->getMInstKind() == MInstKind::StoreTo) {
        auto storeToInst = static_cast<StoreTo *>(inst);
        int totalSize = 0, size = 0;
        auto item = storeToInst->getItem();
        switch (item) {
        case StoreItem::CALL:
          totalSize = storeToInst->getImm();
          break;
        case StoreItem::SPILL:
          totalSize = _funcParamSize + _alignSize + storeToInst->getImm();
          break;
        case StoreItem::LOCAL:
          size = 4;
          totalSize =
              _funcParamSize + _alignSize + _spillSize + storeToInst->getImm();
          break;
        case StoreItem::INNER:
          totalSize = _funcParamSize + _alignSize + _spillSize + _localSize +
                      _savedRegSize + storeToInst->getImm();
          break;
        case StoreItem::OUTER:
          totalSize = _funcParamSize + _alignSize + _spillSize + _localSize +
                      _savedRegSize + _paramInnerSize + _callAddrSize +
                      storeToInst->getImm();
          break;
        }
        if (size == 0) {
          const auto destType = storeToInst->getSrc(0)->getRegType();
          if (destType->getBasicKind() == ir::BasicKind::F32) {
            size = 4;
          } else if (destType->getBasicKind() == ir::BasicKind::I32) {
            size = 8;
          } else {
            throw std::runtime_error("Invalid dest type in LoadFrom");
          }
        }
        if (totalSize < 2048) {
          mBlock->setInstruction(
              i, std::make_unique<Store>(
                     static_cast<MachineInst *>(storeToInst->getOperand(0)),
                     MReg::spInst, totalSize, size));
        } else {
          auto newInst = std::make_unique<Store>(
              static_cast<MachineInst *>(storeToInst->getOperand(0)),
              MReg::t0Inst, 0, size);
          mBlock->setInstruction(i, std::make_unique<LI>(MReg::t0, totalSize));
          mBlock->insertInstruction(
              i + 1, std::make_unique<RRR>(RRROp::ADD, MReg::t0, MReg::spInst,
                                           MReg::t0Inst));
          mBlock->insertInstruction(i + 2, std::move(newInst));
          i += 2;
        }
      }
    }
  }
}

unordered_map<Reg *, unordered_set<Reg *>> FuncRegAlloc::calcConflictMap() {
  unordered_map<Reg *, unordered_set<MachineInst *>> lifespans =
      calcLifespans();
  unordered_map<MachineInst *, unordered_set<Reg *>> regsInEackIR;
  for (const auto &lifespan : lifespans) {
    for (auto inst : lifespan.second) {
      regsInEackIR[inst].insert(lifespan.first);
    }
  }
  unordered_map<Reg *, unordered_set<Reg *>> conflictMap;
  for (const auto &[inst, regs] : regsInEackIR) {
    for (const auto reg : regs) {
      conflictMap[reg].insert(regs.begin(), regs.end());
    }
  }
  for (auto &p : conflictMap) {
    p.second.erase(p.first);
  }
  return conflictMap;
}

using ir::VReg;

void FuncRegAlloc::solveSpill() {
  _spillSize = 0;
  bool toContinueOuter;
  do {
    toContinueOuter = false;
    std::unordered_map<Reg *, std::unordered_set<Reg *>> conflictMap =
        calcConflictMap();
    std::unordered_set<VReg *> allocatedVRegs;
    std::unordered_map<VReg *, MReg *> vRegToMRegMap;
    std::unordered_map<VReg *, int> spilledRegs;
    bool toContinueInner;
    do {
      toContinueInner = false;
      for (const auto &p : conflictMap) {
        if (auto const vreg = dynamic_cast<VReg *>(p.first)) {
          allocatedVRegs.insert(vreg);
          bool toSpill = true;
          const std::vector<MReg *> &regs =
              vreg->getRegType()->isF32() ? MReg::fRegs : MReg::iRegs;
          std::unordered_set<MReg *> usedRegs;
          for (auto const reg : conflictMap.at(vreg)) {
            if (auto const vreg1 = dynamic_cast<VReg *>(reg)) {
              if (vRegToMRegMap.find(vreg1) != vRegToMRegMap.end()) {
                MReg *mreg = vRegToMRegMap.at(vreg1);
                usedRegs.insert(mreg);
              }
              continue;
            }
            if (auto const mreg = dynamic_cast<MReg *>(reg)) {
              usedRegs.insert(mreg);
              continue;
            }
            throw std::runtime_error(
                "unexpected reg type in FuncRegAlloc::solveSpill");
          }
          for (const auto mreg : regs) {
            if (usedRegs.find(mreg) != usedRegs.end()) {
              continue;
            }
            vRegToMRegMap[vreg] = mreg;
            toSpill = false;
            break;
          }
          if (toSpill) {
            VReg *toSpillReg = nullptr;
            int maxVal = 0;
            for (const auto reg : allocatedVRegs) {
              if (conflictMap.at(reg).size() > maxVal) {
                toSpillReg = reg;
                maxVal = static_cast<int>(conflictMap.at(reg).size());
              }
            }
            spilledRegs[toSpillReg] = _spillSize;
            _spillSize += 8;
            allocatedVRegs.erase(toSpillReg);
            conflictMap.erase(toSpillReg);
            // TODO will this work?
            for (auto &entry : conflictMap) {
              entry.second.erase(toSpillReg);
            }
            toContinueInner = true;
            toContinueOuter = true;
            break;
          }
        }
      }
    } while (toContinueInner);
    // // DEBUG
    // std::cout << "size: " << spilledRegs.size() << std::endl;
    for (const auto &toSpill : spilledRegs) {
      VReg *reg = toSpill.first;
      int offset = toSpill.second;
      vector<std::unique_ptr<MachineBlock>> newBlocks;
      for (size_t i = 0; i < _mFunc->size(); i++) {
        auto mBlock = static_cast<MachineBlock *>(_mFunc->getBlock(i));
        auto newMBlock = std::make_unique<MachineBlock>(mBlock->getOrigin());
        mBlock->setIndexInBlock();
        for (size_t i = 0; i < mBlock->size(); i++) {
          auto inst = mBlock->getMInst(i);
          inst->spill(reg, offset, newMBlock.get());
        }
        newBlocks.push_back(std::move(newMBlock));
      }
      // Move all blocks in newBlocks to _mFunc
      for (size_t i = 0; i < newBlocks.size(); i++) {
        auto newMBlock = std::move(newBlocks[i]);
        if (i < _mFunc->size()) {
          // _mFunc->setBlock(i, std::move(newMBlock));
          auto mBlock = static_cast<MachineBlock *>(_mFunc->getBlock(i));
          mBlock->clear();
          for (auto &instPtr : *newMBlock) {
            mBlock->pushInstruction(std::move(instPtr));
          }
        } else {
          _mFunc->pushBlock(std::move(newMBlock));
        }
      }
    }
  } while (toContinueOuter);
}

std::unordered_map<ir::VReg *, MReg *> FuncRegAlloc::calcVRegToMReg() {
  std::unordered_map<Reg *, std::unordered_set<Reg *>> conflictMap =
      calcConflictMap();
  std::unordered_map<VReg *, MReg *> vRegToMReg;
  for (const auto &p : conflictMap) {
    if (auto const vreg = dynamic_cast<VReg *>(p.first)) {
      const std::vector<MReg *> &regs =
          vreg->getRegType()->isF32() ? MReg::fRegs : MReg::iRegs;
      std::unordered_set<MReg *> usedRegs;
      for (const auto reg : conflictMap.at(vreg)) {
        if (auto const vreg1 = dynamic_cast<VReg *>(reg)) {
          MReg *mreg = vRegToMReg[vreg1];
          if (mreg) {
            usedRegs.insert(mreg);
          }
          continue;
        }
        if (auto const mreg = dynamic_cast<MReg *>(reg)) {
          usedRegs.insert(mreg);
          continue;
        }
        throw std::runtime_error(
            "unexpected reg type in FuncRegAlloc::calcVRegToMReg");
      }
      for (const auto mreg : regs) {
        if (usedRegs.find(mreg) != usedRegs.end()) {
          continue;
        }
        vRegToMReg[vreg] = mreg;
        break;
      }
    }
  }
  return vRegToMReg;
}

} // namespace riscv
