#pragma once

#include "Common.h"
#include "DFA.h"
#include "CFA.h"

namespace ir {
    void testLoops(ir::FuncPtr func);
    class Loop {
    private:
        // 循环前驱：循环的前驱
        ir::BBPtr _preheaderBB = nullptr;
        // 循环头：循环的入口
        ir::BBPtr _headerBB = nullptr;
        // 当前循环的父循环
        Ptr<Loop> _parentLoop = nullptr;
        // 当前循环所有基本块
        Set<ir::BBPtr> _bbs{};
        // 当前循环的子循环
        Vector<Ptr<Loop>> _subloops{};
        // 所有回边
        Set<Ptr<ir::CFGEdge>> _backEdges{};
        // 所有退出循环的边
        Set<Ptr<ir::CFGEdge>> _exitEdges{};
        // 循环的所有出口节点
        Set<ir::BBPtr> _exitBBs{};

    public:
        Loop(ir::BBPtr headerBB) : _headerBB(headerBB) { _bbs.insert(headerBB); }
        // 循环的出口
        // std::map<ir::BBPtr, ir::BBPtr> exitsMap;
        bool addBB(ir::BBPtr bb) { return _bbs.insert(bb).second; }
        ir::BBPtr &headerBB() { return _headerBB; }
        ir::BBPtr &preheaderBB() { return _preheaderBB; }
        Ptr<Loop> &parentLoop() { return _parentLoop; }
        void addSubLoop(Ptr<Loop> loop) { _subloops.push_back(loop); }
        Set<ir::BBPtr> &bbs() { return _bbs; }
        Vector<Ptr<Loop>> &subloops() { return _subloops; }
        Set<Ptr<ir::CFGEdge>> &backEdges() { return _backEdges; }
        Set<Ptr<ir::CFGEdge>> &exitEdges() { return _exitEdges; }
        Set<ir::BBPtr> &exitBBs() { return _exitBBs; }
        void addBackEdge(Ptr<ir::CFGEdge> backEdge) { _backEdges.insert(backEdge); }
        void addExitEdge(Ptr<ir::CFGEdge> exitEdge) {
            _exitEdges.insert(exitEdge);
            _exitBBs.insert(exitEdge->src());
        }
        Vector<ir::BBPtr> bfsBBList();
        ir::BBPtr createNewPreheaderBB();
        BBPtr getOrCreatePreheaderBB() {
            if (!_preheaderBB || _preheaderBB == _preheaderBB->parentFunc()->entryBB()) {
                _preheaderBB = createNewPreheaderBB();
            }
            return _preheaderBB;
        };

        // 存在一个前驱块 preheader。
        // 只有一条回边（即 backEdgesBlocksSet 的大小为 1）。
        // 所有出口块的前驱节点都来自于循环内部，并且循环头块 headerBB 支配所有出口块。
        bool isLoopSimplifyForm() {
            bool res = (_preheaderBB != nullptr && _backEdges.size() == 1);
            if (!res)
                return false;
            for (auto exitEdge : _exitEdges) {
                for (auto preBB : exitEdge->dest()->preds()) {
                    // 如果不是所有出口块的前驱节点都来自于循环内部
                    if (_bbs.count(preBB)) {
                        return false;
                    }
                }
            }
            return res;
        }
    };
    class LoopDetectionContext {
    private:
        Vector<Ptr<Loop>> _loops{};
        Map<ir::BBPtr, Ptr<Loop>> _bbToLoop{};
        void discoverLoopAndSubLoops(ir::BBPtr bb, Set<Ptr<ir::CFGEdge>> &backEdges, Ptr<Loop> loop);

    public:
        ir::CFAContext &cfaCtx;
        ir::FuncPtr func = nullptr;

        LoopDetectionContext(ir::CFAContext &cfaCtx);
        LoopDetectionContext(const LoopDetectionContext &) = delete;
        const Vector<Ptr<Loop>> &loops() const { return _loops; }
        void printDebug();
    };
};
