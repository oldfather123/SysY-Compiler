#include "InstCombine.h"
#include "UseDefAnalysis.h"

using namespace ir;

bool isAdd(InstPtr inst) {
    return inst->is<OperInst>() && inst->as<OperInst>()->op() == Op::Add;
}
bool isSub(InstPtr inst) {
    return inst->is<OperInst>() && inst->as<OperInst>()->op() == Op::Sub;
}
bool isMul(InstPtr inst) {
    return inst->is<OperInst>() && inst->as<OperInst>()->op() == Op::Mul;
}
bool isDiv(InstPtr inst) {
    return inst->is<OperInst>() && inst->as<OperInst>()->op() == Op::Div;
}

struct InstCombineContext {
    Map<Ptr<OperInst>, Ptr<OperInst>> replaceMap{};
    UseDefAnalysisContext &useDefCtx;

    InstCombineContext(UseDefAnalysisContext &useDefCtx) : useDefCtx(useDefCtx) { }

    std::pair<Ptr<OperInst>, bool> combineOperation(Ptr<OperInst> oper, Ptr<OperInst> childOper, const Literal &childLiteral, bool lhsIsOper) {
        Ptr<OperInst> ret = nullptr;
        Literal subtreeLiteral{};
        RegPtr subtreeReg = nullptr;
        bool subtreeRhsIsLiteral = false;
        bool subtreeRhsIsReg = false;
        if (childOper->lhs().isLiteral() && childOper->rhs().isRegister()) {
            subtreeRhsIsLiteral = false;
            subtreeRhsIsReg = true;
            subtreeLiteral = childOper->lhs().getLiteral();
            subtreeReg = childOper->rhs().getRegister();
        } else if (childOper->rhs().isLiteral() && childOper->lhs().isRegister()) {
            subtreeRhsIsLiteral = true;
            subtreeRhsIsReg = false;
            subtreeLiteral = childOper->rhs().getLiteral();
            subtreeReg = childOper->lhs().getRegister();
        } else {
            return {oper, false};
        }
        if (isAdd(oper)) {
            if (isAdd(childOper)) {
                ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Add, subtreeReg, foldOperation(Op::Add, childLiteral, subtreeLiteral));
            } else if (isSub(childOper)) {
                if (subtreeRhsIsLiteral) {
                    ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Add, subtreeReg, foldOperation(Op::Sub, childLiteral, subtreeLiteral));
                } else {
                    ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Sub, foldOperation(Op::Add, subtreeLiteral, childLiteral), subtreeReg);
                }
            }
        } else if (isMul(oper)) {
            if (isMul(childOper)) {
                ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Mul, subtreeReg, foldOperation(Op::Mul, childLiteral, subtreeLiteral));
            } else if (isDiv(childOper)) {
                if (subtreeRhsIsLiteral) {
                    return {oper, false};
                } else {
                    ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Div, foldOperation(Op::Mul, subtreeLiteral, childLiteral), subtreeReg);
                }
            }
        } else if (isSub(oper)) {
            if (isAdd(childOper)) {
                if (lhsIsOper) {
                    ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Add, subtreeReg, foldOperation(Op::Sub, subtreeLiteral, childLiteral));
                } else {
                    ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Sub, foldOperation(Op::Sub, childLiteral, subtreeLiteral), subtreeReg);
                }
            } else if (isSub(childOper)) {
                if (lhsIsOper) {
                    if (subtreeRhsIsLiteral) {
                        ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Sub, subtreeReg, foldOperation(Op::Add, subtreeLiteral, childLiteral));
                    } else {
                        ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Sub, foldOperation(Op::Sub, subtreeLiteral, childLiteral), subtreeReg);
                    }
                } else {
                    if (subtreeRhsIsLiteral) {
                        ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Sub, foldOperation(Op::Add, childLiteral, subtreeLiteral), subtreeReg);
                    } else {
                        ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Add, subtreeReg, foldOperation(Op::Sub, childLiteral, subtreeLiteral));
                    }
                }
            }
        } else if (isDiv(oper)) {
            if (isMul(childOper)) {
                return {oper, false};
            } else if (isDiv(childOper)) {
                if (lhsIsOper) {
                    if (subtreeRhsIsLiteral) {
                        ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Div, subtreeReg, foldOperation(Op::Mul, subtreeLiteral, childLiteral));
                    } else {
                        return {oper, false};
                    }
                } else {
                    if (subtreeRhsIsLiteral) {
                        ret = OperInst::createBinary(oper->parentBB(), oper->destReg(), Op::Div, foldOperation(Op::Mul, childLiteral, subtreeLiteral), subtreeReg);
                    } else {
                        return {oper, false};
                    }
                }
            }
        }
        if (!ret) {
            return {oper, false};
        }
        dbgout << "├── Combined operations `" << oper->toString() << "` and `" << childOper->toString() << "` to `" << ret->toString() << "`" << std::endl;
        replaceMap[oper] = ret;
        Instruction::replace(oper, ret);
        return {ret, true};
    }

    std::pair<Vector<Value>, bool> handleInstCombine(Ptr<OperInst> operInst) {
        while (replaceMap[operInst]) {
            operInst = replaceMap[operInst];
        }
        bool changed = false;
        if (operInst->isUnary()) {
            return {{operInst->rhs()}, changed};
        } else {
            if ((operInst->lhs().isLiteral() && operInst->rhs().isRegister()) ||
                (operInst->rhs().isLiteral() && operInst->lhs().isRegister())) {
                // 只处理一个寄存器和一个常数的操作
                Vector<Literal> literals{}; // 当前子树中的所有常数叶子
                Vector<RegPtr> regs{};      // 当前子树中的所有寄存器叶子
                Value literalOperand{};     // 当前指令的常数操作数
                Value regOperand{};         // 当前指令的寄存器操作数
                bool childOperIsLhs = false;
                if (operInst->lhs().isLiteral()) {
                    literalOperand = operInst->lhs();
                    regOperand = operInst->rhs();
                    childOperIsLhs = false;
                } else {
                    literalOperand = operInst->rhs();
                    regOperand = operInst->lhs();
                    childOperIsLhs = true;
                }
                // 收集子树中的所有叶子
                literals.push_back(literalOperand.getLiteral());
                Ptr<OperInst> childOperInst = nullptr;
                auto regOperandDefInsts = useDefCtx.getDefInsts(Variable::reg(regOperand.getRegister()));
                dbgassert(regOperandDefInsts.size() <= 1, "Register variable should have only one definition at most");
                bool flag = false;
                if (regOperandDefInsts.size() == 1) {
                    if (childOperInst = (*regOperandDefInsts.begin())->as<OperInst>()) {
                        auto result = handleInstCombine(childOperInst); // 递归处理
                        auto subtreeLeaves = result.first;
                        changed |= result.second;
                        for (auto val : subtreeLeaves) {
                            if (val.isLiteral()) {
                                literals.push_back(val.getLiteral());
                            } else if (val.isRegister()) {
                                regs.push_back(val.getRegister());
                            }
                        }
                        flag = true;
                    }
                }
                if (!flag) {
                    regs.push_back(regOperand.getRegister());
                }
                // 统计子树中的叶子情况
                if (literals.size() == 2 && regs.size() == 1) {
                    // 达成合并条件
                    dbgassert(childOperInst, "There should be a child operation");
                    auto result = combineOperation(operInst, childOperInst, literalOperand.getLiteral(), childOperIsLhs);
                    operInst = result.first;
                    changed |= result.second;
                }
            }
            if (operInst->isUnary()) {
                return {{operInst->rhs()}, changed};
            } else {
                return {{operInst->lhs(), operInst->rhs()}, changed};
            }
        }
    }
};

bool ir::instCombine(FuncPtr func) {
    bool changed = false;

    dbgout << std::endl
           << "InstCombine pass started (" << func->name() << ")." << std::endl;

    // 只需要处理 ConstFold 没办法处理的模式

    DFAContext dfaCtx{func, DFAContext::NONE};
    UseDefAnalysisContext useDefCtx{dfaCtx};
    InstCombineContext ic{useDefCtx};

    for (auto bb : func->bbSet()) {
        auto insts = bb->getInstSet();
        for (auto inst : insts) {
            if (auto operInst = inst->as<OperInst>()) {
                auto result = ic.handleInstCombine(operInst); // COMPLEXITY O(n!) ←_← p_q
                changed |= result.second;
            }
        }
    }

    dbgout << "└── InstCombine pass done." << std::endl;
    return changed;
}
