/**
 * early common subexpression elimination
 */

#ifndef _CSE_H_
#define _CSE_H_

#include <map>

#include "Optimization.hh"
using std::map;

class CommonSubexpElimination : public Optimization {
 private:
  bool isSimpleExpr(Instruction* instr);
  string hashToString(Instruction* instr);
  bool cseDfs(BasicBlock* block);

  // A node to store exprs defined in a block
  class InfoNode {
   private:
    InfoNode* parent;
    map<string, Instruction*> hashToInstr;

   public:
    InfoNode() : parent(0){};
    InfoNode(InfoNode* parent_) : parent(parent_){};

    Instruction* findInstruction(string& hashStr) {
      auto item = hashToInstr.find(hashStr);
      if (item != hashToInstr.end()) {
        return item->second;
      }
      if (parent) {
        return parent->findInstruction(hashStr);
      }
      return nullptr;
    }

    InfoNode* getParent() { return parent; }

    void pushInstruction(string hashStr, Instruction* instr) {
      hashToInstr[hashStr] = instr;
    }
  };

  InfoNode* currNode;
  DomTree* domTree;
  LinkedList<Instruction*> trashList;

  void pushNode() { currNode = new InfoNode(currNode); }

  void popNode() {
    assert(currNode);
    InfoNode* old = currNode;
    currNode = currNode->getParent();
    delete old;
  }

 public:
  CommonSubexpElimination() : currNode(0){};
  bool runOnModule(ANTPIE::Module* module) override;
  bool runOnFunction(Function* func) override;
};

#endif