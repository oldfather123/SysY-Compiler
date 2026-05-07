#include "SSADestruct.h"

using namespace ir;

struct ParallelCopy {
    RegPtr dest;
    Value src;

    bool operator==(const ParallelCopy &other) const {
        return dest == other.dest && src == other.src;
    }
    bool operator!=(const ParallelCopy &other) const {
        return !(*this == other);
    }
};

namespace std {
    template <>
    struct hash<ParallelCopy> {
        size_t operator()(const ParallelCopy &pc) const {
            size_t result = 0;
            hashCombine(result, pc.dest);
            hashCombine(result, pc.src);
            return result;
        }
    };
}

void ir::ssaDestruct(FuncPtr func) {
    dbgout << std::endl
           << "SSADestruct pass started (" << func->name() << ")." << std::endl;

    // 并行复制
    Map<BBPtr, Set<ParallelCopy>> pcsMap{};
    auto bbSet = func->bbSet();
    for (auto bb : bbSet) {
        for (auto phi : bb->getPhiNodes()) {
            for (auto &tuple : phi->getTuples()) {
                // 判断关键边
                if (tuple.bb()->succs().size() > 1) {
                    // 关键边分割
                    Ptr<CFGEdge> edge = nullptr;
                    for (auto inEdge : bb->inEdges()) {
                        if (inEdge->src() == tuple.bb()) {
                            edge = inEdge;
                            break;
                        }
                    }
                    dbgassert(edge != nullptr, "Critical edge should be found, but not found");
                    auto splitBB = BasicBlock::create(func, "split");
                    // 分割边
                    auto splittedEdges = BasicBlock::splitEdge(edge, splitBB);
                    auto e2 = splittedEdges.second;
                    e2->setUncondBr();
                    // 插入并行复制
                    pcsMap[splitBB].insert(ParallelCopy{phi->destReg(), tuple.value()});
                } else {
                    // 直接插入并行复制
                    pcsMap[tuple.bb()].insert(ParallelCopy{phi->destReg(), tuple.value()});
                }
            }
            dbgout << "├── Removed phi node: " << phi->toString() << std::endl;
            Instruction::remove(phi);
        }
    }
    // 并行转串行
    for (auto bb : func->bbSet()) {
        auto &pcs = pcsMap[bb];
        auto insertPos = bb->exitInst();
        Map<RegPtr, Set<RegPtr>> srcDests{}; // Key: src, Value: dests
        for (auto it = pcs.begin(); it != pcs.end();) {
            auto &pc = *it;
            auto src = pc.src;
            auto dest = pc.dest;
            if (src.isLiteral()) {
                // 直接插入
                Instruction::insertBefore(insertPos, MoveInst::create(bb, dest, src));
                it = pcs.erase(it);
            } else {
                // 预处理 srcDests
                dbgassert(src.isRegister(), "Invalid source value");
                srcDests[src.getRegister()].insert(dest);
                ++it;
            }
        }
        // 序列化：每次选出度为 0 的点，类似拓扑排序；遇到环则拆开
        while (!pcs.empty()) {
            bool changed = false;
            for (auto it = pcs.begin(); it != pcs.end();) {
                auto &pc = *it;      // k.1 = k.4'
                auto src = pc.src;   // k.4'
                auto dest = pc.dest; // k.1
                if (srcDests[dest].size() == 0) {
                    // 出度为 0，可以插入
                    Instruction::insertBefore(insertPos, MoveInst::create(bb, dest, src));
                    it = pcs.erase(it);
                    if (src.isRegister()) {
                        srcDests[src.getRegister()].erase(dest);
                    }
                    changed = true;
                } else {
                    ++it;
                }
            }
            if (!changed) {
                // 有环，断开环
                auto &pc = *pcs.begin();         // k.1 = k.4
                auto src = pc.src.getRegister(); // k.4
                auto dest = pc.dest;             // k.1
                pcs.erase(pc);
                auto tmpReg = Register::create(func, src->dataType(), src->name() + "'"); // k.4'
                Instruction::insertBefore(insertPos, MoveInst::create(bb, tmpReg, src));  // k.4' = k.4
                pcs.insert(ParallelCopy{dest, Value(tmpReg)});                            // k.1 = k.4'
                srcDests[src].erase(dest);                                                // k.4 出度减 1
            }
        }
    }

    dbgout << "└── SSADestruct pass done." << std::endl;
}
