#ifndef RISCV_REG_ALLOCATE_H
#define RISCV_REG_ALLOCATE_H

#include "machine_func.h"
#include "module.h"
#include "register.h"
#include "registers.h"

#include <unordered_map>
#include <unordered_set>

namespace riscv {

class BlockInfo {
private:
  MachineBlock *_mBlock;
  std::unordered_set<ir::Reg *> _liveUse, _liveDef;
  std::unordered_set<ir::Reg *> _liveIn, _liveOut;
  std::unordered_set<BlockInfo *> _nexts;

public:
  BlockInfo(MachineBlock *mBlock) : _mBlock(mBlock) {}

  void addUse(ir::Reg *reg) { _liveUse.insert(reg); }

  void addDef(ir::Reg *reg) { _liveDef.insert(reg); }

  void addNext(BlockInfo *next) { _nexts.insert(next); }

  void calcIn() {
    _liveIn.clear();
    _liveIn.insert(_liveOut.begin(), _liveOut.end());
    for (const auto def : _liveDef) {
      if (_liveIn.find(def) != _liveIn.end()) {
        _liveIn.erase(def);
      }
    }
    _liveIn.insert(_liveUse.begin(), _liveUse.end());
  }

  void calcOut() {
    for (const auto next : _nexts) {
      _liveOut.insert(next->_liveIn.begin(), next->_liveIn.end());
    }
  }

  bool containsInDef(ir::Reg *reg) const {
    return _liveDef.find(reg) != _liveDef.end();
  }

  MachineBlock *getMBlock() const { return _mBlock; }

  std::unordered_set<ir::Reg *> getRegs() const {
    std::unordered_set<ir::Reg *> regs;
    regs.insert(_liveUse.begin(), _liveUse.end());
    regs.insert(_liveDef.begin(), _liveDef.end());
    regs.insert(_liveIn.begin(), _liveIn.end());
    regs.insert(_liveOut.begin(), _liveOut.end());
    return std::move(regs);
  }

  std::unordered_set<ir::Reg *> &getOut() { return _liveOut; }

  size_t sizeOfInOut() const { return _liveIn.size() + _liveOut.size(); }
};

class FuncRegAlloc {
private:
  MachineFunc *_mFunc;
  std::vector<MReg *> _iCallerRegs;
  std::vector<MReg *> _fCallerRegs;
  std::vector<MReg *> _iCalleeRegs{};
  std::vector<MReg *> _fCalleeRegs{};
  int _funcParamSize{}, _alignSize{}, _spillSize{}, _localSize{};
  int _savedRegSize{}, _callAddrSize{}, _paramInnerSize;

  static void calcInOut(std::vector<BlockInfo *> &blocks);

  std::vector<BlockInfo *> calcBlockInfos() const;
  std::unordered_map<ir::Reg *, std::unordered_set<ir::Reg *>>
  calcConflictMap();
  std::unordered_map<ir::Reg *, std::unordered_set<MachineInst *>>
  calcLifespans();
  void calcUseDef(std::vector<BlockInfo *> &blocks) const;
  std::unordered_map<ir::VReg *, MReg *> calcVRegToMReg();
  void makeFrameInfo();
  void pushFrame();
  void popFrame();
  void replaceFakeMIRs();
  void solveSpill();

public:
  explicit FuncRegAlloc(MachineFunc *func) : _mFunc(func) {
    _iCallerRegs = std::vector<MReg *>(MReg::iCallerRegs.begin(),
                                       MReg::iCallerRegs.begin() +
                                           _mFunc->getICallerNum());
    _fCallerRegs = std::vector<MReg *>(MReg::fCallerRegs.begin(),
                                       MReg::fCallerRegs.begin() +
                                           _mFunc->getFCallerNum());
    _paramInnerSize = (func->getICallerNum() + func->getFCallerNum()) * 8;
  }

  void allocate() {
    solveSpill();
    auto vRegToMReg = calcVRegToMReg();
    for (const auto &blockPtr : *_mFunc) {
      auto mBlock = static_cast<MachineBlock *>(blockPtr.get());
      for (const auto &instPtr : *mBlock) {
        auto inst = static_cast<MachineInst *>(instPtr.get());
        inst->replaceReg(vRegToMReg);
      }
    }
    makeFrameInfo();
    pushFrame();
    popFrame();
    replaceFakeMIRs();
  }
};

class ModuleRegAlloc {
private:
  ir::Module *_module;

public:
  explicit ModuleRegAlloc(ir::Module *module) : _module(module) {}

  void allocate() {
    for (size_t i = 0; i < _module->numMFuncs(); i++) {
      auto func = static_cast<MachineFunc *>(_module->getMFunction(i));
      FuncRegAlloc(func).allocate();
    }
  }
};

} // namespace riscv

#endif
