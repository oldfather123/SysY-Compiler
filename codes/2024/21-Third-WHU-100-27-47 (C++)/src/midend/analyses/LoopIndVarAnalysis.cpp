#include "LoopIndVarAnalysis.h"

using namespace ir;

ir::LoopIndVarAnalysisContext::LoopIndVarAnalysisContext(LoopInvariantAnalysisContext &loopInvarCtx) : loopInvarCtx(loopInvarCtx) {
    loop = loopInvarCtx.loop;
    auto &cfaCtx = loopInvarCtx.cfaCtx;

    auto fitPattern = [&](RegPtr operandReg, RegPtr destReg) -> bool {
        bool found = false;
        for (auto phi : loop->headerBB()->getPhiNodes()) {
            if (phi->destReg() == operandReg) {
                found = true;
                for (auto tuple : phi->getTuples()) {
                    if (tuple.value() == destReg) {
                        // 源值是归纳变量
                        continue;
                    } else {
                        // 不是归纳变量的源值应当来自循环外
                        if (loop->bbs().count(tuple.bb())) {
                            return false;
                        }
                    }
                }
            }
        }
        return found;
    };

    auto getInitVal = [&](RegPtr reg) -> Value {
        // 确保循环有唯一前置节点
        auto preheaderBB = loop->getOrCreatePreheaderBB();
        // 找到 Phi 来自前置节点的源值
        for (auto phi : loop->headerBB()->getPhiNodes()) {
            if (phi->destReg() != reg) {
                continue;
            }
            for (auto tuple : phi->getTuples()) {
                if (tuple.bb() == preheaderBB) {
                    return tuple.value();
                }
            }
        }
        dbgassert(false, "No init value found for basic induction variable");
        return Value{};
    };

    // Mark basic induction variables
    for (auto bb : loop->bbs()) {
        for (auto inst : bb->getInstSet()) {
            RegPtr destReg = nullptr;
            RegPtr indReg = nullptr;
            Value initVal{};
            Value step{};
            bool subtractStep = false;
            if (auto oper = inst->as<OperInst>()) {
                auto op = oper->op();
                destReg = oper->destReg();
                if (op == Op::Add) {
                    if (loopInvarCtx.isInvariant(oper->lhs()) && oper->rhs().isRegister()) {
                        if (fitPattern(oper->rhs().getRegister(), oper->destReg())) {
                            indReg = oper->rhs().getRegister();
                            initVal = getInitVal(indReg);
                            step = oper->lhs();
                            subtractStep = false;
                        }
                    } else if (loopInvarCtx.isInvariant(oper->rhs()) && oper->lhs().isRegister()) {
                        if (fitPattern(oper->lhs().getRegister(), oper->destReg())) {
                            indReg = oper->lhs().getRegister();
                            initVal = getInitVal(indReg);
                            step = oper->rhs();
                            subtractStep = false;
                        }
                    }
                } else if (op == Op::Sub) {
                    if (loopInvarCtx.isInvariant(oper->rhs()) && oper->lhs().isRegister()) {
                        if (fitPattern(oper->lhs().getRegister(), oper->destReg())) {
                            indReg = oper->lhs().getRegister();
                            initVal = getInitVal(indReg);
                            step = oper->rhs();
                            subtractStep = true;
                        }
                    }
                }
            }
            if (indReg) {
                // Found a basic induction variable
                auto biv = BasicIndVar::create(indReg, initVal, step, subtractStep);
                basicConjMap.insert({destReg, biv});
                ivs.insert(biv);
                ivMap[indReg] = biv;
                dbgout << "├── Marked basic loop induction variable: " << biv->toString() << std::endl;
            }
        }
    }

    // Mark derived induction variables
    FixedPoint{
        [&](bool &changed) {
            for (auto bb : loop->bbs()) {
                for (auto inst : bb->getInstSet()) {
                    RegPtr indReg = nullptr;
                    Ptr<IndVar> dep = nullptr;
                    Value coef{};
                    Value bias{};
                    bool recipCoef = false;
                    bool negBias = false;
                    bool isMulDiv = false;
                    if (auto oper = inst->as<OperInst>()) {
                        auto op = oper->op();
                        indReg = oper->destReg();
                        auto zero = oper->destReg()->dataType() == PrimaryDataType::Float ? Literal{0.F} : Literal{0};
                        auto one = oper->destReg()->dataType() == PrimaryDataType::Float ? Literal{1.F} : Literal{1};
                        if (op == Op::Add) {
                            if (loopInvarCtx.isInvariant(oper->lhs()) && oper->rhs().isRegister() && ivMap.count(oper->rhs().getRegister())) {
                                dep = ivMap[oper->rhs().getRegister()];
                                coef = one;
                                bias = oper->lhs();
                                recipCoef = false;
                                negBias = false;
                            } else if (loopInvarCtx.isInvariant(oper->rhs()) && ivMap.count(oper->lhs().getRegister())) {
                                dep = ivMap[oper->lhs().getRegister()];
                                coef = one;
                                bias = oper->rhs();
                                recipCoef = false;
                                negBias = false;
                            }
                        } else if (op == Op::Sub) {
                            if (loopInvarCtx.isInvariant(oper->rhs()) && oper->lhs().isRegister() && ivMap.count(oper->lhs().getRegister())) {
                                dep = ivMap[oper->lhs().getRegister()];
                                coef = one;
                                bias = oper->rhs();
                                recipCoef = false;
                                negBias = true;
                            } else if (loopInvarCtx.isInvariant(oper->lhs()) && oper->rhs().isRegister() && ivMap.count(oper->rhs().getRegister())) {
                                dep = ivMap[oper->rhs().getRegister()];
                                coef = foldOperation(Op::Sub, 0, one);
                                bias = oper->lhs();
                                recipCoef = false;
                                negBias = false;
                            }
                        } else if (op == Op::Mul) {
                            if (loopInvarCtx.isInvariant(oper->lhs()) && oper->rhs().isRegister() && ivMap.count(oper->rhs().getRegister())) {
                                dep = ivMap[oper->rhs().getRegister()];
                                coef = oper->lhs();
                                bias = zero;
                                recipCoef = false;
                                negBias = false;
                                isMulDiv = true;
                            } else if (loopInvarCtx.isInvariant(oper->rhs()) && oper->lhs().isRegister() && ivMap.count(oper->lhs().getRegister())) {
                                dep = ivMap[oper->lhs().getRegister()];
                                coef = oper->rhs();
                                bias = zero;
                                recipCoef = false;
                                negBias = false;
                                isMulDiv = true;
                            }
                        } else if (op == Op::Div && oper->destReg()->dataType() == PrimaryDataType::Float) {
                            if (loopInvarCtx.isInvariant(oper->rhs()) && oper->lhs().isRegister() && ivMap.count(oper->lhs().getRegister())) {
                                dep = ivMap[oper->lhs().getRegister()];
                                coef = oper->rhs();
                                bias = zero;
                                recipCoef = true;
                                negBias = false;
                                isMulDiv = true;
                            }
                        }
                    }
                    if (dep && !ivMap.count(indReg)) {
                        // Check if the instruction dominates all back edge sources
                        for (auto backEdge : loop->backEdges()) {
                            auto backEdgeSrc = backEdge->src();
                            if (!cfaCtx.dominates(inst, backEdgeSrc)) {
                                dep = nullptr;
                                break;
                            }
                        }
                        if (!dep) {
                            continue;
                        }
                        // Found a derived induction variable
                        auto div = DerivedIndVar::create(indReg, inst, dep, coef, bias, recipCoef, negBias);
                        div->isMulDiv = isMulDiv;
                        if (basicConjMap.count(indReg)) {
                            div->inDesiredForm = true;
                        }
                        ivs.insert(div);
                        ivMap[indReg] = div;
                        dbgout << "├── Marked derived loop induction variable: " << div->toString() << std::endl;
                        changed = true;
                    }
                }
            }
        },
        true,
        "Derived Loop Induction Variable Marking",
        loop->headerBB()->label(),
    };

    dbgout << "├── LoopIndVarAnalysisContext constructed for loop " << loop->headerBB()->label() << "." << std::endl;
}
