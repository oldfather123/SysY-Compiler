#include "Function.h"
#include "Instruction.h"
#include "phiEliminate.h"

// reference: https://github.com/dtcxzyw/cmmc/blob/3888660547f82af579fb9013cdfefea52fd923f8/cmmc/Transforms/DCE/PhiEliminate.cpp#L4

std::pair<NodeIndex, std::vector<NodeIndex>> calcSCC(const Graph& graph) {   // NOLINT
    const auto size = graph.size();
    std::vector<NodeIndex> dfn(size), low(size), st(size), col(size);
    NodeIndex top = 0, ccnt = 0, icnt = 0;
    std::vector<bool> flag(size);
    const auto dfs = [&](auto&& self, NodeIndex u) -> void {
        dfn[u] = low[u] = ++icnt;
        flag[u] = true;
        st[top++] = u;
        for(auto v : graph[u]) {
            if(dfn[v]) {
                if(flag[v])
                    low[u] = std::min(low[u], dfn[v]);
            } else {
                self(self, v);
                low[u] = std::min(low[u], low[v]);
            }
        }
        if(dfn[u] == low[u]) {
            NodeIndex c = ccnt++, v;
            do {
                v = st[--top];
                flag[v] = false;
                col[v] = c;
            } while(u != v);
        }
    };

    for(NodeIndex i = 0; i < size; ++i)
        if(!dfn[i])
            dfs(dfs, i);
    return { ccnt, std::move(col) };
}
static std::vector<std::unordered_set<Value*>> calcSubgraphSCC(const std::unordered_set<Value*>& set) {
    uint32_t idx = 0;
    std::unordered_map<Value*, uint32_t> idxMap;
    Graph graph;
    auto getNode = [&](Value* u) {
        if(auto iter = idxMap.find(u); iter != idxMap.cend())
            return iter->second;
        const auto id = idx++;
        idxMap[u] = id;
        while(graph.size() <= id)
            graph.push_back({});
        return id;
    };
    auto addEdge = [&](Value* u, Value* v) {
        auto ui = getNode(u);
        auto vi = getNode(v);
        graph[ui].push_back(vi);
    };

    for(auto inst : set) {
        auto phi = inst->as<PhiInstruction>();
        for(auto [pred, val] : phi->incomings()) {
            addEdge(phi, val->val);
        }
    }

    const auto n = idx;
    std::vector<Value*> invMap(n);
    for(auto [k, v] : idxMap)
        invMap[v] = k;

    auto [ccnt, col] = calcSCC(graph);
    std::vector<std::unordered_set<Value*>> groups(ccnt);
    for(uint32_t i = 0; i < n; ++i)
        groups[col[i]].insert(invMap[i]);
    return groups;
}


static bool processSCC(const std::unordered_set<Value*>& scc) {
    if(scc.size() == 1)  // trivial phi func/outer inst
        return false;

    std::unordered_set<Value*> inner;
    std::unordered_set<Value*> outerOps;
    for(auto phi : scc) {
        const auto phiInst = phi->as<PhiInstruction>();
        bool isInner = true;
        for(auto [pred, val] : phiInst->incomings()) {
            if(!scc.count(val->val)) {
                isInner = false;
                outerOps.insert(val->val);
            }
        }
        if(isInner)
            inner.insert(phi);
    }

    if(outerOps.size() == 1) {
        const auto rep = *outerOps.begin();
        for(auto val : scc) {
            val->as<Instruction>()->replaceWith(rep);
        }
        return true;
    }
    if(outerOps.size() > 1) {
        return removeRedundantPhis(inner);
    }
    return false;
}

static bool removeRedundantPhis(const std::unordered_set<Value*>& scc) {
    if(scc.empty())
        return false;
    auto groups = calcSubgraphSCC(scc);
    bool modified = false;
    // in topological order
    for(auto iter = groups.rbegin(); iter != groups.rend(); ++iter)
        modified |= processSCC(*iter);
    return modified;
}

bool runPhiEliminate(FunctionPtr func) {
    std::unordered_set<Value*> phis;
    for(auto& block : func->basicBlocks)
        for(auto& inst : block->instructionsRef()) {
            if(inst->insId == InsID::Phi) {
                phis.insert(inst.get());
            } else
                break;
        }

    return removeRedundantPhis(phis);
}