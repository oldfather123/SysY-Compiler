#ifndef PASS_PASS_MANAGER_H
#define PASS_PASS_MANAGER_H

#include "module.h"
#include "passes.h"

namespace pass {
class PassManager {
private:
  const ir::Module *_module;

public:
  explicit PassManager(const ir::Module *module) : _module(module) {}

  void run() const {
    bool once = false;
    bool l1 = true;
    while (l1) {
      l1 = false;
      // Inner loop
      bool l0 = true;
      while (l0) {
        l0 = false;
        l0 |= DeadCodeElim(_module).onModule();
        l0 |= ConstProp(_module).onModule();
        l0 |= BranchOpti(_module).onModule();
      }
      // l1 |= MemoryProm(_module).onModule();
      if (!once) {
        once = true;
        MemoryProm(_module).onModule();
        DeadCodeElim(_module).onModule();
        ConstProp(_module).onModule();
        // BranchOpti(_module).onModule();
        // DeadCodeElim(_module).onModule();
        // ConstProp(_module).onModule();
        // BranchOpti(_module).onModule();
      }
    }
  }

  void runLower() const { return; }

  void runLast() const { RemoveJump(_module).onModule(); }
};
} // namespace pass

#endif // PASS_PASS_MANAGER_HPP