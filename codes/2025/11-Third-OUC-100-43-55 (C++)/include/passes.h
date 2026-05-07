#ifndef PASS_PASSES_HPP
#define PASS_PASSES_HPP

#include "module.h"
#include <stack>

namespace pass {

class Pass {
protected:
  const ir::Module *module;

public:
  explicit Pass(const ir::Module *module) { this->module = module; }

  virtual ~Pass() = default;

  virtual bool onModule() = 0;
};

class FunctionPass : public Pass {
public:
  explicit FunctionPass(const ir::Module *module) : Pass(module) {}

  bool onModule() override {
    bool modified = false;
    for (auto function : module->getFunctions()) {
      if (function->empty()) {
        continue; // Skip empty functions
      }
      modified |= onFunction(function);
    }
    return modified;
  }

  virtual bool onFunction(ir::Function *function) = 0;
};

class LowerPass : public Pass {
public:
  explicit LowerPass(const ir::Module *module) : Pass(module) {}

  bool onModule() override {
    bool modified = false;
    for (auto function : module->getMFuncs()) {
      modified |= onFunction(function);
    }
    return modified;
  }

  virtual bool onFunction(riscv::MachineFunc *function) = 0;
};

class BranchOpti : public FunctionPass {
public:
  explicit BranchOpti(const ir::Module *module) : FunctionPass(module) {}

  bool onFunction(ir::Function *function) override;
};

class ConstProp : public FunctionPass {
public:
  explicit ConstProp(const ir::Module *module) : FunctionPass(module) {}

  bool onFunction(ir::Function *function) override;
};

class DeadCodeElim : public FunctionPass {
public:
  explicit DeadCodeElim(const ir::Module *module) : FunctionPass(module) {}

  ~DeadCodeElim() override = default;

  bool onFunction(ir::Function *function) override;
};

class MemoryProm : public FunctionPass {
private:
  std::unordered_map<ir::BasicBlock *,
                     std::unordered_map<ir::AllocaInst *, ir::PhiInst *>>
      _globalPhiMap;
  std::unordered_set<ir::AllocaInst *> static analyzePromotableAllocaInsts(
      ir::Function *function);
  static bool isPromotable(ir::AllocaInst *allocaInst);
  void removeUnreachableBlocks(ir::Function *func);
  void insertPhi(ir::Function *func,
                 std::unordered_map<ir::BasicBlock *,
                                    std::unordered_set<ir::BasicBlock *>> &df,
                 std::unordered_set<ir::AllocaInst *> &allocaInsts);
  void replace(
      ir::BasicBlock *block,
      std::unordered_map<ir::AllocaInst *, std::stack<ir::Value *>> &replaceMap,
      std::unordered_set<ir::AllocaInst *> &allocaInsts,
      std::unordered_map<ir::BasicBlock *, std::unordered_set<ir::BasicBlock *>>
          &domTree);
  void clearPhi(ir::Function *func);

public:
  explicit MemoryProm(const ir::Module *module) : FunctionPass(module) {}

  bool onFunction(ir::Function *function) override;
};

// class ReducePhi : public FunctionPass {
// public:
//   explicit ReducePhi(const ir::Module *module) : FunctionPass(module) {}
//   bool onFunction(ir::Function *function) override;
// };

// class CommonExpElim : public FunctionPass {
// public:
//   explicit CommonExpElim(const ir::Module *module) : FunctionPass(module) {}
//   bool onFunction(ir::Function *function) override;
// };

class RemoveJump : public LowerPass {
public:
  explicit RemoveJump(const ir::Module *module) : LowerPass(module) {}
  bool onFunction(riscv::MachineFunc *function) override;
};

} // namespace pass

#endif // PASS_PASSES_HPP
