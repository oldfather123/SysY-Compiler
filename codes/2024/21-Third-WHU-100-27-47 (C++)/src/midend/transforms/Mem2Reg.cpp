#include "Mem2Reg.h"
#include "DFA.h"
#include "CFA.h"

using namespace ir;

struct _Mem2RegPass {
    Set<RegPtr> regsToPromote{};
    Map<BBPtr, Map<RegPtr, Ptr<PhiInst>>> phiMap{}; // 记录每个基本块每个变量已经插入的 Phi 节点

    void _renameDfs(BBPtr bb, Set<BBPtr> &visited, Map<RegPtr, std::stack<RegPtr>> &ssaStack, Map<RegPtr, unsigned> &counter, bool &changed) {
        if (visited.find(bb) != visited.end()) {
            return;
        }
        visited.insert(bb);

        Map<RegPtr, int> pushCount{};
        for (auto &inst : bb->getInstTopoList()) {
            if (!castPtr<PhiInst>(inst)) {
                for (auto useRef : inst->regsReadRefs()) {
                    auto use = useRef.get();
                    if (regsToPromote.find(use) == regsToPromote.end()) {
                        continue;
                    }
                    if (ssaStack.find(use) == ssaStack.end() || ssaStack[use].empty()) {
                        continue;
                    }
                    RegPtr ssaReg = nullptr;
                    if (ssaStack[use].empty()) {
                        ssaReg = use;
                    } else {
                        ssaReg = ssaStack[use].top();
                    }
                    inst->replaceReg(useRef, ssaReg);
                    changed = true;
                }
                inst->recompute();
            }
            for (auto defRef : inst->regsWrittenRefs()) {
                auto def = defRef.get();
                if (regsToPromote.find(def) == regsToPromote.end()) {
                    continue;
                }
                auto num = counter[def]++;
                // Replace ".addr" in def->name()
                std::string name = def->name();
                size_t pos = name.find(".addr");
                if (pos != std::string::npos) {
                    name.replace(pos, 5, "");
                }
                auto ssaReg = ir::Register::create(bb->parentFunc(), def->dataType(), name + (num < 0 ? "" : "." + std::to_string(num)));
                ssaReg->isAnonymous() = def->isAnonymous();
                ssaReg->isRet() = def->isRet();
                ssaReg->argIndex() = def->argIndex();
                inst->replaceReg(defRef, ssaReg);
                ssaStack[def].push(ssaReg);
                ++pushCount[def];
                changed = true;
            }
            inst->recompute();
        }
        for (auto succ : bb->succs()) {
            if (phiMap.find(succ) != phiMap.end()) {
                for (auto pair : phiMap[succ]) {
                    auto reg = pair.first;
                    auto phiInst = pair.second;
                    if (ssaStack.find(reg) == ssaStack.end() || ssaStack[reg].empty()) {
                        continue;
                    }
                    auto ssaReg = ssaStack[reg].top();
                    changed |= phiInst->updateTuple(bb, ssaReg);
                }
            }
        }
        for (auto succ : bb->succs()) {
            _renameDfs(succ, visited, ssaStack, counter, changed);
        }
        // 退栈
        for (auto pair : pushCount) {
            auto reg = pair.first;
            auto count = pair.second;
            while (count--) {
                ssaStack[reg].pop();
            }
        }
    }

    _Mem2RegPass(FuncPtr func) {
        // 执行活跃变量分析
        DFAContext dfaCtx{func, DFAContext::LV};

        // 计算支配树，顺便获得 dfs 序
        CFAContext cfaCtx{func};

        // 记录所有 SSA 相关的的指针寄存器
        for (auto bb : func->bbSet()) {
            for (auto inst : bb->getInstSet()) {
                if (auto allocInst = castPtr<AllocInst>(inst)) {
                    auto dataType = allocInst->destReg()->dataType();
                    if (dataType.isPointer() && dataType.derefType().isPrimary()) {
                        regsToPromote.insert(allocInst->destReg());
                    }
                } else if (auto retInst = castPtr<RetInst>(inst)) {
                    dbgassert(retInst->retVal().isRegister(), "Return value must be a register at Mem2Reg pass");
                    regsToPromote.insert(retInst->retVal().getRegister());
                }
            }
        }

        // Promote
        for (auto bb : func->dfsBBList()) {
            for (auto inst : bb->getInstTopoList()) {
                if (auto allocInst = castPtr<AllocInst>(inst)) {
                    if (regsToPromote.find(allocInst->destReg()) != regsToPromote.end()) {
                        allocInst->destReg()->dataType() = allocInst->destReg()->dataType().derefType();
                        Instruction::remove(allocInst);
                    }
                } else if (auto storeInst = castPtr<StoreInst>(inst)) {
                    if (regsToPromote.find(storeInst->destAddrReg()) != regsToPromote.end()) {
                        dbgassert(!storeInst->destAddrReg()->dataType().isPointer(), "The register should have already been converted to non-pointer");
                        Instruction::replace(storeInst, ir::MoveInst::create(bb, storeInst->destAddrReg(), storeInst->srcVal()));
                    }
                } else if (auto loadInst = castPtr<LoadInst>(inst)) {
                    if (regsToPromote.find(loadInst->srcAddrReg()) != regsToPromote.end()) {

                        dbgassert(!loadInst->srcAddrReg()->dataType().isPointer(), "The register should have already been converted to non-pointer");
                        Instruction::replace(loadInst, ir::MoveInst::create(bb, loadInst->destReg(), loadInst->srcAddrReg()));
                    }
                }
            }
        }

        // 插入 Phi 节点
        // 不动点法
        FixedPoint{
            [&](bool &changed) {
                changed = false;
                for (auto bb : func->bbSet()) {
                    if (bb == func->entryBB()) {
                        continue;
                    }
                    for (auto inst : bb->getInstSet()) {
                        for (auto def : inst->regsWritten()) {
                            if (regsToPromote.find(def) != regsToPromote.end()) {
                                for (auto v : cfaCtx.getDF(bb)) {
                                    // 判断该寄存器是否在 v 的入口处活跃，如果不活跃就不插入 Phi 节点了
                                    // TODO Necessary?
                                    auto &lvInSet = dfaCtx.getLVInSet(v);
                                    if (lvInSet.find(Variable::reg(def)) == lvInSet.end()) {
                                        continue;
                                    }
                                    // 找对应的 Phi 函数，如果没有就创建一个
                                    if (phiMap.find(v) == phiMap.end()) {
                                        phiMap[v] = Map<RegPtr, Ptr<PhiInst>>{};
                                    }
                                    Ptr<PhiInst> phiInst = nullptr;
                                    if (phiMap[v].find(def) == phiMap[v].end()) {
                                        phiInst = PhiInst::create(v, def);
                                        Instruction::insertAfter(v->entryInst(), phiInst);
                                        phiMap[v][def] = phiInst;
                                        changed = true;
                                    } else {
                                        phiInst = phiMap[v][def];
                                    }
                                    for (auto pred : v->preds()) {
                                        changed |= phiInst->addTuple(pred, def);
                                    }
                                }
                            }
                        }
                    }
                }
            },
            true,
            "Mem2Reg Phi Insertion",
            func->name(),
        };

        // 变量重命名
        FixedPoint{
            [&](bool &changed) {
                if (func->isGlobalInitFunc()) {
                    return;
                }
                Set<BBPtr> visited{};
                Map<RegPtr, std::stack<RegPtr>> ssaStack{};
                Map<RegPtr, unsigned> counter{};
                for (auto entrySucc : func->entryBB()->succs()) {
                    _renameDfs(entrySucc, visited, ssaStack, counter, changed);
                }
            },
            true,
            "Mem2Reg Renaming",
            func->name(),
        };
    }
};

void ir::mem2reg(FuncPtr func) {
    dbgout << std::endl
           << "Mem2Reg pass started (" << func->name() << ")." << std::endl;

    _Mem2RegPass{func};

    dbgout << "└── Mem2Reg pass done." << std::endl;
}
