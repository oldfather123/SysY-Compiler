#include "Scopes.h"
#include "Instruction.h"
#include "CFG.h"
#include <queue>
using namespace ir;

const Set<InstPtr> BasicBlock::getInstSet() const {
    Set<InstPtr> ret{_nonPhiInstSet.begin(), _nonPhiInstSet.end()};
    ret.insert(_phiNodes.begin(), _phiNodes.end());
    return ret;
}

Ptr<ExitInst> BasicBlock::exitInst() const {
    auto exitInst = castPtr<ExitInst>(_exitInst);
    dbgassert(exitInst != nullptr, "Exit instruction should be an instance of ExitInst");
    return exitInst;
}

Ptr<CFGEdge> BasicBlock::addUnconditionalEdge(const BBPtr &src, const BBPtr &dest) {
    auto ret = addEdge(src, dest);
    ret->setUncondBr();
    return ret;
}

Ptr<CFGEdge> BasicBlock::addConditionalEdge(const BBPtr &src, const BBPtr &dest, const Value &condition, const bool &condBool) {
    auto ret = addEdge(src, dest);
    ret->setCondBr(condition, condBool);
    return ret;
}

void BasicBlock::replace(const BBPtr &bb, const BBPtr &newBB) {
    GraphNode::replace(bb, newBB);
    auto parentFunc = bb->_parentFunc;
    parentFunc->_removeBB(bb);
    parentFunc->_addBB(newBB);
}

void BasicBlock::remove(const BBPtr &bb) {
    GraphNode::remove(bb);
    auto parentFunc = bb->_parentFunc;
    parentFunc->_removeBB(bb);
}

BBPtr BasicBlock::clone(const FuncPtr &func, const BBPtr &bb, const String &labelPrefix, const String &labelSuffix) {
    auto cloneBB = BasicBlock::create(func, (labelPrefix.empty() ? "" : labelPrefix + ".") + bb->label() + (labelSuffix.empty() ? "" : "." + labelSuffix));
    for (auto inst : bb->getInstTopoList()) {
        if (inst == bb->entryInst() || inst == bb->exitInst()) {
            continue;
        }
        auto newInst = Instruction::clone(inst, cloneBB);
        ir::Instruction::insertBefore(cloneBB->exitInst(), newInst);
    }
    return cloneBB;
}

std::pair<BBPtr, BBPtr> BasicBlock::split(const BBPtr &bb, const InstPtr &splitInst, const String &labelPrefix, const String &labelSuffix) {
    auto newBB = BasicBlock::clone(bb->parentFunc(), bb, labelPrefix, labelSuffix);
    bool pastSplitInst = false;
    auto instList = bb->getInstTopoList();
    auto newInstList = newBB->getInstTopoList();
    for (int i = 0; i < instList.size(); ++i) {
        if (instList[i] == splitInst) {
            pastSplitInst = true;
        }
        if (instList[i] == bb->entryInst() || instList[i] == bb->exitInst()) {
            continue;
        }
        if (pastSplitInst) {
            Instruction::remove(instList[i]);
            newBB->_addInst(instList[i]);
            Instruction::replace(newInstList[i], instList[i]);
        } else {
            Instruction::remove(newInstList[i]);
        }
    }

    // Update CFG
    auto outEdges = bb->outEdges();
    for (auto outEdge : outEdges) {
        outEdge->redirectSrc(newBB);
    }
    auto splitEdge = BasicBlock::addEdge(bb, newBB);
    splitEdge->setUncondBr();

    return {bb, newBB};
}

BBPtr BasicBlock::merge(const BBPtr &bb1, const BBPtr &bb2) {
    dbgassert(bb1->parentFunc() == bb2->parentFunc(), "Can only merge two basic blocks that belong to the same function");
    dbgassert(bb1->succCount() == 1 && bb2->predCount() == 1 && bb1->firstSucc() == bb2 && bb2->firstPred() == bb1, "Basic blocks should satisfy the merge requirements");

    auto exit1 = bb1->exitInst();
    auto entry2 = bb2->entryInst();
    auto exit2 = bb2->exitInst();
    for (auto inst : bb2->getInstTopoList()) {
        bb2->_removeInst(inst, false);
        bb1->_addInst(inst);
    }
    Instruction::addEdge(exit1, entry2);
    bb1->_exitInst = exit2;
    Instruction::remove(exit1);
    Instruction::remove(entry2);

    // Update CFG
    auto outEdges = bb2->outEdges();
    for (auto outEdge : outEdges) {
        outEdge->redirectSrc(bb1);
    }

    // Remove bb2
    BasicBlock::remove(bb2);

    return bb1;
}

void Function::remove(const FuncPtr &func) {
    auto parentModule = func->_parentModule;
    parentModule->_removeFunction(func);
}

FuncPtr ir::Function::clone(const FuncPtr &func) {
    // TODO
    auto cloneFunc = Function::create(func->parentModule(), func->retDataType(), "_clone_" + func->name());

    // a set of all basicblock in original function
    // which is used to decide if an edge would be copied
    Set<BBPtr> bbSet = func->bbSet();
    Map<BBPtr, BBPtr> cloneBB;
    Set<Ptr<CFGEdge>> visitedEdges;

    for (auto bb : func->bbSet()) {
        auto newBB = BasicBlock::clone(cloneFunc, bb);
        cloneBB.insert({bb, newBB});
    }

    for (auto bb : func->bbSet()) {
        if (bb == func->exitBB()) {
            continue;
        }
        for (auto outEdge : bb->outEdges()) {
            if (visitedEdges.find(outEdge) != visitedEdges.end()) {
                continue;
            }
            if (bbSet.find(outEdge->src()) != bbSet.end() && bbSet.find(outEdge->dest()) != bbSet.end()) {
                auto newEdge = BasicBlock::duplicateEdge(outEdge);
                newEdge->redirectSrc(cloneBB[outEdge->src()]);
                newEdge->redirectDest(cloneBB[outEdge->dest()]);
            }
        }
    }
    return cloneFunc;
}

void Function::replace(const FuncPtr &func, const FuncPtr &newFunc) {
    auto parentModule = func->_parentModule;
    parentModule->_removeFunction(func);
    parentModule->_addFunction(newFunc);
}

#define DBG_ASSERT_BB_BOUNDARY(condition) dbgassert(condition, "Traversal exceeds basic block boundary, maybe there are some improperly managed edges towards outside the basic block")

Vector<InstPtr> BasicBlock::getInstTopoList() const {
    auto ret = Instruction::_topoSort(this->_nonPhiInstSet);
    DBG_ASSERT_BB_BOUNDARY(ret.size() == getInstSet().size());
    return ret;
}

BBPtr BasicBlock::create(const FuncPtr &parentFunc, const String &label) {
    auto ret = makePtr<BasicBlock>(label);

    // Add entry pseudo-instruction to the basic block
    ret->_entryInst = PseudoInst::createEntry(ret);
    ret->_exitInst = ExitInst::create(ret);
    Instruction::addEdge(ret->_entryInst, ret->_exitInst);

    if (parentFunc) {
        parentFunc->_addBB(ret);
    }

    return ret;
}

bool BasicBlock::isEmpty() const { return _entryInst == _exitInst || _entryInst->firstSucc() == _exitInst; }

void BasicBlock::_addInst(const InstPtr &inst) {
    dbgassert(inst->parentBB() == nullptr, "Cannot add a instruction when it already belongs to a basic block, remove it from its current parent first");
    auto self = thisPtr(BasicBlock);
    inst->_parentBB = self;
    if (auto phiInst = castPtr<PhiInst>(inst)) {
        _phiNodes.insert(phiInst);
    } else {
        _nonPhiInstSet.insert(inst);
    }
}

void BasicBlock::_removeInst(const InstPtr &inst, bool checkEntryExit) {
#ifdef DEBUG
    if (checkEntryExit) {
        dbgassert(inst != _entryInst && inst != _exitInst, "Invalid operation to remove an entry or exit instruction from a basic block, operate the edges instead of the exit instruction");
    }
#endif
    if (auto phiInst = castPtr<PhiInst>(inst)) {
        _phiNodes.erase(phiInst);
    } else {
        _nonPhiInstSet.erase(inst);
    }
    inst->_parentBB = nullptr;
}

FuncPtr Function::create(const Ptr<Module> &parentModule, const DataType &retDataType, const String &name) {
    auto ret = makePtr<Function>(retDataType, name);
    if (parentModule) {
        parentModule->_addFunction(ret);
    }
    return ret;
}
FuncPtr Function::create(const Ptr<Module> &parentModule, const DataType &retDataType, const String &name, const Vector<RegPtr> &paramList) {
    auto ret = makePtr<Function>(retDataType, name);
    ret->_paramList = paramList;
    if (parentModule) {
        parentModule->_addFunction(ret);
    }
    return ret;
}
FuncPtr Function::createPrototype(const Ptr<Module> &parentModule, const DataType &retDataType, const String &name, const Vector<DataType> &paramDataTypeList) {
    auto ret = create(parentModule, retDataType, name);
    ret->_isPrototype = true;
    for (auto paramDataType : paramDataTypeList) {
        ret->_paramList.push_back(Register::create(ret, paramDataType));
    }
    return ret;
}

#define DBG_ASSERT_FUNC_BOUNDARY(condition) dbgassert(condition, "Traversal exceeds function boundary, maybe there are some improperly managed edges towards outside the function")

void _dfsFunction(const BBPtr &bb, bool reverse, Set<BBPtr> &visited, Vector<BBPtr> &dfsBBList) {
    DBG_ASSERT_FUNC_BOUNDARY(bb->parentFunc() != nullptr);
    if (visited.find(bb) != visited.end()) {
        return;
    }
    visited.insert(bb);
    dfsBBList.push_back(bb);
    if (reverse) {
        for (auto pred : bb->preds()) {
            DBG_ASSERT_FUNC_BOUNDARY(pred->parentFunc() == bb->parentFunc());
            _dfsFunction(pred, reverse, visited, dfsBBList);
        }
    } else {
        for (auto succ : bb->succs()) {
            DBG_ASSERT_FUNC_BOUNDARY(succ->parentFunc() == bb->parentFunc());
            _dfsFunction(succ, reverse, visited, dfsBBList);
        }
    }
}

Vector<BBPtr> Function::dfsBBList(BBPtr startBB, bool reverse) const {
    Vector<BBPtr> ret;
    if (startBB == nullptr) {
        startBB = this->_entryBB;
    }
    Set<BBPtr> visited{};
    _dfsFunction(startBB, reverse, visited, ret);
    DBG_ASSERT_FUNC_BOUNDARY(ret.size() <= this->_bbSet.size());
    return ret;
}

void _bfsFunction(const BBPtr &bb, bool reverse, Set<BBPtr> &visited, Vector<BBPtr> &bfsList) {
    std::queue<BBPtr> q;
    q.push(bb);
    visited.insert(bb);
    while (!q.empty()) {
        auto cur = q.front();
        q.pop();
        DBG_ASSERT_FUNC_BOUNDARY(cur->parentFunc() != nullptr);
        bfsList.push_back(cur);
        if (reverse) {
            for (auto pred : cur->preds()) {
                DBG_ASSERT_FUNC_BOUNDARY(pred->parentFunc() == bb->parentFunc());
                if (visited.find(pred) == visited.end()) {
                    q.push(pred);
                    visited.insert(pred);
                }
            }
        } else {
            for (auto succ : cur->succs()) {
                DBG_ASSERT_FUNC_BOUNDARY(succ->parentFunc() == bb->parentFunc());
                if (visited.find(succ) == visited.end()) {
                    q.push(succ);
                    visited.insert(succ);
                }
            }
        }
    }
}
Vector<BBPtr> Function::bfsBBList(BBPtr startBB, bool reverse) const {
    Vector<BBPtr> ret;
    if (startBB == nullptr) {
        startBB = this->_entryBB;
    }
    Set<BBPtr> visited{};
    _bfsFunction(startBB, reverse, visited, ret);
    DBG_ASSERT_FUNC_BOUNDARY(ret.size() <= this->_bbSet.size());
    return ret;
}

void Function::_addBB(const BBPtr &bb) {
    dbgassert(bb->parentFunc() == nullptr, "Cannot add a basic block when it already belongs to a function, remove it from its current parent first");
    auto self = thisPtr(Function);
    bb->_parentFunc = self;
    this->_bbSet.insert(bb);

    while (_usedBBLabels.find(bb->_label) != _usedBBLabels.end()) {
        bb->_label += "." + std::to_string(_usedBBLabels[bb->_label]++);
    }
    _usedBBLabels[bb->_label] = 1;
}

void Function::_removeBB(const BBPtr &bb) {
    this->_bbSet.erase(bb);
    --_usedBBLabels[bb->_label];
    bb->_parentFunc = nullptr;
}

String Function::getUniqueRegName(const String &name) {
    if (name.empty() && !_usedRegNames.count(name)) {
        _usedRegNames[name] = 1;
    }
    String ret = name;
    while (_usedRegNames.find(ret) != _usedRegNames.end()) {
        if (!name.empty()) {
            ret += ".";
        }
        ret += std::to_string(_usedRegNames[ret]++);
    }
    _usedRegNames[ret] = 1;
    return ret;
}

bool Function::isGlobalInitFunc() const {
    return this == this->parentModule()->_globalInitFunc.get();
}

const Vector<Ptr<GlobalInst>> Module::getGlobalInitInsts() const {
    Vector<Ptr<GlobalInst>> ret{};
    auto globalBB = this->_globalInitFunc->entryBB();
    for (auto inst : globalBB->getInstTopoList()) {
        if (inst == globalBB->entryInst() || inst == globalBB->exitInst() || inst->is<RetInst>()) {
            continue;
        }
        dbgassert(inst->is<GlobalInst>(), "Invalid instruction type in global initialization function");
        ret.push_back(inst->as<GlobalInst>());
    }
    return ret;
}

const Set<RegPtr> Module::getGlobalRegs() const {
    Set<RegPtr> ret{};
    for (auto globalInst : this->getGlobalInitInsts()) {
        dbgassert(globalInst->destReg()->isGlobal(), "Global initialization instruction should define a global variable");
        ret.insert(globalInst->destReg());
    }
    return ret;
}

void Module::_addFunction(const FuncPtr &func) {
    dbgassert(func->parentModule() == nullptr, "Cannot add a global function when it already belongs to a module, remove it from its current parent first");
    auto self = thisPtr(Module);
    func->_parentModule = self;
    this->_funcSet.insert(func);
}

void Module::_removeFunction(const FuncPtr &func) {
    this->_funcSet.erase(func);
    func->_parentModule = nullptr;
}
