#include "Instruction.h"
#include "Values.h"

using namespace ir;

void Instruction::recompute() {
    if (this->is<MoveInst>()) {
        auto moveInst = this->as<MoveInst>();
        if (moveInst->destReg()->dataType().isPointer() && moveInst->srcVal().isRegister()) {
            moveInst->destReg()->joinAliasGroup(moveInst->srcVal().getRegister());
        }
    } else if (this->is<GEPInst>()) {
        auto gepInst = this->as<GEPInst>();
        if (gepInst->destReg()->dataType().isPointer()) {
            gepInst->destReg()->joinAliasGroup(gepInst->arrPtrReg());
        }
    } else if (this->is<OperInst>()) {
        auto operInst = this->as<OperInst>();
        if (operInst->rhs().isRegister() && operInst->rhs().getRegister()->dataType().isPointer()) {
            operInst->destReg()->joinAliasGroup(operInst->rhs().getRegister());
        } else if (operInst->isBinary() && operInst->lhs().isRegister() && operInst->lhs().getRegister()->dataType().isPointer()) {
            operInst->destReg()->joinAliasGroup(operInst->lhs().getRegister());
        }
    }
}

void Instruction::_init(const InstPtr &self, const BBPtr &parentBB) {
    // Init self
    _self = self;
    // Add to parent basic block
    if (parentBB) {
        parentBB->_addInst(self);
    }
    recompute();
}

void Instruction::replaceReg(RegPtrRef regRef, const Value &value) {
    regRef._replace(value);
}

Set<Variable> Instruction::mustDefVars() {
    Set<Variable> ret{};
    for (auto reg : regsWritten()) {
        ret.insert(Variable::reg(reg));
    }
    if (this->is<AllocInst>()) {
        ret.insert(Variable::mem(this->as<AllocInst>()->destReg()));
    }
    if (this->is<StoreInst>()) {
        ret.insert(Variable::mem(this->as<StoreInst>()->destAddrReg()));
    }
    if (this->is<CallInst>()) {
        auto args = this->as<CallInst>()->argList();
        for (auto arg : args) {
            if (arg.dataType().isPointer() && arg.isRegister()) {
                ret.insert(Variable::mem(arg.getRegister()));
            }
        }
        for (auto global : _parentBB->parentFunc()->parentModule()->getGlobalRegs()) {
            ret.insert(Variable::mem(global));
        }
    }
    return ret;
}
Set<Variable> Instruction::mustUseVars() {
    Set<Variable> ret{};
    for (auto reg : regsRead()) {
        ret.insert(Variable::reg(reg));
    }
    if (this->is<LoadInst>()) {
        ret.insert(Variable::mem(this->as<LoadInst>()->srcAddrReg()));
    }
    if (this->is<CallInst>()) {
        auto args = this->as<CallInst>()->argList();
        for (auto arg : args) {
            if (arg.dataType().isPointer() && arg.isRegister()) {
                ret.insert(Variable::mem(arg.getRegister()));
            }
        }
        for (auto global : _parentBB->parentFunc()->parentModule()->getGlobalRegs()) {
            ret.insert(Variable::mem(global));
        }
    }
    return ret;
}
Set<Variable> Instruction::mayDefVars() {
    Set<Variable> ret = mustDefVars();
    if (this->is<StoreInst>()) {
        ret.insert(Variable::mem(this->as<StoreInst>()->destAddrReg()->findAliasGroup()));
    }
    if (this->is<CallInst>()) {
        auto args = this->as<CallInst>()->argList();
        auto callee = this->as<CallInst>()->function();
        if (!callee->isPure()) {
            for (auto arg : args) {
                if (arg.dataType().isPointer() && arg.isRegister()) {
                    ret.insert(Variable::mem(arg.getRegister()->findAliasGroup()));
                }
            }
            if (!callee->isPrototype()) {
                for (auto global : _parentBB->parentFunc()->parentModule()->getGlobalRegs()) {
                    ret.insert(Variable::mem(global->findAliasGroup()));
                }
            }
        }
    }
    return ret;
}
Set<Variable> Instruction::mayUseVars() {
    Set<Variable> ret = mustUseVars();
    if (this->is<LoadInst>()) {
        ret.insert(Variable::mem(this->as<LoadInst>()->srcAddrReg()->findAliasGroup()));
    }
    if (this->is<CallInst>()) {
        auto args = this->as<CallInst>()->argList();
        auto callee = this->as<CallInst>()->function();
        if (!callee->isPure()) {
            for (auto arg : args) {
                if (arg.dataType().isPointer() && arg.isRegister()) {
                    ret.insert(Variable::mem(arg.getRegister()->findAliasGroup()));
                }
            }
            if (!callee->isPrototype()) {
                for (auto global : _parentBB->parentFunc()->parentModule()->getGlobalRegs()) {
                    ret.insert(Variable::mem(global->findAliasGroup()));
                }
            }
        }
    }
    return ret;
}

bool CallInst::isTailCall() const {
    return this->firstSucc() == _parentBB->exitInst() &&
           _parentBB->exitInst()->isUncondBr() &&
           _parentBB->exitInst()->unconditionalTarget() == _parentBB->parentFunc()->exitBB();
}

void Instruction::replace(const InstPtr &inst, const InstPtr &newInst) {
    dbgassert(inst->_parentBB == newInst->_parentBB, "Cannot replace with an instruction from a different basic block");
    GraphNode::replace(inst, newInst);
    auto parentBB = inst->_parentBB;
    parentBB->_removeInst(inst);
}

void Instruction::remove(const InstPtr &inst) {
    auto inEdges = inst->inEdges();
    auto outEdges = inst->outEdges();
    GraphNode::remove(inst);
    for (auto inEdge : inEdges) {
        auto pred = inEdge->src();
        for (auto outEdge : outEdges) {
            auto succ = outEdge->dest();
            if (pred != inst && succ != inst) {
                Instruction::addEdge(pred, succ);
            }
        }
    }
    auto parentBB = inst->_parentBB;
    parentBB->_removeInst(inst);
}

void Instruction::addTo(const InstPtr &inst, const BBPtr &bb) {
    bb->_addInst(inst);
}

ir::InstPtr ir::Instruction::clone(const InstPtr &inst, const BBPtr &newBB) {
    switch (inst->instType()) {
        case Move: {
            auto oldMove = castPtr<MoveInst>(inst);
            auto moveInst = MoveInst::create(newBB, oldMove->destReg(), oldMove->srcVal());
            return moveInst;
            break;
        }

        case Oper: {
            auto oldOper = castPtr<OperInst>(inst);

            if (oldOper->isBinary()) {
                auto operInst = OperInst::createBinary(newBB, oldOper->destReg(), oldOper->op(), oldOper->lhs(), oldOper->rhs());
                return operInst;
                break;
            } else {
                auto operInst = OperInst::createUnary(newBB, oldOper->destReg(), oldOper->op(), oldOper->rhs());
                return operInst;
                break;
            }
        }

        case Call: {
            auto oldCall = castPtr<CallInst>(inst);
            auto callInst = CallInst::create(newBB, oldCall->destReg(), oldCall->function(), oldCall->argList());
            return callInst;
            break;
        }

        case Load: {
            auto oldLoad = castPtr<LoadInst>(inst);
            auto loadInst = LoadInst::create(newBB, oldLoad->destReg(), oldLoad->srcAddrReg());
            return loadInst;
            break;
        }

        case Store: {
            auto oldStore = castPtr<StoreInst>(inst);
            auto storeInst = StoreInst::create(newBB, oldStore->srcVal(), oldStore->destAddrReg());
            return storeInst;
            break;
        }

        case Alloc: {
            auto allocInst = AllocInst::create(newBB, *castPtr<AllocInst>(inst)->regsWritten().begin());
            return allocInst;
            break;
        }

        case Cast: {
            auto castInst = CastInst::create(newBB, castPtr<CastInst>(inst)->destReg(), castPtr<CastInst>(inst)->srcVal());
            return castInst;
            break;
        }

        case GEP: {
            auto oldGEPInst = castPtr<GEPInst>(inst);
            auto gepInst = GEPInst::create(newBB, oldGEPInst->destReg(), oldGEPInst->arrPtrReg(), oldGEPInst->indexVal());
            return gepInst;
            break;
        }

        case Phi: {
            auto phiInst = PhiInst::create(newBB, castPtr<PhiInst>(inst)->destReg());
            for (auto inEdge : inst->_parentBB->inEdges()) {
                if (inEdge->isFake()) {
                    continue;
                }
                inEdge->addParallelCopy(phiInst, inEdge->getCopySrcVal(castPtr<PhiInst>(inst)));
            }
            return phiInst;
            break;
        }

        case Ret: {
            auto retInst = RetInst::create(newBB, castPtr<ir::RetInst>(inst)->retVal());
            return retInst;
            break;
        }

        case Exit: {
            auto exitInst = ExitInst::create(newBB);
            // 这里应该要对控制流图进行修改
            return exitInst;
            break;
        }
        case PseudoEntry: {
            auto psInst = PseudoInst::create(newBB, inst->as<PseudoInst>()->instType());
            return psInst;
            break;
        }

        default:
            dbgassert(false, "can not clone unknown instruction");
            return nullptr;
    }
}

void Instruction::insertAfter(const InstPtr &inst, const InstPtr &newInst) {
    dbgassert(inst->_parentBB == newInst->_parentBB, "Cannot insert instruction into a different basic block");
    GraphNode::insertAfter(inst, newInst);
}

void Instruction::insertBefore(const InstPtr &inst, const InstPtr &newInst) {
    dbgassert(inst->_parentBB == newInst->_parentBB, "Cannot insert instruction into a different basic block");
    GraphNode::insertBefore(inst, newInst);
}
