#include "CSE.h"
using namespace ir;

bool cseTrip(FuncPtr func, CFAContext &cfaCtx, DFAContext &dfaCtx) {
    bool changed = false;
    unsigned replacedCount = 0;
    dbgout << func->toString(); // DEBUG

    for (auto bb : dfaCtx.func->dfsBBList()) {
        Set<Expr> curAvailExprs = dfaCtx.getAEInSet(bb);
        Map<Variable, Set<Expr>> exprsUsing{};

        // 预处理 exprsUsing
        for (auto availExpr : curAvailExprs) {
            auto defInst = *availExpr.evalInsts.begin();
            if (!defInst->parentBB()) {
                // 被 CSE 消除了
                continue;
            }
            for (auto mayUseVar : defInst->mayUseVars()) {
                exprsUsing[mayUseVar].insert(availExpr);
            }
        }

        // 按顺序遍历指令
        for (auto inst : bb->getInstTopoList()) {

            if (auto defInst = inst->as<DefInst>()) {
                auto genExpr = Expr{defInst}; // 当前指令计算的表达式

                if (genExpr.canBeAvailable) {
                    // 如果可用表达式里有当前指令计算的表达式，则将当前指令替换为复制
                    auto it = curAvailExprs.find(genExpr);
                    if (it != curAvailExprs.end()) {
                        auto foundExpr = *it;
                        // 判断是否所有求值都支配当前指令
                        // 找到其中最早的求值
                        Ptr<DefInst> earliestDomEval = nullptr;
                        for (auto evalInst : foundExpr.evalInsts) {
                            if (!evalInst->parentBB()) {
                                // 该求值指令已经被替换掉
                                continue;
                            }
                            if (cfaCtx.dominates(evalInst, earliestDomEval ? earliestDomEval : inst)) {
                                earliestDomEval = evalInst;
                            } else {
                                earliestDomEval = nullptr;
                                break;
                            }
                        }
                        // 找到了才可以替换
                        if (earliestDomEval) {
                            Instruction::insertBefore(inst, MoveInst::create(bb, defInst->destReg(), earliestDomEval->destReg()));
                            dbgout << "├── Replaced common sub-expression calculation: `" << inst->toString() << "` -> " << earliestDomEval->destReg()->toString() << std::endl;
                            Instruction::remove(inst);
                            ++replacedCount;
#ifdef DEBUG
                            if (replacedCount % 100 == 0) {
                                dbgout << "├── " << replacedCount << " common sub-expression calculations replaced..." << std::endl;
                            }
#endif
                            changed = true;
                            continue;
                        }
                    }

                    // gen
                    curAvailExprs.insert(genExpr);
                    for (auto mayUseVar : inst->mayUseVars()) {
                        exprsUsing[mayUseVar].insert(genExpr);
                    }
                }
            }

            // kill
            for (auto mayDefVar : inst->mayDefVars()) {
                for (auto killExpr : exprsUsing[mayDefVar]) { // COMPLEXITY
                    curAvailExprs.erase(killExpr);
                }
            }
        }
    }

    dbgout << "├── " << replacedCount << " common sub-expression calculations replaced in this trip." << std::endl;
    return changed;
}

bool ir::cse(FuncPtr func, unsigned maxIter) {
    bool changed = false;

    dbgout << std::endl
           << "CSE pass started (" << func->name() << ")." << std::endl;

    // 执行控制流分析
    CFAContext cfaCtx{func};

    FixedPoint{
        [&](bool &_changed) {
            // 执行可用表达式分析
            DFAContext dfaCtx{func, DFAContext::AE};

            // 执行一趟公共子表达式消除
            _changed |= cseTrip(func, cfaCtx, dfaCtx);
            changed |= _changed;
        },
        true,
        "Common Sub-expression Elimination",
        func->name(),
    };

    dbgout << "└── CSE pass done." << std::endl;
    return changed;
}
