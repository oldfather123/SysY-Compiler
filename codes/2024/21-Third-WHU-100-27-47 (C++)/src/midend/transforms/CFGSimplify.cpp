#include "CFGSimplify.h"
#include "CFA.h"

using namespace ir;

bool _checkRemoveBB(BBPtr bb) {
    // Check prequisites for removing bb
    // 1. bb and any of its successors should not have more than one predecessor at the same time
    // 2. If bb has more than one successor, the entry should not be a predecessor of bb
    // 3. If a successor has phi nodes, it should not have any same predecessor as bb, which should only have one predecessor at most
    // 4. If a successor has phi nodes, the entry should not be a predecessor of bb
    if (bb->succCount() > 1) {
        for (auto pred : bb->preds()) {
            if (pred->succCount() > 1 || pred == bb->parentFunc()->entryBB()) {
                return false;
            }
        }
    }
    for (auto succ : bb->succs()) {
        if (succ->getPhiNodes().size() > 0) {
            if (bb->predCount() > 1 ||
                (!succ->inEdges().empty() && bb->predCount() != 0 && succ->preds().count(bb->firstPred()) != 0) ||
                bb->preds().count(bb->parentFunc()->entryBB()) != 0) {
                return false;
            }
        }
    }
    return true;
}

void ir::cfgSimplify(FuncPtr func) {
    dbgout << std::endl
           << "CFGSimplify pass started (" << func->name() << ")." << std::endl;

    // 执行控制流分析
    CFAContext cfaCtx{func};

    // 将条件为重言式或矛盾式或两个目标块相同的条件跳转指令替换为无条件跳转指令
    // 简化重言式或矛盾式的条件跳转会导致其中一条 CFG 边被删除，除非这条边是割边，则不能删除，而是重新连接到出口节点，防止造成出口节点不可达
    {
        for (auto bb : func->dfsBBList()) {
            if (bb == func->entryBB() || bb == func->exitBB()) {
                continue;
            }

            if (bb->exitInst()->isBranch() && bb->exitInst()->isCondBr()) {
                auto brInst = bb->exitInst();
                auto cond = brInst->condition();
                if (cond.isLiteral()) {
                    dbgassert(cond.getLiteral().isBool(), "Condition type is not Bool");
                    bool condBool = cond.getLiteral().getBool();
                    auto edgeToKeep = condBool ? brInst->condBrTrueEdge() : brInst->condBrFalseEdge();
                    auto edgeToRemove = condBool ? brInst->condBrFalseEdge() : brInst->condBrTrueEdge();
                    if (cfaCtx.isCutEdge(edgeToRemove)) {
                        // 要删除的边是割边
                        if (edgeToRemove->dest() != func->exitBB()) {
                            dbgout << "├── Simplified conditional branch with always-" << (String)(condBool ? "true" : "false") << " condition: " << bb->label() << "->" << edgeToKeep->dest()->label() << " (kept), " << bb->label() << "->" << edgeToRemove->dest()->label() << " (cut edge, redirected to exit BB `" << func->exitBB()->label() << "`)" << std::endl;
                            edgeToRemove->redirectDest(func->exitBB());
                            edgeToRemove->isFake() = true; // 这条边实际上永远不可能有控制流通过，只是为了维护图的连通性
                        }
                    } else {
                        dbgout << "├── Simplified conditional branch with always-" << (String)(condBool ? "true" : "false") << " condition: " << bb->label() << "->" << edgeToKeep->dest()->label() << " (made unconditional), " << bb->label() << "->" << edgeToRemove->dest()->label() << " (removed)" << std::endl;
                        edgeToKeep->setUncondBr();
                        BasicBlock::removeEdge(edgeToRemove);
                    }
                } else if (brInst->trueTarget() == brInst->falseTarget()) {
                    dbgout << "├── Simplified conditional branch with identical targets: " << bb->label() << "->" << brInst->trueTarget()->label() << " (made unconditional)" << std::endl;
                    brInst->condBrTrueEdge()->setUncondBr();
                    BasicBlock::removeEdge(brInst->condBrFalseEdge());
                }
            }
        }
    }

    // 删除只有一个跳转指令的基本块（如果符合条件的话）
    {
        auto bbSet = func->bbSet();
        for (auto bb : bbSet) {
            if (bb == func->entryBB() || bb == func->exitBB()) {
                continue;
            }

            if (bb != func->entryBB() && bb->isEmpty()) {
                bool delFlag = _checkRemoveBB(bb); // Check if bb can be removed
                if (delFlag) {
                    // Remove the basic block, remove or redirect corresponding edges
                    auto inEdges = bb->inEdges();
                    auto outEdges = bb->outEdges();
                    if (bb->succCount() == 1) {
                        // Copy the outgoing edge and redirect the destination of the it to the only successor
                        auto outEdge = bb->firstOutEdge();
                        auto succ = outEdge->dest();
                        for (auto inEdge : inEdges) {
                            auto newEdge = BasicBlock::duplicateEdge(outEdge);
                            auto pred = inEdge->src();
                            // Merge edges
                            newEdge->redirectSrc(pred);                     // The new edge will have the same properties as the original outgoing edge
                            newEdge->tag() = inEdge->tag();                 // Except for the tag
                            newEdge->brCondition() = inEdge->brCondition(); // And the branch condition
                        }
                    } else if (bb->succCount() == 2) {
                        // In this case, all predecessors of bb should have only one successor, which is bb
                        // Copy the outgoing edges and redirect the source of them to the predecessors of the basic block
                        for (auto inEdge : inEdges) {
                            auto pred = inEdge->src();
                            dbgassert(pred->succCount() == 1, "Predecessor of bb should have only one successor in this case");
                            for (auto outEdge : outEdges) {
                                auto newEdge = BasicBlock::duplicateEdge(outEdge);
                                // Merge edges
                                newEdge->redirectSrc(pred); // The new edge will have the same properties as the original outgoing edge
                            }
                        }
                    } else {
                        dbgassert(false, "Unexpected number of successors for basic block");
                    }
                    // Remove bb from the CFG and from its parent function
                    BasicBlock::remove(bb);
                    dbgout << "├── Removed empty basic block: " << bb->label() << std::endl;
                }
            }
        }
    }

    // 删除不可达的基本块
    {
        auto reachableBBList = func->dfsBBList();
        Set<BBPtr> reachableBBs{reachableBBList.begin(), reachableBBList.end()};
        auto bbSet = func->bbSet();
        for (auto bb : bbSet) {
            if (reachableBBs.find(bb) == reachableBBs.end()) {
                dbgassert(bb != func->exitBB(), "Exit BB should not become unreachable");
                BasicBlock::remove(bb);
                dbgout << "├── Removed unreachable basic block: " << bb->label() << std::endl;
            }
        }
    }

    // 替换只有一个元组的 Phi 节点
    {
        for (auto bb : func->bbSet()) {
            for (auto phi : bb->getPhiNodes()) {
                if (phi->getTuples().size() == 1) {
                    auto &tuple = *phi->getTuples().begin();
                    Instruction::replace(phi, MoveInst::create(bb, phi->destReg(), tuple.value()));
                }
            }
        }
    }

    // 合并基本块
    {
        // 按深度优先序遍历，可以一趟合并完
        for (auto bb : func->dfsBBList()) {
            if (bb == func->entryBB() || bb == func->exitBB()) {
                continue;
            }
            if (bb->parentFunc() == nullptr) {
                // 已被合并掉
                dbgassert(false, "I think this should not happen");
                continue;
            }
            if (bb->predCount() == 1 && bb->firstPred()->succCount() == 1) {
                // 只有一个前驱，且前驱只有一个后继，可以尝试合并
                auto pred = bb->firstPred();
                if (pred == func->entryBB()) {
                    // 不能合并入口块
                    continue;
                }
                auto mergedBB = BasicBlock::merge(pred, bb);
                dbgout << "├── Merged basic block `" << pred->label() << "` and `" << bb->label() << "` to `" << mergedBB->label() << "`" << std::endl;
            }
        }
    }

    dbgout << "└── CFGSimplify pass done." << std::endl;
}
