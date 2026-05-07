/**
 * BasicBlock ir
 * Create by Zhang Junbin at 2024/6/2
 */
#ifndef _BASIC_BLOCK_H
#define _BASIC_BLOCK_H
#include "Instruction.hh"
#include "LabelManager.hh"
#include "LinkedList.hh"
#include "Value.hh"

class BasicBlock : public Value {
 private:
  LinkedList<Instruction*> instructions;
  Instruction* tail;
  bool empty;
  Function* function;

 public:
  BasicBlock(string name_)
      : Value(nullptr, LabelManager::getLabel(name_), VT_BB),
        tail(nullptr),
        empty(false) {}
  BasicBlock(string name_, bool empty_)
      : Value(nullptr, LabelManager::getLabel(name_), VT_BB),
        tail(nullptr),
        empty(empty_) {}
  ~BasicBlock();
  bool pushInstr(Instruction* i);
  void pushInstrAtHead(Instruction* i);
  void pushInstrBefore(Instruction* i, LinkedList<Instruction*>::Iterator iter);
  void printIR(ostream& stream) const override;
  Instruction* getTailInstr() { return tail; }
  void setTailInstr(Instruction* tail_) { tail = tail_; }
  Function* getParent() { return function; }
  void setParent(Function* func) { function = func; }
  void eraseFromParent();
  const LinkedList<Instruction*>* getInstructions() const {
    return &instructions;
  }
  LinkedList<Instruction*>* getInstructions() { return &instructions; }
  uint32_t getInstrSize() const { return instructions.getSize(); }
  Instruction* getInstructionAt(uint32_t idx) { return instructions.at(idx); }
  BasicBlock* clone(unordered_map<Value*, Value*>& replaceMap);
  bool isEmpty() { return empty; }
  BasicBlock* split(LinkedList<Instruction*>::Iterator iter);
  BasicBlock* splitBlockPredecessors(vector<BasicBlock*>& preds,
                                     unordered_map<Value*, Value*>& phiMap);
};

struct BasicBlockPtrHash {
  std::size_t operator()(const BasicBlock* bb) const {
    return std::hash<const BasicBlock*>()(bb);
  }
};

struct BasicBlockPtrEqual {
  bool operator()(const BasicBlock* lhs, const BasicBlock* rhs) const {
    return lhs == rhs;
  }
};

#endif