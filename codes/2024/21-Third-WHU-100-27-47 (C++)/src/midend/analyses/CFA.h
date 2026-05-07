#pragma once

#include "Common.h"
#include "IR.h"

namespace ir {
    struct CFAContext {
    private:
        Map<ir::BBPtr, unsigned> dfn{};           // 节点的先序遍历 dfs 序
        Map<ir::BBPtr, ir::BBPtr> idom{};         // 直接支配节点
        Map<ir::BBPtr, Set<ir::BBPtr>> domTree{}; // 支配树的出边
        Map<ir::BBPtr, Set<ir::BBPtr>> df{};      // 支配边界
        Set<Ptr<ir::CFGEdge>> cutEdges{};         // 割边
        Set<ir::BBPtr> cutPoints{};               // 割点
        Map<ir::BBPtr, unsigned> domTreeSt{};     // dfsDomTree 时基本块的进入时间
        Map<ir::BBPtr, unsigned> domTreeEd{};     // dfsDomTree 时基本块的离开时间
        Map<ir::InstPtr, unsigned> instDfn{};     // 指令按拓扑序的 dfs 序

        void dfsDomTree(ir::BBPtr bb, Set<ir::BBPtr> &visited, unsigned &dfnCounter);

    public:
        ir::FuncPtr func;

        const Set<ir::BBPtr> &getDF(const ir::BBPtr &bb) const { return df.at(bb); }
        const unsigned getDFN(const ir::BBPtr &bb) const { return dfn.at(bb); }
        const unsigned getDFN(const ir::InstPtr &inst) const { return instDfn.at(inst); }
        ir::BBPtr getIdom(const ir::BBPtr &bb) const { return idom.at(bb); }
        const Map<ir::BBPtr, Set<ir::BBPtr>> &getDomTree() const { return domTree; }
        const Set<ir::BBPtr> &getDomTreeChildren(const ir::BBPtr &bb) const { return domTree.at(bb); }
        bool isReachable(const ir::BBPtr &bb) const { return dfn.at(bb) != 0; }
        bool isCutEdge(const Ptr<ir::CFGEdge> &edge) const { return cutEdges.count(edge) != 0; }
        bool isCutPoint(const ir::BBPtr &bb) const { return cutPoints.count(bb) != 0; }

        // 判断 bb1 是否支配 bb2
        bool dominates(ir::BBPtr bb1, ir::BBPtr bb2) {
            return domTreeSt.at(bb1) <= domTreeSt.at(bb2) && domTreeEd.at(bb1) >= domTreeEd.at(bb2);
        }

        // 判断 inst1 是否支配 inst2
        bool dominates(ir::InstPtr inst1, ir::InstPtr inst2) {
            if (inst1->parentBB() != inst2->parentBB()) {
                return dominates(inst1->parentBB(), inst2->parentBB());
            }
            return instDfn.at(inst1) < instDfn.at(inst2);
        }

        // 判断 inst 是否支配 bb
        bool dominates(ir::InstPtr inst, ir::BBPtr bb) {
            return dominates(inst->parentBB(), bb);
        }

        // 判断 bb 是否支配 inst
        bool dominates(ir::BBPtr bb, ir::InstPtr inst) {
            return dominates(bb, inst->parentBB());
        }

        CFAContext(ir::FuncPtr func);
        CFAContext(const CFAContext &other) = delete;
    };
}
