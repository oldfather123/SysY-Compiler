#include "DFA.h"

using namespace ir;

// 到达定值分析
void DFAContext::rd() {
    unsigned defCounter = 0;                   // 给定值编号用的计数器
    Map<Definition, unsigned> defIndexMap;     // 定值与编号的关联
    Map<Variable, Set<Definition>> varDefsMap; // 变量与定值的关联
    Set<Variable> allMayUseVars{};

    // 先找到每个变量的所有定值，以及所有可能使用的变量
    for (auto bb : func->bbSet()) {
        for (auto inst : bb->getInstSet()) {
            for (auto mayDefVar : inst->mayDefVars()) {
                auto def = Definition{mayDefVar, inst};

                // 记录定值
                varDefsMap[mayDefVar].insert(def);

                // 给定值编号
                if (defIndexMap.find(def) == defIndexMap.end()) { // 防止重复给定值编号
                    defIndexMap[def] = defCounter++;
                }
            }
            for (auto mayUseVar : inst->mayUseVars()) {
                allMayUseVars.insert(mayUseVar);
            }
        }
    }

    // gen kill
    // 认为入口块定值了所有全局变量和当前函数的所有指针形参
    auto &entryBBInfo = bbInfoMap[func->entryBB()];
    for (auto mayUseVar : allMayUseVars) {
        auto def = Definition{mayUseVar, PseudoInst::createEntry(func->entryBB())};
        entryBBInfo.rdGenSet.insert(def);
        defIndexMap[def] = defCounter++;
    }
    for (auto bb : func->bbSet()) {
        auto &bbInfo = bbInfoMap[bb];
        auto &genSet = bbInfo.rdGenSet;   // 当前基本块的gen集合
        auto &killSet = bbInfo.rdKillSet; // 当前基本块的kill集合

        // 按顺序遍历基本块中的每个指令
        for (auto inst : bb->getInstTopoList()) {
            for (auto mayDefVar : inst->mayDefVars()) {
                auto def = Definition{mayDefVar, inst};

                // kill
                auto allDefs = varDefsMap[mayDefVar]; // 当前变量的所有定值
                // 将当前变量的其他所有定值加入 kill，并去掉 gen 中的对应定值
                for (auto otherDef : allDefs) {
                    if (otherDef != def) { // 不考虑自己
                        killSet.insert(otherDef);
                        genSet.erase(otherDef);
                    }
                }

                // gen
                genSet.insert(def);
            }
        }

        // 构造 bitset
        for (auto defInst : genSet) {
            auto index = defIndexMap[defInst];
            bbInfo.rdGenBitset.set(index);
        }
        for (auto defInst : killSet) {
            auto index = defIndexMap[defInst];
            bbInfo.rdKillBitset.set(index);
        }
    }
    auto defCount = defCounter;

    // in out
    // 初始化
    for (auto bb : func->bbSet()) {
        auto &bbInfo = bbInfoMap[bb];
        bbInfo.rdInBitset.reset();
        bbInfo.rdOutBitset.reset();
    }
    // 不动点法
    FixedPoint{
        [&](bool &changed) {
            for (auto bb : func->dfsBBList(func->entryBB(), false)) {
                auto &bbInfo = bbInfoMap[bb];
                auto &inBitset = bbInfo.rdInBitset;
                auto &outBitset = bbInfo.rdOutBitset;
                auto &genBitset = bbInfo.rdGenBitset;
                auto &killBitset = bbInfo.rdKillBitset;

                for (auto pred : bb->preds()) {
                    inBitset |= bbInfoMap[pred].rdOutBitset;
                }
                auto lastOutBitset = outBitset;
                outBitset = genBitset | (inBitset & ~killBitset);
                if (lastOutBitset != outBitset) {
                    changed = true;
                }
            }
        },
        true,
        "DFA Reaching Definition",
        func->name(),
    };
    // bitset 转 unordered_set
    Vector<Definition> defList(defCount); // 定值列表，下标为编号
    for (auto pair : defIndexMap) {
        auto def = pair.first;
        auto index = pair.second;
        defList[index] = def;
    }
    for (auto bb : func->bbSet()) {
        auto &bbInfo = bbInfoMap[bb];
        auto &inBitset = bbInfo.rdInBitset;
        auto &outBitset = bbInfo.rdOutBitset;
        for (unsigned i = 0; i < defCount; i++) {
            if (inBitset.test(i)) {
                bbInfo.rdInSet.insert(defList[i]);
            }
            if (outBitset.test(i)) {
                bbInfo.rdOutSet.insert(defList[i]);
            }
        }
    }
}

// 活跃变量分析
void DFAContext::lv() {
    unsigned varCounter = 0;             // 给变量编号用的计数器
    Map<Variable, unsigned> varIndexMap; // 变量与编号的关联

    // use def
    // 认为出口块使用了所有全局变量和当前函数的所有指针形参，则效果为它们在出口处活跃
    auto &exitBBInfo = bbInfoMap[func->exitBB()];
    for (auto global : func->parentModule()->getGlobalRegs()) {
        auto var = Variable::mem(global);
        exitBBInfo.lvUseSet.insert(var);
        varIndexMap[var] = varCounter++;
    }
    for (auto param : func->paramList()) {
        if (param->dataType().isPointer()) {
            auto var = Variable::mem(param);
            exitBBInfo.lvUseSet.insert(var);
            varIndexMap[var] = varCounter++;
        }
    }
    for (auto bb : func->bbSet()) {
        auto &bbInfo = bbInfoMap[bb];
        auto &useSet = bbInfo.lvUseSet; // 当前基本块的使用变量
        auto &defSet = bbInfo.lvDefSet; // 当前基本块的定值变量

        // 按顺序遍历基本块中的每个指令
        for (auto inst : bb->getInstTopoList()) {
            auto mayUseVars = inst->mayUseVars(); // 当前指令可能使用的变量（保守分析）
            auto defVars = inst->mustDefVars();   // 当前指令定值的变量

            // 先分析使用
            for (auto mayUseVar : mayUseVars) {
                if (defSet.find(mayUseVar) == defSet.end()) { // 判断是否先于所有的定值被使用
                    useSet.insert(mayUseVar);
                }

                // 顺便给变量编号
                if (varIndexMap.find(mayUseVar) == varIndexMap.end()) { // 防止重复给变量编号
                    varIndexMap[mayUseVar] = varCounter++;
                }
            }
            // 再分析定值
            for (auto defVar : defVars) {
                if (useSet.find(defVar) == useSet.end()) { // 判断是否先于所有的使用被定值
                    defSet.insert(defVar);
                }
                // 顺便给变量编号
                if (varIndexMap.find(defVar) == varIndexMap.end()) { // 防止重复给变量编号
                    varIndexMap[defVar] = varCounter++;
                }
            }
        }

        // 构造 bitset
        for (auto useVar : useSet) {
            auto index = varIndexMap[useVar];
            bbInfo.lvUseBitset.set(index);
        }
        for (auto defVar : defSet) {
            auto index = varIndexMap[defVar];
            bbInfo.lvDefBitset.set(index);
        }
    }
    auto varCount = varCounter;

    // in out
    // 初始化
    for (auto bb : func->bbSet()) {
        auto &bbInfo = bbInfoMap[bb];
        bbInfo.lvInBitset.reset();
        bbInfo.lvOutBitset.reset();
    }
    // 不动点法
    FixedPoint{
        [&](bool &changed) {
            // 从 exitBB 反向遍历
            dbgassert(func->exitBB()->preds().size() != 0, "Exit BB doesn't have any predecessors, which means that it is unreachable");
            for (auto bb : func->dfsBBList(func->exitBB(), true)) {
                auto &bbInfo = bbInfoMap[bb];
                auto &inBitset = bbInfo.lvInBitset;
                auto &outBitset = bbInfo.lvOutBitset;
                auto &useBitset = bbInfo.lvUseBitset;
                auto &defBitset = bbInfo.lvDefBitset;

                for (auto succ : bb->succs()) {
                    outBitset |= bbInfoMap[succ].lvInBitset;
                }
                auto lastInBitset = inBitset;
                inBitset = useBitset | (outBitset & ~defBitset);
                if (lastInBitset != inBitset) {
                    changed = true;
                }
            }
        },
        true,
        "DFA Live Variable",
        func->name(),
    };
    // bitset 转 unordered_set
    Vector<Variable> varList(varCount); // 变量列表，下标为编号
    for (auto pair : varIndexMap) {
        auto var = pair.first;
        auto index = pair.second;
        varList[index] = var;
    }
    for (auto bb : func->bbSet()) {
        auto &bbInfo = bbInfoMap[bb];
        auto &inBitset = bbInfo.lvInBitset;
        auto &outBitset = bbInfo.lvOutBitset;
        for (unsigned i = 0; i < varCount; i++) {
            if (inBitset.test(i)) {
                bbInfo.lvInSet.insert(varList[i]);
            }
            if (outBitset.test(i)) {
                bbInfo.lvOutSet.insert(varList[i]);
            }
        }
    }
}

// 可用表达式分析
void DFAContext::ae() {
    unsigned evalCounter = 0;                    // 给求值指令编号用的计数器
    Map<Ptr<DefInst>, unsigned> evalIndexMap;    // 求值指令与编号的关联
    Map<Variable, Set<Ptr<DefInst>>> evalsUsing; // 变量与使用它的求值指令的集合的关联
    Map<Ptr<DefInst>, Expr> exprMap;             // 指令与其求值的表达式的关联

    // 预处理
    for (auto bb : func->bbSet()) {
        for (auto inst : bb->getInstSet()) {
            if (auto defInst = inst->as<DefInst>()) {
                auto expr = Expr{defInst}; // 当前指令求值的表达式

                if (expr.canBeAvailable) {
                    // 预处理 exprMap
                    exprMap[defInst] = expr;

                    // 预处理 evalsUsing
                    for (auto mayUseVar : inst->mayUseVars()) {
                        evalsUsing[mayUseVar].insert(defInst);
                    }

                    // 给求值指令编号
                    if (evalIndexMap.find(defInst) == evalIndexMap.end()) { // 防止重复给求值指令编号
                        evalIndexMap[defInst] = evalCounter++;
                    }
                }
            }
        }
    }

    // gen kill
    for (auto bb : func->bbSet()) {
        if (bb->label() == "inlined.inc_a.start") {
            int breakpoint = 0;
        }

        auto &bbInfo = bbInfoMap[bb];
        auto &genSet = bbInfo.aeGenSet;   // 当前基本块的gen集合
        auto &killSet = bbInfo.aeKillSet; // 当前基本块的kill集合

        // 按顺序遍历基本块中的每个指令
        int i = 0;
        auto instSet = bb->getInstSet();
        for (auto inst : bb->getInstTopoList()) {
            ++i;

            // gen
            if (auto defInst = castPtr<DefInst>(inst)) {
                auto expr = exprMap[defInst];
                if (expr.canBeAvailable) {
                    genSet.insert(defInst);
                    // for (auto it = killSet.begin(); it != killSet.end();) {
                    //     auto killEval = *it;
                    //     if (exprMap[killEval] == expr) {
                    //         it = killSet.erase(it);
                    //     } else {
                    //         ++it;
                    //     }
                    // }
                }
            }

            // kill
            for (auto mayDefVar : inst->mayDefVars()) {
                auto killEvals = evalsUsing[mayDefVar];
                for (auto killEval : killEvals) {
                    if (!instSet.count(killEval)) {
                        killSet.insert(killEval);
                    }
                    genSet.erase(killEval);
                }
            }
        }

        // 构造 bitset
        for (auto eval : genSet) {
            auto index = evalIndexMap[eval];
            bbInfo.aeGenBitset.set(index);
        }
        for (auto eval : killSet) {
            auto index = evalIndexMap[eval];
            bbInfo.aeKillBitset.set(index);
        }
    }
    auto evalCount = evalCounter;

    // in out
    // 初始化
    for (auto bb : func->bbSet()) {
        auto &bbInfo = bbInfoMap[bb];
        if (bb == func->entryBB()) {
            bbInfo.aeInBitset.reset();
            bbInfo.aeOutBitset.reset();
        } else {
            bbInfo.aeInBitset.set();
            bbInfo.aeOutBitset.set();
        }
    }
    // 不动点法
    FixedPoint{
        [&](bool &changed) {
            for (auto bb : func->dfsBBList(func->entryBB(), false)) {
                auto &bbInfo = bbInfoMap[bb];
                auto &inBitset = bbInfo.aeInBitset;
                auto &outBitset = bbInfo.aeOutBitset;
                auto &genBitset = bbInfo.aeGenBitset;
                auto &killBitset = bbInfo.aeKillBitset;

                for (auto pred : bb->preds()) {
                    inBitset &= bbInfoMap[pred].aeOutBitset;
                }
                auto lastOutBitset = outBitset;
                outBitset = genBitset | (inBitset & ~killBitset);
                if (lastOutBitset != outBitset) {
                    changed = true;
                }
            }
        },
        true,
        "DFA Available Expression",
        func->name(),
    };
    // bitset 转 unordered_set
    Vector<Ptr<DefInst>> evalList(evalCount); // 求值指令列表，下标为编号
    for (auto pair : evalIndexMap) {
        auto eval = pair.first;
        auto index = pair.second;
        evalList[index] = eval;
    }
    for (auto bb : func->bbSet()) {
        auto &bbInfo = bbInfoMap[bb];
        auto &inBitset = bbInfo.aeInBitset;
        auto &outBitset = bbInfo.aeOutBitset;
        for (unsigned i = 0; i < evalCount; i++) {
            auto expr = exprMap[evalList[i]];
            if (inBitset.test(i)) {
                auto it = bbInfo.aeInSet.find(expr);
                if (it != bbInfo.aeInSet.end()) {
                    expr = *it;
                    bbInfo.aeInSet.erase(it);
                }
                expr.evalInsts.insert(evalList[i]);
                bbInfo.aeInSet.insert(expr);
            }
            if (outBitset.test(i)) {
                auto it = bbInfo.aeOutSet.find(expr);
                if (it != bbInfo.aeOutSet.end()) {
                    expr = *it;
                    bbInfo.aeOutSet.erase(it);
                }
                expr.evalInsts.insert(evalList[i]);
                bbInfo.aeOutSet.insert(expr);
            }
        }
    }
}
