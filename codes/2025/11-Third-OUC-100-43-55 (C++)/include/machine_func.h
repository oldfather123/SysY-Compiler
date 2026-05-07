#ifndef RISCV_MACHINE_FUNC_H
#define RISCV_MACHINE_FUNC_H

#include "basic_block.h"
#include "instructions.h"
#include "machine_insts.h"
#include "type.h"

namespace riscv {

class MachineFunc;
class MachineInst;
class PhiNode;
class Jump;

class MachineBlock : public ir::BlockBase {
private:
  static int _counter;
  ir::BasicBlock *_origin = nullptr;
  std::string _name;

public:
  MachineBlock();
  MachineBlock(ir::BasicBlock *origin)
      : BlockBase(origin->getID()), _origin(origin) {}
  MachineBlock(int id);
  MachineBlock(std::string name);

  MachineInst *pushMInst(std::unique_ptr<MachineInst> inst);
  MachineInst *getMInst(size_t index) const;
  bool hasUncondJump() const;
  bool hasCondJump() const;
  Jump *getUncondJump() const;
  Jump *getCondJump() const;

  ir::ValueKind getValueKind() const override {
    return ir::ValueKind::MachineBlock;
  }
  ir::BasicBlock *getOrigin() const { return _origin; }
  std::string getLabel() const override;
  std::string getName() const override;
};

class MachineFunc : public ir::FuncBase {
private:
  ir::Function *_origin;
  int _localSize, _iCallerNum, _fCallerNum;
  std::unordered_map<ir::AllocaInst *, int> _localOffsets;
  std::unordered_map<ir::Argument *, std::pair<bool, int>> _argOffsets;
  std::unordered_map<ir::Instruction *, MachineInst *> _instMap;
  std::vector<std::unique_ptr<PhiNode>> _phiNodes;

  void initCallerNums();
  void initLocalOffsets();
  void initArgOffsets();

  MachineInst *handleArg(ir::Argument *arg, MachineBlock *block);

public:
  int maxFuncParamNum = 0;
  std::unordered_map<ir::BasicBlock *, MachineBlock *> blockMap;

  // Not sure about the type but I guess it's fine (ATTENTION)
  MachineFunc(ir::Function *func);

  PhiNode *pushPhiNode(std::unique_ptr<PhiNode> phiNode);
  void addInstPair(ir::Instruction *inst, MachineInst *mInst) {
    assert(inst && mInst);
    _instMap[inst] = mInst;
  }

  int getFCallerNum() const { return _fCallerNum; }
  int getICallerNum() const { return _iCallerNum; }
  int getLocalSize() const { return _localSize; }
  ir::Function *getOriginalFunc() const { return _origin; }
  std::string getName() const override { return getRawName(); }
  ir::ValueKind getValueKind() const override {
    return ir::ValueKind::MachineFunc;
  }
  std::string str() const override;

  void binary(ir::BinaryInst *inst, MachineBlock *block);
  void branch(ir::BranchInst *inst, MachineBlock *block);
  int call(ir::CallInst *inst, MachineBlock *block);
  void gep(ir::GetElementPtrInst *inst, MachineBlock *block);
  void load(ir::LoadInst *inst, MachineBlock *block);
  void ret(ir::RetInst *inst, MachineBlock *block, MachineBlock *exitBlock);
  void store(ir::StoreInst *inst, MachineBlock *block);
  void icmp(ir::CmpInst *inst, MachineBlock *block);
  void fcmp(ir::CmpInst *inst, MachineBlock *block);
  void bitcast(ir::CastInst *inst, MachineBlock *block);
  void zext(ir::CastInst *inst, MachineBlock *block);
  void sext(ir::CastInst *inst, MachineBlock *block);
  void sitofp(ir::CastInst *inst, MachineBlock *block);
  void fptosi(ir::CastInst *inst, MachineBlock *block);
  void move(ir::MoveInst *inst, MachineBlock *block);
};

} // namespace riscv

#endif
