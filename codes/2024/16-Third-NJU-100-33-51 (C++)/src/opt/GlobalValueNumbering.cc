#include "GlobalValueNumbering.hh"

bool GlobalValueNumbering::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* function : *module->getFunctions()) {
    changed |= runOnFunction(function);
  }
  return changed;
}

bool GlobalValueNumbering::runOnFunction(Function* func) {
  bool changed = false;
  CFG* cfg = func->getCFG();
  if (!cfg) cfg = func->buildCFG();

  vector<BasicBlock*> postOrder;
  unordered_set<BasicBlock*> visited;
  std::function<void(BasicBlock*)> postOrderDfs = [&](BasicBlock* block) {
    for (BasicBlock* succ : *cfg->getSuccOf(block)) {
      if (!visited.count(succ)) {
        visited.insert(succ);
        postOrderDfs(succ);
      }
    }
    postOrder.push_back(block);
  };
  postOrderDfs(func->getEntry());

  unordered_map<string, Instruction*> exprMap;
  for (auto it = postOrder.rbegin(); it != postOrder.rend(); it++) {
    LinkedList<Instruction*>* instrList = (*it)->getInstructions();
    for (auto instrIt = instrList->begin(); instrIt != instrList->end();) {
      Instruction* instr = *instrIt;
      ++instrIt;
      if (!canNumbering(instr)) continue;

      string hashStr = hashToString(instr);
      auto item = exprMap.find(hashStr);
      if (item != exprMap.end()) {
        changed = true;
        instr->replaceAllUsesWith(item->second);
        instr->eraseFromParent();
        instr->deleteUseList();
      } else {
        exprMap.emplace(hashStr, instr);
      }
    }
  }
  return changed;
}

string GlobalValueNumbering::hashToString(Instruction* instr) {
  assert(canNumbering(instr));
  Value* lhs = 0;
  Value* rhs = 0;
  string opName;

  if (instr->isa(VT_ICMP)) {
    IcmpInst* icmp = dynamic_cast<IcmpInst*>(instr);
    lhs = instr->getRValue(0);
    rhs = instr->getRValue(1);
    OpTag opTag = icmp->getOpTag();

    if (opTag == EQ || opTag == NE) {
      if (lhs < rhs) {
        std::swap(lhs, rhs);
      }
    } else if (opTag == SGE) {
      std::swap(lhs, rhs);
      opTag = SLE;
    } else if (opTag == SGT) {
      std::swap(lhs, rhs);
      opTag = SLT;
    }
    opName = instr->getOpName(opTag);

  } else if (instr->isa(VT_FCMP)) {
    FcmpInst* fcmp = dynamic_cast<FcmpInst*>(instr);
    lhs = instr->getRValue(0);
    rhs = instr->getRValue(1);
    OpTag opTag = fcmp->getOpTag();

    if (opTag == OEQ || opTag == ONE) {
      if (lhs < rhs) {
        std::swap(lhs, rhs);
      }
    } else if (opTag == OGE) {
      std::swap(lhs, rhs);
      opTag = OLE;
    } else if (opTag == OGT) {
      std::swap(lhs, rhs);
      opTag = OLT;
    }
    opName = instr->getOpName(opTag);
  } else if (instr->isa(VT_BOP)) {
    BinaryOpInst* bop = dynamic_cast<BinaryOpInst*>(instr);
    lhs = instr->getRValue(0);
    rhs = instr->getRValue(1);
    OpTag opTag = bop->getOpTag();
    if (opTag == ADD || opTag == FADD || opTag == MUL || opTag == FMUL ||
        opTag == AND || opTag == OR || opTag == XOR) {
      if (lhs < rhs) {
        std::swap(lhs, rhs);
      }
    }
    opName = instr->getOpName(opTag);
  } else if (instr->isa(VT_FPTOSI)) {
    FptosiInst* fptosi = dynamic_cast<FptosiInst*>(instr);
    lhs = instr->getRValue(0);
    opName = "fptosi";
  } else if (instr->isa(VT_SITOFP)) {
    SitofpInst* sitofp = dynamic_cast<SitofpInst*>(instr);
    lhs = instr->getRValue(0);
    opName = "sitofp";
  } else if (instr->isa(VT_ZEXT)) {
    ZextInst* zext = dynamic_cast<ZextInst*>(instr);
    lhs = instr->getRValue(0);
    opName = "zext" + zext->getType()->toString();
  } else if (instr->isa(VT_GEP)) {
    GetElemPtrInst* gep = dynamic_cast<GetElemPtrInst*>(instr);
    lhs = instr->getRValue(1);

    if (instr->getRValueSize() == 3) {
      rhs = instr->getRValue(2);
    }
    opName = "gep" + instr->getRValue(0)->toString();
  } 

  if (!rhs) {
    return opName + "|" + lhs->toString();
  } else {
    return opName + "|" + lhs->toString() + "|" + rhs->toString();
  }
}

bool GlobalValueNumbering::canNumbering(Instruction* instr) {
  return instr->isa(VT_ICMP) || instr->isa(VT_FCMP) || instr->isa(VT_BOP) ||
         instr->isa(VT_FPTOSI) || instr->isa(VT_SITOFP) ||
         instr->isa(VT_ZEXT) || instr->isa(VT_GEP);
}