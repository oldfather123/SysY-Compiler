#include "DCE.h"

using namespace ir;

bool dceTrip(FuncPtr func, ir::DFAContext &dfaCtx) {
    bool changed = false;
    unsigned removedCount = 0;

    for (auto bb : func->bbSet()) {
        if (bb == func->exitBB()) {
            continue;
        }

        auto &liveVarsOut = dfaCtx.getLVOutSet(bb); // 基本块结尾处的活跃变量集
        Set<Variable> curLiveVars = liveVarsOut;    // 当前活跃变量的集合，初始化为基本块出口处的活跃变量集

        Vector<InstPtr> instList = bb->getInstTopoList();
        for (auto instIt = instList.rbegin(); instIt != instList.rend(); instIt++) { // 逆拓扑序遍历基本块
            auto inst = *instIt;
            bool dead = false;

            // 满足以下条件：
            // 1. 不是函数调用指令
            // 2. 定值的变量在此处不活跃
            // 则当前指令无用
            for (auto def : inst->mustDefVars()) {
                if (curLiveVars.find(def) == curLiveVars.end()) {
                    // 定值的变量在此处不活跃
                    if (inst->is<CallInst>()) {
                        // 函数调用指令不参与死代码消除，但目标变量不活跃，标记为弃元
                        if (def.isReg()) {
                            def.getReg()->isDiscard() = true;
                        }
                        continue;
                    }
                    if (inst->is<StoreInst>() && def.isMem()) {
                        // 存储指令，检查其基指针的内存变量是否也不活跃
                        auto it = curLiveVars.find(Variable::mem(def.getMemPtr()->findAliasGroup()));
                        if (it != curLiveVars.end()) {
                            // 基变量活跃，检查 excepts 是否有 def
                            auto baseMemVar = *it;
                            if (!baseMemVar.hasExcept(def)) {
                                // excepts 也没有 def，认为活跃
                                continue;
                            }
                        }
                    }
                    // 标记为无用
                    dead = true;
                    break;
                }
            }

            if (dead) {
                // 删除无用指令
                dbgout << "├── Removed dead instruction: " << inst->toString() << std::endl;
                Instruction::remove(inst);
                ++removedCount;
#ifdef DEBUG
                if (removedCount % 100 == 0) {
                    dbgout << "├── " << removedCount << " dead instructions removed..." << std::endl;
                }
#endif
                changed = true;
            } else {
                // 根据当前指令，更新当前活跃变量集
                // 先处理定值，再处理使用
                for (auto def : inst->mustDefVars()) {
                    curLiveVars.erase(def);
                    if (def.isMem()) {
                        // 尝试找到该定值变量的基变量并将该定值变量添加到其 excepts
                        auto it = curLiveVars.find(Variable::mem(def.getMemPtr()->findAliasGroup()));
                        if (it != curLiveVars.end()) {
                            auto baseMemVar = *it;
                            curLiveVars.erase(it);
                            baseMemVar.addExcept(def);
                            curLiveVars.insert(baseMemVar);
                        }
                    }
                }
                for (auto use : inst->mayUseVars()) {
                    // 将该使用变量设为活跃
                    curLiveVars.erase(use); // 先 erase 再 insert，确保清空其 excepts
                    curLiveVars.insert(use);
                }
            }
        }
    }

    dbgout << "├── " << removedCount << " dead instructions removed in this trip." << std::endl;
    return changed;
}

bool ir::dce(FuncPtr func, unsigned maxIter) {
    bool changed = false;

    dbgout << std::endl
           << "DCE pass started (" << func->name() << ")." << std::endl;

    FixedPoint{
        [&](bool &_changed) {
            // 执行活跃变量分析
            DFAContext dfaCtx{func, DFAContext::LV};

            // 执行一趟死代码消除
            _changed |= dceTrip(func, dfaCtx);
            changed |= _changed;
        },
        true,
        "Dead Code Elimination",
        func->name(),
        maxIter,
    };

    dbgout << "└── DCE pass done." << std::endl;
    return changed;
}
