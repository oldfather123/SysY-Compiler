#ifndef _CFG_H_
#define _CFG_H_

#include <map>

#include "BasicBlock.hh"
#include "VisualizeGraph.hh"
using std::map;

typedef LinkedList<BasicBlock*>* BBListPtr;
class Function;

class CFG {
 private:
  LinkedList<BasicBlock*> blocks;
  BasicBlock *entry, *exit;
  map<BasicBlock*, BBListPtr> blkPredMap;
  map<BasicBlock*, BBListPtr> blkSuccMap;

 public:
  CFG() {}
  CFG(Function* func);
  BBListPtr getPredOf(BasicBlock* bb) const { return blkPredMap.at(bb); }
  BBListPtr getSuccOf(BasicBlock* bb) const { return blkSuccMap.at(bb); }
  BBListPtr getBlocks() { return &blocks; }
  BasicBlock* getEntry() const { return entry; }
  BasicBlock* getExit() const { return exit; }
  void setEntry(BasicBlock* entry_) { entry = entry_; }
  void setExit(BasicBlock* exit_) { exit = exit_; }

  void addNode(BasicBlock* bb);
  void addEdge(BasicBlock* src, BasicBlock* dest);

  void eraseNode(BasicBlock* bb);
  void eraseEdge(BasicBlock* src, BasicBlock* dest);
  
  void debug();
  void draw();
};

#endif