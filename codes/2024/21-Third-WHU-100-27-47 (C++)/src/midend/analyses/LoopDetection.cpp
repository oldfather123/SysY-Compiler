#include "LoopDetection.h"

using namespace ir;

Vector<BBPtr> Loop::bfsBBList() {
    Vector<BBPtr> res;
    Set<BBPtr> visited;
    std::queue<BBPtr> q;
    q.push(_headerBB);
    visited.insert(_headerBB);
    while (!q.empty()) {
        auto bb = q.front();
        q.pop();
        res.push_back(bb);
        for (auto succ : bb->succs()) {
            if (_bbs.find(succ) != _bbs.end() && visited.find(succ) == visited.end()) {
                q.push(succ);
                visited.insert(succ);
            }
        }
    }
    return res;
}

BBPtr Loop::createNewPreheaderBB() {
    auto func = _headerBB->parentFunc();
    _preheaderBB = BasicBlock::create(func, "loop.preheader");
    dbgout << "├── Created new preheader: " << _preheaderBB->label() << std::endl;
    auto headerBB = _headerBB;
    auto inEdges = headerBB->inEdges();
    for (auto it = inEdges.begin(); it != inEdges.end();) {
        auto inEdge = *it;
        dbgassert(!inEdge->isFake(), "Edge should not be fake here");
        if (_backEdges.count(inEdge)) {
            it = inEdges.erase(it);
        } else {
            inEdge->redirectDest(_preheaderBB);
            ++it;
        }
    }
    for (auto phi : headerBB->getPhiNodes()) {
        auto newReg = Register::create(func, phi->destReg()->dataType());
        auto newPhi = PhiInst::create(_preheaderBB, newReg);
        Instruction::insertAfter(_preheaderBB->entryInst(), newPhi);
        for (auto inEdge : inEdges) {
            inEdge->addParallelCopy(newPhi, inEdge->getCopySrcVal(phi));
        }
    }
    BasicBlock::addEdge(_preheaderBB, headerBB)->setUncondBr();
    return _preheaderBB;
}

// 从检测到的循环头开始，递归地发现属于当前循环的所有基本块，并处理可能的子循环
void LoopDetectionContext::discoverLoopAndSubLoops(ir::BBPtr bb, Set<Ptr<ir::CFGEdge>> &backEdges, Ptr<Loop> loop) {
    Vector<ir::BBPtr> workList{};
    Set<ir::BBPtr> visited{};
    workList.reserve(backEdges.size());
    for (auto backEdge : backEdges) {
        workList.push_back(backEdge->src());
    }
    while (!workList.empty()) {
        auto bb = workList.back();
        workList.pop_back();
        loop->addBB(bb);
        // 如果bb没有在某一个Loop中
        if (_bbToLoop.find(bb) == _bbToLoop.end()) {
            _bbToLoop[bb] = loop;
            for (auto pred : bb->preds()) {
                workList.push_back(pred);
            }
        } else if (_bbToLoop[bb] != loop) {
            // 如果bb在另一个Loop中
            auto subLoop = _bbToLoop[bb];
            while (subLoop->parentLoop() != nullptr) {
                subLoop = subLoop->parentLoop();
            }
            if (subLoop == loop) {
                continue;
            }
            subLoop->parentLoop() = loop;
            loop->addSubLoop(subLoop);
            for (auto _bb : subLoop->headerBB()->preds()) {
                workList.push_back(_bb);
            }
        }
    }
}

// 普通的递归函数，用于深度优先遍历支配树
void _dfsDomTree(BBPtr bb, const ir::CFAContext &cfaCtx, Vector<BBPtr> &postOrder) {
    if (bb == nullptr) {
        return;
    }

    // 先遍历所有子节点
    for (BBPtr child : cfaCtx.getDomTree().at(bb)) {
        _dfsDomTree(child, cfaCtx, postOrder);
    }

    // 再记录当前节点
    postOrder.push_back(bb);
}

Vector<BBPtr> _getPostOrder(ir::CFAContext &cfaCtx, FuncPtr func) {
    Vector<BBPtr> postOrder;

    // 从根节点开始遍历，根节点是没有直接支配者的节点
    BBPtr entryBB = func->entryBB();
    _dfsDomTree(entryBB, cfaCtx, postOrder);

    return postOrder;
}

LoopDetectionContext::LoopDetectionContext(CFAContext &cfaCtx) : cfaCtx(cfaCtx) {
    func = cfaCtx.func;

    for (auto bb : _getPostOrder(cfaCtx, func)) {
        Set<Ptr<CFGEdge>> backEdges;

        // 找到所有后向边
        for (auto inEdge : bb->inEdges()) {
            auto pred = inEdge->src();
            // 如果bb支配pred，则pred->bb是后向边（环）
            if (cfaCtx.dominates(bb, pred)) {
                backEdges.insert(inEdge);
            }
        }

        // 如果没有后向边，则不是循环
        if (backEdges.empty()) {
            continue;
        }

        // 处理环的情况
        auto loop = makePtr<Loop>(bb);

        // 找 preheader
        auto idom = cfaCtx.getIdom(bb);
        if (bb->preds().count(idom)) {
            loop->preheaderBB() = idom;
        } else {
            // No valid preheader BB found
            loop->preheaderBB() = nullptr;
        }

        _bbToLoop[bb] = loop;
        for (auto backEdge : backEdges) {
            loop->addBackEdge(backEdge);
        }
        _loops.push_back(loop);

        // 识别循环
        discoverLoopAndSubLoops(bb, backEdges, loop);
    }

    FixedPoint{
        [&](bool &changed) {
            for (auto loop : _loops) {
                for (auto bb : loop->bbs()) {
                    auto parentLoop = loop->parentLoop();
                    if (parentLoop) {
                        changed |= parentLoop->addBB(bb);
                    }
                }
            }
        },
    };

    for (auto loop : _loops) {
        for (auto bb : loop->bbs()) {
            for (auto outEdge : bb->outEdges()) {
                auto out = outEdge->dest();
                if (!loop->bbs().count(out)) {
                    loop->addExitEdge(outEdge);
                }
            }
        }
    }

    dbgout << "├── LoopDetectionContext constructed for function " << func->name() << "." << std::endl;
}

void ir::testLoops(ir::FuncPtr func) {
    ir::CFAContext cfaCtx{func};
    ir::LoopDetectionContext test{cfaCtx};
    Vector<Ptr<Loop>> ans = test.loops();
    if (ans.size() > 0) {
        dbgout << "Loop Getting Done" << std::endl;
        dbgout << "Loop Number: " << ans.size() << std::endl;
        dbgout << "----------------------------------------" << std::endl;
        // 展示循环信息
        dbgout << "*** Loop Information ***" << std::endl;
        test.printDebug();
        return;
    } else {
        dbgout << "No Loops Found" << std::endl;
    }
}

void LoopDetectionContext::printDebug() {
    for (auto loop : _loops) {
        dbgout << "Loop Header: " << loop->headerBB()->label() << std::endl;
        dbgout << "Loop Preheader: " << (loop->preheaderBB() ? loop->preheaderBB()->label() : "null") << std::endl;
        dbgout << "Loop Blocks: ";
        for (auto bb : loop->bbs()) {
            dbgout << bb->label() << " ";
        }
        dbgout << std::endl;
        dbgout << "Loop BackEdges: ";
        for (auto backEdge : loop->backEdges()) {
            dbgout << backEdge->src()->label() << "->" << backEdge->dest()->label() << " ";
        }
        dbgout << std::endl;
        dbgout << "Loop ExitEdges: ";
        for (auto exitEdge : loop->exitEdges()) {
            dbgout << exitEdge->src()->label() << "->" << exitEdge->dest()->label() << " ";
        }
        dbgout << std::endl;
        dbgout << "Loop SubLoops: ";
        for (auto subLoop : loop->subloops()) {
            dbgout << subLoop->headerBB()->label() << " ";
        }
        dbgout << std::endl;
        dbgout << "----------------------------------------" << std::endl;
    }
}
