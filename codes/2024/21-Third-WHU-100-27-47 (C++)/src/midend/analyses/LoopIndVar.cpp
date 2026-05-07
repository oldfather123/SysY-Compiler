#include "LoopIndVar.h"
#include "LoopDetection.h"
#include "LoopInvariantAnalysis.h"

using namespace ir;

bool LoopInductionVariableAnalysis::fitPattern(Ptr<PhiInst> phiNode, Value val, Value destReg, Ptr<Loop> loop) {
    if (!val.isRegister()) {
        return false;
    }
    if (phiNode->destReg() == val.getRegister()) {
        for (auto inEdge : phiNode->parentBB()->inEdges()) {
            if (inEdge->getCopySrcVal(phiNode) == destReg) {
                continue;
            } else {
                if (loop->bbs().count(inEdge->src()) != 0) {
                    return false;
                }
            }
        }
    } else {
        return false;
    }
    return true;
}
// TODO: handle the initial value of branching variable
// for example, if b = phi [a : a bb in the loop, others : b = a +/- c],
// then we can have the conclusion that a is the initial value of b is a +/- c
// so we need to handle the initial value of b that was defined outside of the loop
Value handleInitialValue(Ptr<PhiInst> phiNode, Value val, Value destReg, Ptr<Loop> loop) {
    auto preHeaderBB = loop->preheaderBB();
    if (!preHeaderBB || preHeaderBB == loop->headerBB()->parentFunc()->entryBB()) {
        loop->createNewPreheaderBB();
    }
    if (phiNode->destReg() == val.getRegister()) {
        for (auto inEdge : phiNode->parentBB()->inEdges()) {
            if (inEdge->getCopySrcVal(phiNode) == destReg) {
                continue; // skip the edge that comes from the loop
            } else {
                auto inBB = inEdge->src();
                auto inVal = inEdge->getCopySrcVal(phiNode);
                if (inVal.isLiteral()) {
                    return inVal;
                }
            }
        }
    }
    return Value{};
}
Value stepChecker(Value step, OperInst::Operator op) {
    // 用于处理步长的正负号，加法则为正，减法则为负
    if (step.isLiteral()) {
        if (op == OperInst::Operator::Add) {
            return step;
        } else if (op == OperInst::Operator::Sub) {
            if (step.getLiteral().isInt()) {
                return Value(Literal(-step.getLiteral().getInt()));
            } else if (step.getLiteral().isFloat()) {
                return Value(Literal(-step.getLiteral().getFloat()));
            }
        }
    }
    return step;
}
// we assume that all basic induction variable fit phi pattern*:
//
// phi pattern:
//
// ----------
// loop headerBB : phiNodes
// b = phi [a : a bb in the loop, others : none of them comes from a bb in the loop]
// ----------
// loop body : add or sub operInst
// a = b +/- e
// (e is invariant)
// ----------
// then we can have the conclusion that a and b are both basic induction variable
//
// so the way to inplement the algorithm to find basci induction variable :
// 1. find an add or sub instruction
// 2. look for loop headerBB's all inedge
// 3. confirm that only one edge comes from loop'bb
// the following function finds the basic induction variable in a loop
void LoopInductionVariableAnalysis::findBasicIndVar(Ptr<Loop> loop) {
    Set<RegPtr> basicIndVars;

    auto phiNodes = loop->headerBB()->getPhiNodes();
    for (auto bb : loop->bbs()) {
        for (auto inst : bb->getInstSet()) {
            if (auto operInst = castPtr<OperInst>(inst)) {
                if (operInst->isBinary()) {
                    if (operInst->op() == OperInst::Operator::Add || operInst->op() == OperInst::Operator::Sub) {
                        // try to fit phi pattern
                        if (this->invariantCtx->isInvariant(operInst->lhs()) && operInst->rhs().isRegister()) {
                            for (auto phiNode : phiNodes) {
                                if (fitPattern(phiNode, operInst->rhs(), operInst->destReg(), loop)) {
                                    basicIndVars.insert(operInst->rhs().getRegister());
                                    basicIndVars.insert(operInst->destReg());
                                    auto step = stepChecker(operInst->lhs(), operInst->op());
                                    BIVStep.insert({operInst->rhs().getRegister(), step});
                                    BIVStep.insert({operInst->destReg(), step});
                                    auto initialValue = handleInitialValue(phiNode, operInst->rhs(), operInst->destReg(), loop);
                                    BIVIniVal.insert({operInst->rhs().getRegister(), initialValue});
                                    auto modifiedVal = ir::foldOperation(operInst->op(), initialValue.getLiteral(), operInst->lhs().getLiteral());
                                    BIVIniVal.insert({operInst->destReg(), modifiedVal});
                                }
                            }
                        } else if (this->invariantCtx->isInvariant(operInst->rhs()) && operInst->lhs().isRegister()) {
                            for (auto phiNode : phiNodes) {
                                if (fitPattern(phiNode, operInst->lhs(), operInst->destReg(), loop)) {
                                    basicIndVars.insert(operInst->lhs().getRegister());
                                    basicIndVars.insert(operInst->destReg());
                                    auto step = stepChecker(operInst->rhs(), operInst->op());
                                    BIVStep.insert({operInst->lhs().getRegister(), step});
                                    BIVStep.insert({operInst->destReg(), step});
                                    auto initialValue = handleInitialValue(phiNode, operInst->lhs(), operInst->destReg(), loop);
                                    BIVIniVal.insert({operInst->lhs().getRegister(), initialValue});
                                    auto modifiedVal = ir::foldOperation(operInst->op(), initialValue.getLiteral(), operInst->rhs().getLiteral());
                                    BIVIniVal.insert({operInst->destReg(), modifiedVal});
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    this->basicIndVar = basicIndVars;
}

Value computeStep(Value step, Value invariant, OperInst::Operator op) {
    // 分两种情况：
    // 1. j = i +/- c
    // then j's step equal to i's step
    if (op == OperInst::Operator::Add || op == OperInst::Operator::Sub) {
        return step;
    }
    // 2. j = k mul/div c
    // j's step equal to k's step mul / div c
    auto stepLiteral = step.getLiteral();
    auto invariantLiteral = invariant.getLiteral();
    return ir::foldOperation(op, stepLiteral, invariantLiteral);
}

Value computeInitialValue(Value init, Value lhs, OperInst::Operator op) {
    auto initLiteral = init.getLiteral();
    auto lhsLiteral = lhs.getLiteral();
    return ir::foldOperation(op, initLiteral, lhsLiteral);
}
void LoopInductionVariableAnalysis::findIndVar(Ptr<Loop> loop) {
    auto basicIndvars = this->basicIndVar;
    Set<RegPtr> indVars = {basicIndvars.begin(), basicIndvars.end()};
    IVIniVal = BIVIniVal;
    IVStep = BIVStep;

    FixedPoint{
        [&](bool &changed) {
            for (auto bb : loop->bbs()) {
                for (auto inst : bb->getInstSet()) {
                    if (auto operInst = castPtr<OperInst>(inst)) {
                        if (operInst->isBinary() && (operInst->op() == OperInst::Operator::Add || operInst->op() == OperInst::Operator::Sub || operInst->op() == OperInst::Operator::Mul || operInst->op() == OperInst::Operator::Div)) {
                            // 检查操作数是否为归纳变量和循环不变量的组合
                            if (this->invariantCtx->isInvariant(operInst->lhs()) && operInst->rhs().isRegister()) {
                                auto rhsReg = operInst->rhs().getRegister();
                                if (indVars.find(rhsReg) != indVars.end()) {
                                    // 将新发现的归纳变量加入集合
                                    if (indVars.insert(operInst->destReg()).second) {
                                        // 查找rhsReg的初始值和步长
                                        auto stepVal = IVStep.at(rhsReg);
                                        auto initVal = IVIniVal.at(rhsReg);
                                        if (stepVal.isLiteral() && initVal.isLiteral() && operInst->lhs().isLiteral()) {
                                            // 计算新值
                                            Value newStepVal = computeStep(stepVal, operInst->lhs(), operInst->op());
                                            IVStep.insert({operInst->destReg(), newStepVal});
                                            Value newInitVal = computeInitialValue(initVal, operInst->lhs(), operInst->op());
                                            IVIniVal.insert({operInst->destReg(), newInitVal});
                                        }
                                        changed = true; // 发现新的依赖归纳变量
                                    }
                                }
                            } else if (this->invariantCtx->isInvariant(operInst->rhs()) && operInst->lhs().isRegister()) {
                                auto lhsReg = operInst->lhs().getRegister();
                                if (indVars.find(lhsReg) != indVars.end()) {
                                    if (indVars.insert(operInst->destReg()).second) {
                                        // 查找lhsReg的初始值和步长
                                        auto stepVal = IVStep.at(lhsReg);
                                        auto initVal = IVIniVal.at(lhsReg);
                                        if (stepVal.isLiteral() && initVal.isLiteral() && operInst->rhs().isLiteral()) {
                                            // 计算新值
                                            Value newStepVal = computeStep(stepVal, operInst->rhs(), operInst->op());
                                            IVStep.insert({operInst->destReg(), newStepVal});
                                            Value newInitVal = computeInitialValue(initVal, operInst->rhs(), operInst->op());
                                            IVIniVal.insert({operInst->destReg(), newInitVal});
                                        }
                                        changed = true; // 发现新的依赖归纳变量
                                    }
                                }
                            }
                        }
                    }
                }
            }
        },
        /* debugPrint = */ true,
        "Loop Induction Variable Analysis",
    };
    this->allIndVar = indVars;
}
