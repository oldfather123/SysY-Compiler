#include "Reassociate.hh"

#include "DeadCodeElimination.hh"

bool Reassociate::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  for (Function* function : *module->getFunctions()) {
    changed |= runOnFunction(function);
  }
  return changed;
}

bool Reassociate::runOnFunction(Function* func) {
  bool changed = false;
  for (BasicBlock* block : *func->getBasicBlocks()) {
    changed |= runOnBasicBlock(block);
  }
  if (changed) {
    DeadCodeElimination dce;
    dce.runOnFunction(func);
  }
  return changed;
}

bool Reassociate::runOnBasicBlock(BasicBlock* block) {
  bool changed = false;
  unordered_map<Instruction*, vector<Value*>> argsMap;
  auto instrList = block->getInstructions();
  for (auto it = instrList->begin(); it != instrList->end();) {
    Instruction* instr = *it;
    ++it;
    BinaryOpInst* bopInstr = dynamic_cast<BinaryOpInst*>(instr);
    if (!bopInstr) continue;
    OpTag op = bopInstr->getOpTag();
    if (!(op == ADD || op == MUL || op == AND || op == OR)) continue;
    auto& args = argsMap[bopInstr];
    int argSize = bopInstr->getRValueSize();
    for (int i = 0; i < argSize; i++) {
      Value* arg = bopInstr->getRValue(i);
      Instruction* argInstr = dynamic_cast<Instruction*>(arg);
      if (argInstr && canReduce(bopInstr, arg)) {
        for (Value* subArg : argsMap[argInstr]) {
          args.push_back(subArg);
        }
      } else {
        args.push_back(arg);
      }
    }

    // self reduce
    if (args.size() <= 2) continue;
    bool flag = true;
    for (Use* use = instr->getUseHead(); use; use = use->next) {
      Instruction* userInstr = use->instr;
      BinaryOpInst* userBop = dynamic_cast<BinaryOpInst*>(userInstr);
      if (!userBop || !canReduce(userBop, instr)) {
        flag = false;
        break;
      }
    }
    unordered_map<const Value*, int> idx;
    int i = 0;
    for (auto arg : args) {
      idx[arg] = i++;
    }
    if (!flag) {
      changed = true;
      std::sort(args.begin(), args.end(),
                [&](const Value* lhs, const Value* rhs) {
                  if (lhs->isa(VT_INTCONST) && !rhs->isa(VT_INTCONST))
                    return true;
                  if (rhs->isa(VT_INTCONST) && !lhs->isa(VT_INTCONST))
                    return false;
                  if (lhs->isa(VT_PHI) && !rhs->isa(VT_PHI)) return false;
                  if (!lhs->isa(VT_PHI) && rhs->isa(VT_PHI)) return true;

                  return idx[lhs] < idx[rhs];
                });
      int constValue = 0;
      int initValue = 0;
      switch (op) {
        case MUL:
          initValue = constValue = 1;
          break;
        case AND:
          initValue = constValue = -1;
          break;
      }
      Value* varInstr = 0;
      for (Value* arg : args) {
        if (IntegerConstant* intConst = dynamic_cast<IntegerConstant*>(arg)) {
          switch (op) {
            case ADD:
              constValue += intConst->getValue();
              break;
            case MUL:
              constValue *= intConst->getValue();
              break;
            case AND:
              constValue &= intConst->getValue();
              break;
            case OR:
              constValue |= intConst->getValue();
              break;
            default:
              break;
          }
        } else {
          if (!varInstr) {
            varInstr = arg;
          } else {
            switch (op) {
              case ADD:
                varInstr = new BinaryOpInst(ADD, varInstr, arg, "re.add");
                block->pushInstrBefore((Instruction*)varInstr, it);
                break;
              case MUL:
                varInstr = new BinaryOpInst(MUL, varInstr, arg, "re.mul");
                block->pushInstrBefore((Instruction*)varInstr, it);
                break;
              case AND:
                varInstr = new BinaryOpInst(AND, varInstr, arg, "re.mul");
                block->pushInstrBefore((Instruction*)varInstr, it);
                break;
              case OR:
                varInstr = new BinaryOpInst(OR, varInstr, arg, "re.mul");
                block->pushInstrBefore((Instruction*)varInstr, it);
                break;
              default:
                break;
            }
          }
        }
      }
      Value* constArg = IntegerConstant::getConstInt(constValue);

      if (constArg && varInstr) {
        switch (op) {
          case ADD:
            varInstr = new BinaryOpInst(ADD, varInstr, constArg, "re.add");
            block->pushInstrBefore((Instruction*)varInstr, it);
            break;
          case MUL:
            varInstr = new BinaryOpInst(MUL, varInstr, constArg, "re.mul");
            block->pushInstrBefore((Instruction*)varInstr, it);
            break;
          case AND:
            varInstr = new BinaryOpInst(AND, varInstr, constArg, "re.mul");
            block->pushInstrBefore((Instruction*)varInstr, it);
            break;
          case OR:
            varInstr = new BinaryOpInst(OR, varInstr, constArg, "re.mul");
            block->pushInstrBefore((Instruction*)varInstr, it);
            break;
          default:
            break;
        }
        instr->replaceAllUsesWith(varInstr);
      } else if (!varInstr) {
        instr->replaceAllUsesWith(constArg);
      } else {
        assert(varInstr);
        instr->replaceAllUsesWith(varInstr);
      }
    }
  }
  return changed;
}

bool Reassociate::canReduce(BinaryOpInst* instr, Value* arg) {
  int i = arg->getUserSize();
  BinaryOpInst* bopArg = dynamic_cast<BinaryOpInst*>(arg);
  return bopArg && bopArg->getOpTag() == instr->getOpTag() &&
         bopArg->getParent() == instr->getParent() && i == 1;
}
