/**
 * Function ir
 * Create by Zhang Junbin at 2024/6/2
 */

#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "AliasAnalysisResult.hh"
#include "BasicBlock.hh"
#include "DomTree.hh"
#include "Type.hh"
#include "Value.hh"

struct LoopInfoBase;
class CFG;
namespace ANTPIE {
class Module;
}

class Function : public GlobalValue {
 private:
  bool isCache = false;
  bool external = 0;
  // Basic structure
  LinkedList<BasicBlock*> basicBlocks;
  BasicBlock *entry, *exit;
  ANTPIE::Module* module;
  unordered_set<CallInst*> callSites;

  // Analysis result structure
  CFG* cfg = 0;
  DomTree* dt = 0;
  AliasAnalysisResult* aaResult;
  LoopInfoBase* loopInfoBase;
  struct FunctionProperties {
    bool isRecursive : 1;
    bool hasSideEffects : 1;
    bool pureFunction : 1;
    bool memRead : 1;
    bool memWrite : 1;
    size_t lineSize;
    unordered_set<Function*> callerFunctions;
    unordered_set<Function*> calledFunctions;

    FunctionProperties()
        : isRecursive(false), hasSideEffects(false), pureFunction(true) {}

    void addCalled(Function* func) { calledFunctions.insert(func); }

    void addCaller(Function* func) { callerFunctions.insert(func); }

  } props;

 public:
  Function(FuncType* fType, string name);
  Function(FuncType* fType, bool isExt, string name);
  ~Function();
  void pushBasicBlock(BasicBlock* bb);
  void pushBasicBlockAtHead(BasicBlock* bb);
  void insertBasicBlockBefore(BasicBlock* newBlock, BasicBlock* loc);
  void printIR(ostream& stream) const override;
  void setParent(ANTPIE::Module* module_) { module = module_; }
  ANTPIE::Module* getParent() const { return module; }
  bool isExtern() const { return external; }

  LinkedList<BasicBlock*>* getBasicBlocks() { return &basicBlocks; }

  BasicBlock* getEntry() const { return entry; }
  BasicBlock* getExit() const { return exit; }

  // Control Flow Graph
  CFG* buildCFG();
  CFG* getCFG() { return cfg; }
  void resetCFG();

  // Dominance Tree
  DomTree* buildDT();
  DomTree* getDT() { return dt; }
  void resetDT();

  // Function propertites analysis
  bool isRecursive() { return props.isRecursive; }
  bool hasSideEffects() { return props.hasSideEffects; }
  bool isPureFunction() { return props.pureFunction; }
  bool hasMemRead() { return props.memRead; }
  bool hasMemWrite() { return props.memWrite; }
  size_t getLineSize() { return props.lineSize; }

  void setRecursive(bool isRecure) { props.isRecursive = isRecure; }
  void setSideEffects(bool sideEffect) { props.hasSideEffects = sideEffect; }
  void setPureFunction(bool pureFunction) { props.pureFunction = pureFunction; }
  void setMemRead(bool mr) { props.memRead = mr; }
  void setMemWrite(bool mw) { props.memWrite = mw; }
  void setSize(size_t size) { props.lineSize = size; }

  unordered_set<Function*>& getCallers() { return props.callerFunctions; }
  unordered_set<Function*>& getCallees() { return props.calledFunctions; }
  void clearCall() {
    props.callerFunctions.clear();
    props.calledFunctions.clear();
  }

  static void addCall(Function* caller, Function* callee) {
    caller->props.addCalled(callee);
    callee->props.addCaller(caller);
  }

  // Alias Analysis
  AliasAnalysisResult* getAliasAnalysisResult() { return aaResult; }
  void setAliasAnalysisResult(AliasAnalysisResult* ret) { aaResult = ret; }

  // Loop Analysis
  LoopInfoBase* getLoopInfoBase() { return loopInfoBase; }
  void setLoopInfoBase(LoopInfoBase* LI) { loopInfoBase = LI; }

  bool isCacheFunction() { return isCache; }
  void setCacheFunction(bool c) { isCache = c; }

  void addCallSite(CallInst* callInst) { callSites.insert(callInst); }
  void deleteCallSite(CallInst* callInst) { callSites.erase(callInst); }
  unordered_set<CallInst*>* getCallSites() { return &callSites; }
};
#endif