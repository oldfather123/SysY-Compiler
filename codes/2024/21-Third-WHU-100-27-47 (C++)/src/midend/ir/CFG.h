#pragma once

#include "Common.h"
#include "Graph.h"

namespace ir {
    class CFGUtils {
    public:
        static Set<BBPtr> cloneSubgraph(const Set<BBPtr> &subgraph) {
            Set<BBPtr> clonedSubgraph;
            for (auto bb : subgraph) {
                // clonedSubgraph.insert(bb->clone());
            }
            return clonedSubgraph;
        }
    };
    class CFGEdge : public std::enable_shared_from_this<CFGEdge> {
        friend class GraphNode<BasicBlock, CFGEdge>;

    public:
        BBPtr _src;
        BBPtr _dest;
        enum Tag {
            Unknown,
            BrUncond,
            BrTrue,
            BrFalse,
        };
        CFGEdge(const BBPtr &src, const BBPtr &dest) : _src(src), _dest(dest) {
            dbgassert(src != nullptr && dest != nullptr, "Invalid null argument");
        }
        static Ptr<CFGEdge> _create(const BBPtr &src, const BBPtr &dest) {
            return makePtr<CFGEdge>(src, dest);
        }
        static Ptr<CFGEdge> _copy(const Ptr<CFGEdge> &edge) {
            return makePtr<CFGEdge>(*edge);
        }

    private:
        Tag _tag{Unknown};
        Value _brCondition{};
        Vector<Value *> _parallelCopyValRefs{};
        Map<Ptr<const PhiInst>, Value *> _parallelCopyMap{}; // Parallel copy map for Phi, Key: destination register, Value: source value
        bool _isFake{false};                                 // True if this edge can never be accessed but is used to keep the connectivity of the graph

    public:
        ~CFGEdge() {
            for (auto val : _parallelCopyValRefs) {
                delete val;
            }
        }
        CFGEdge(const CFGEdge &other) {
            _src = other._src;
            _dest = other._dest;
            _tag = other._tag;
            _brCondition = other._brCondition;
            _isFake = other._isFake;
            for (auto pair : other._parallelCopyMap) {
                auto phi = pair.first;
                auto valPtr = pair.second;
                valPtr = new Value(*valPtr);
                _parallelCopyValRefs.push_back(valPtr);
                _parallelCopyMap.insert({phi, valPtr});
            }
        }

        BBPtr &src() { return _src; }
        const BBPtr &src() const { return _src; }
        BBPtr &dest() { return _dest; }
        const BBPtr &dest() const { return _dest; }

        Tag &tag() { return _tag; }
        const Tag &tag() const { return _tag; }
        Value &brCondition() { return _brCondition; }
        const Value &brCondition() const { return _brCondition; }
        bool &isFake() { return _isFake; }
        const bool &isFake() const { return _isFake; }

        void redirectSrc(const BBPtr &newSrc);
        void redirectDest(const BBPtr &newDest);

        Ptr<CFGEdge> setUncondBr() {
            _tag = BrUncond;
            _brCondition = Value{};
            return thisPtr(CFGEdge);
        }
        Ptr<CFGEdge> setCondBr(const Value &condition, const bool &condBool) {
            _tag = condBool ? BrTrue : BrFalse;
            _brCondition = condition;
            return thisPtr(CFGEdge);
        }
        const Value *getCopySrcValRef(const Ptr<const PhiInst> &phi) const { return _parallelCopyMap.at(phi); }
        Value *getCopySrcValRef(const Ptr<PhiInst> &phi) { return _parallelCopyMap.at(phi); }
        Value getCopySrcVal(const Ptr<PhiInst> &phi) { return *getCopySrcValRef(phi); }
        /// @return `true` if the parallel copy is actually added, `false` if the parallel copy already exists
        bool addParallelCopy(const Ptr<PhiInst> &phi, const Value &val) {
            if (_parallelCopyMap.count(phi) != 0) {
                return false;
            }
            auto valPtr = new Value(val);
            _parallelCopyValRefs.push_back(valPtr);
            _parallelCopyMap.insert({phi, valPtr});
            return true;
        }
        /// @return `true` if the parallel copy is actually updated, `false` if not
        bool updateParallelCopy(const Ptr<PhiInst> &phi, const Value &val) {
            dbgassert(_parallelCopyMap.count(phi) != 0, "Parallel copy not found");
            if (*_parallelCopyMap.at(phi) == val) {
                return false;
            }
            *_parallelCopyMap.at(phi) = val;
            return true;
        }
    };
} // namespace ir
