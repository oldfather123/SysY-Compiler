#include "CFA.h"

using namespace ir;

struct LengauerTarjan {
    Map<BBPtr, unsigned> &dfn;       // 节点的先序遍历 dfs 序
    Map<BBPtr, BBPtr> &idom;         // 直接支配节点
    Vector<BBPtr> dfsList{};         // 先序遍历的节点列表
    Map<BBPtr, BBPtr> dfsParent{};   // dfs 树的父节点
    Map<BBPtr, BBPtr> sdom{};        // 半支配节点
    Map<BBPtr, BBPtr> mn{};          // 半支配节点的 dfn 最小的祖先
    Map<BBPtr, Set<BBPtr>> bucket{}; // 桶，一个节点是其桶中节点的半支配节点

    void dfs(BBPtr bb, unsigned &dfnCounter) {
        dfn[bb] = dfnCounter++;
        dfsList.push_back(bb);
        for (auto &succ : bb->succs()) {
            if (dfn.find(succ) == dfn.end()) {
                dfsParent[succ] = bb;
                dfs(succ, dfnCounter);
            }
        }
    }

    // 并查集
    struct UnionFind {
        Map<BBPtr, BBPtr> parent;

        void init(Vector<BBPtr> &nodes) {
            for (auto u : nodes) {
                parent[u] = u;
            }
        }
    } uf;

    BBPtr query(BBPtr u) {
        if (uf.parent[u] == u) {
            return u;
        }
        auto root = query(uf.parent[u]);
        if (dfn[sdom[mn[uf.parent[u]]]] < dfn[sdom[mn[u]]]) {
            mn[u] = mn[uf.parent[u]];
        }
        uf.parent[u] = root;
        return mn[u];
    }

    LengauerTarjan(FuncPtr func, Map<BBPtr, unsigned> &dfn, Map<BBPtr, BBPtr> &idom) : dfn(dfn), idom(idom) {
        // 先序遍历
        unsigned counter = 0;
        dfs(func->entryBB(), counter);
        unsigned n = counter;

        // 初始化
        uf.init(dfsList);
        for (auto u : dfsList) {
            mn[u] = u;
            sdom[u] = u;
            bucket[u] = Set<BBPtr>{};
        }
        for (auto bb : func->bbSet()) {
            idom[bb] = nullptr;
        }

        for (unsigned i = n - 1; i > 0; i--) { // 逆先序遍历，除了根节点
            // 计算半支配节点
            auto u = dfsList[i];
            for (auto v : u->preds()) {
                if (dfn[v] == 0) {
                    // 从根节点不可达的基本块
                    continue;
                }
                auto k = query(v);
                sdom[u] = dfn[sdom[k]] < dfn[sdom[u]] ? sdom[k] : sdom[u];
            }
            uf.parent[u] = dfsParent[u];

            bucket[sdom[u]].insert(u);
            for (auto v : bucket[dfsParent[u]]) {
                auto k = query(v);
                idom[v] = dfn[sdom[k]] < dfn[sdom[v]] ? k : dfsParent[u];
            }
        }

        for (unsigned i = 1; i < n; ++i) {
            auto u = dfsList[i];
            if (idom[u] != sdom[u]) {
                idom[u] = idom[idom[u]];
            }
        }

        // 补充入口节点的支配关系（？）
        for (auto entrySucc : func->entryBB()->succs()) {
            idom[entrySucc] = func->entryBB();
        }
    }
};

struct Tarjan {
    Set<Ptr<CFGEdge>> &cutEdges; // 割边
    Map<BBPtr, unsigned> low{}, dfn{};
    unsigned dfsCounter = 0;

    void dfs(BBPtr u, BBPtr p) {
        low[u] = dfn[u] = dfsCounter++;
        auto adj = u->outEdges();
        auto inv = u->inEdges();
        adj.insert(inv.begin(), inv.end()); // 反向边也要遍历，因为这个算法是针对无向图的
        for (auto e : adj) {
            auto v = inv.find(e) != inv.end() ? e->src() : e->dest();
            if (!dfn[v]) {
                dfs(v, u);
                low[u] = MIN(low[u], low[v]);
                if (low[v] > dfn[u]) {
                    cutEdges.insert(e);
                }
            } else if (dfn[v] < dfn[u] && v != p) {
                low[u] = MIN(low[u], dfn[v]);
            }
        }
    }

    Tarjan(FuncPtr func, Set<Ptr<CFGEdge>> &cutEdges) : cutEdges(cutEdges) {
        dfs(func->entryBB(), nullptr);
    }
};

void CFAContext::dfsDomTree(BBPtr bb, Set<BBPtr> &visited, unsigned &dfnCounter) {
    domTreeSt[bb] = ++dfnCounter;
    for (auto &domChild : domTree[bb]) {
        if (!visited.count(domChild)) {
            dfsDomTree(domChild, visited, dfnCounter);
        }
    }
    domTreeEd[bb] = dfnCounter;
}

CFAContext::CFAContext(FuncPtr func) : func(func) {
    LengauerTarjan lt{func, this->dfn, this->idom};

    // 构建支配树
    for (auto bb : func->bbSet()) {
        domTree[bb] = Set<BBPtr>{};
    }
    for (auto pair : idom) {
        auto bb = pair.first;
        auto dom = pair.second;
        if (!dom) {
            continue;
        }
        domTree[dom].insert(bb);
    }

    // 预处理间接支配关系
    {
        Set<BBPtr> visited{};
        unsigned dfnCounter = 0;
        dfsDomTree(func->entryBB(), visited, dfnCounter);
    }

    // 计算支配边界
    for (auto n : func->bbSet()) {
        df[n] = Set<BBPtr>{};
    }
    for (auto n : func->bbSet()) {
        if (dfn[n] == 0) {
            // 不可达
            continue;
        }
        for (auto p : n->preds()) {
            if (dfn[p] == 0) {
                // 不可达
                continue;
            }
            auto runner = p;
            while (runner != idom[n]) {
                df[runner].insert(n);
                runner = idom[runner];
            }
        }
    }

    // 计算割边
    Tarjan{func, this->cutEdges};

    // 计算指令的全局 dfs 序
    unsigned i = 0;
    for (auto bb : func->dfsBBList()) {
        for (auto inst : bb->getInstTopoList()) {
            instDfn[inst] = i++;
        }
    }

    dbgout << "├── CFAContext constructed for function " << func->name() << "." << std::endl;

#ifdef DEBUG
    dbgout << "├── Func " << func->name() << std::endl;
    for (auto bb : func->bfsBBList()) {
        dbgout << "│   ├── " << bb->label() << ":" << std::endl;
        dbgout << "│   │   ├── DFN: " << lt.dfn[bb] << std::endl;
        dbgout << "│   │   ├── SDom: " << (lt.sdom[bb] ? lt.sdom[bb]->label() : "null") << std::endl;
        dbgout << "│   │   ├── IDom: " << (idom[bb] ? idom[bb]->label() : "null") << std::endl;
        dbgout << "│   │   └── DF:";
        for (auto n : df[bb]) {
            dbgout << " " << n->label();
        }
        dbgout << std::endl;
    }
    dbgout << "│   Cut edges:" << std::endl;
    for (auto cutEdge : cutEdges) {
        dbgout << "│   ├── " << cutEdge->src()->label() << "->" << cutEdge->dest()->label() << std::endl;
    }
#endif
}
