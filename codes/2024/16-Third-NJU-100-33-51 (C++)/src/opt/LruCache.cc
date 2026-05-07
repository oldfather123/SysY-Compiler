#include "LruCache.hh"
#define CACHE_SIZE 1024

bool LruCache::runOnModule(ANTPIE::Module* module) {
  bool changed = false;
  replaceCacheFuncs.clear();
  for (Function* func : *module->getFunctions()) {
    changed |= runOnFunction(func);
  }

  for (Function* func : *module->getFunctions()) {
    for (BasicBlock* block : *func->getBasicBlocks()) {
      for (Instruction* inst : *block->getInstructions()) {
        CallInst* callInst = dynamic_cast<CallInst*>(inst);
        if (!callInst) continue;
        Function* callee = callInst->getFunction();
        auto it = replaceCacheFuncs.find(callee);
        if (it != replaceCacheFuncs.end() && func != it->second) {
          callInst->setFunction(it->second);
        }
      }
    }
  }
  return changed;
}

/**
 * If a function is pure function
 * and parameters is (int, int),
 * add a cache for it.
 *
 * int foo(int x, int y) {
 *    body
 * }
 * =>
 * int x_cache[CACHE_SIZE] = {1};
 * int y_cache[CACHE_SIZE];
 * int v_cache[CACHE_SIZE];
 * int foo_cache(int x, int y) {
 *     int loc = (x + y) & (CACHE_SIZE - 1);
 *     if (x_cache[loc] == x && y_cache[loc] == y) {
 *       return v_cheche[loc];
 *     }
 *     x_cache[loc] = x;
 *     y_cache[loc] = y;

 *     int retval = foo(x, y);
 *     v_cache[loc] = retval;
 *     return retval;
 *
 * }
 *
 */

bool LruCache::runOnFunction(Function* func) {
  if (func->isCacheFunction()) return false;
  if (!func->isPureFunction() || !func->isRecursive()) return false;
  FuncType* funcType = (FuncType*)func->getType();
  if (funcType->getArgSize() != 2) return false;
  Argument* args[2] = {funcType->getArgument(0), funcType->getArgument(1)};
  if ((!args[0]->isInteger() && !args[0]->isFloat()) ||
      (!args[1]->isInteger() && !args[1]->isFloat()))
    return false;
  vector<Argument*> cacheArgs = {new Argument("x", args[0]->getType()),
                                 new Argument("y", args[1]->getType())};

  // build cache function
  funcType = FuncType::getFuncType(funcType->getRetType(), cacheArgs);
  auto module = func->getParent();
  Function* cacheFunc =
      module->addFunction(funcType, func->getName() + "_cache");

  // build cache array

  ArrayType* arrayTypex = new ArrayType(CACHE_SIZE, args[0]->getType());
  ArrayConstant* arrayConst = ArrayConstant::getConstArray(arrayTypex);
  if (args[0]->isInteger()) {
    arrayConst->put(0, IntegerConstant::getConstInt(1));
  } else {
    arrayConst->put(0, FloatConstant::getConstFloat(1));
  }
  auto xArray = module->addGlobalVariable(arrayTypex, func->getName() + ".x");
  auto yArray = module->addGlobalVariable(
      new ArrayType(CACHE_SIZE, args[1]->getType()), func->getName() + ".y");
  auto vArray = module->addGlobalVariable(
      new ArrayType(CACHE_SIZE, funcType->getRetType()),
      func->getName() + ".v");

  // build basicblocks
  BasicBlock* entryBlock = new BasicBlock("entry");
  BasicBlock* lhsTrueBlock = new BasicBlock("land.lhs.true");
  BasicBlock* ifThenBlock = new BasicBlock("if.then");
  BasicBlock* ifEndBlock = new BasicBlock("if.end");

  // build instruction in entry
  Value* lhs = cacheArgs[0];
  Value* rhs = cacheArgs[1];
  if (lhs->isFloat()) {
    lhs = new FptosiInst(lhs, "f2i");
    entryBlock->pushInstr((Instruction*)lhs);
  }
  if (rhs->isFloat()) {
    rhs = new FptosiInst(rhs, "f2i");
    entryBlock->pushInstr((Instruction*)rhs);
  }
  auto addBop = new BinaryOpInst(ADD, lhs, rhs, "add");
  auto andBop = new BinaryOpInst(
      AND, addBop, IntegerConstant::getConstInt(CACHE_SIZE - 1), "and");
  auto arrayidx = new GetElemPtrInst(xArray, IntegerConstant::getConstInt(0),
                                     andBop, "arrayidx");
  auto loadx = new LoadInst(arrayidx, "loadx");
  Instruction* cmp = nullptr;
  if (loadx->isInteger()) {
    cmp = new IcmpInst(EQ, loadx, cacheArgs[0], "cmp");
  } else {
    cmp = new FcmpInst(OEQ, loadx, cacheArgs[0], "cmp");
  }
  auto entryBr = new BranchInst(cmp, lhsTrueBlock, ifEndBlock);

  entryBlock->pushInstr(addBop);
  entryBlock->pushInstr(andBop);
  entryBlock->pushInstr(arrayidx);
  entryBlock->pushInstr(loadx);
  entryBlock->pushInstr(cmp);
  entryBlock->pushInstr(entryBr);
  cacheFunc->pushBasicBlock(entryBlock);
  // cacheFunc->setEntry(entryBlock);

  // build instructions in lhsTrueBlock
  auto arrayidx2 = new GetElemPtrInst(yArray, IntegerConstant::getConstInt(0),
                                      andBop, "arrayidx");
  auto loady = new LoadInst(arrayidx2, "loady");
  Instruction* cmp3 = nullptr;
  if (loady->isInteger()) {
    cmp3 = new IcmpInst(EQ, loady, cacheArgs[1], "cmp");
  } else {
    cmp3 = new FcmpInst(OEQ, loady, cacheArgs[1], "cmp");
  }
  auto ltBr = new BranchInst(cmp3, ifThenBlock, ifEndBlock);

  Function* putint = 0;
  Function* putch = 0;
  for (Function* extFunc : *module->getexternFunctions()) {
    if (extFunc->getName() == "putint") {
      putint = extFunc;
    }
    if (extFunc->getName() == "putch") {
      putch = extFunc;
    }
  }

  lhsTrueBlock->pushInstr(arrayidx2);
  lhsTrueBlock->pushInstr(loady);
  lhsTrueBlock->pushInstr(cmp3);
  lhsTrueBlock->pushInstr(ltBr);
  cacheFunc->pushBasicBlock(lhsTrueBlock);

  // build instructions in ifThenBlock
  auto arrayidx5 = new GetElemPtrInst(vArray, IntegerConstant::getConstInt(0),
                                      andBop, "arrayidx");
  auto loadv = new LoadInst(arrayidx5, "loadv");
  // vector<Value*> ta = {loadx};
  // auto outx = new CallInst(putint, ta, "");
  // ta = {IntegerConstant::getConstInt(32)};
  // auto outsp1 = new CallInst(putch, ta, "");
  // ta = {loady};
  // auto outy = new CallInst(putint, ta, "");
  // ta = {IntegerConstant::getConstInt(32)};
  // auto outsp2 = new CallInst(putch, ta, "");
  // ta = {loadv};
  // auto outv = new CallInst(putint, ta, "");
  // ta = {IntegerConstant::getConstInt(20)};
  // auto outsp3 = new CallInst(putch, ta, "");
  auto dirReturn = new ReturnInst(loadv);
  ifThenBlock->pushInstr(arrayidx5);
  ifThenBlock->pushInstr(loadv);
  // ifThenBlock->pushInstr(outx);
  // ifThenBlock->pushInstr(outsp1);
  // ifThenBlock->pushInstr(outy);
  // ifThenBlock->pushInstr(outsp2);
  // ifThenBlock->pushInstr(outv);
  // ifThenBlock->pushInstr(outsp3);
  ifThenBlock->pushInstr(dirReturn);
  cacheFunc->pushBasicBlock(ifThenBlock);

  // build instructionn in ifEndBlock
  vector<Value*> callArgs = {cacheArgs[0], cacheArgs[1]};
  auto call = new CallInst(func, callArgs, "call");
  auto storex = new StoreInst(cacheArgs[0], arrayidx);
  auto arrayidx9 = new GetElemPtrInst(yArray, IntegerConstant::getConstInt(0),
                                      andBop, "arrayidx");
  auto storey = new StoreInst(cacheArgs[1], arrayidx9);
  auto arrayidx12 = new GetElemPtrInst(vArray, IntegerConstant::getConstInt(0),
                                       andBop, "arrayidx");
  auto storev = new StoreInst(call, arrayidx12);
  auto ret = new ReturnInst(call);

  ifEndBlock->pushInstr(call);
  ifEndBlock->pushInstr(storex);
  ifEndBlock->pushInstr(arrayidx9);
  ifEndBlock->pushInstr(storey);
  ifEndBlock->pushInstr(arrayidx12);
  ifEndBlock->pushInstr(storev);
  ifEndBlock->pushInstr(ret);
  cacheFunc->pushBasicBlock(ifEndBlock);

  cacheFunc->setCacheFunction(true);
  replaceCacheFuncs[func] = cacheFunc;
  cacheFunc->buildCFG();
  return true;
}