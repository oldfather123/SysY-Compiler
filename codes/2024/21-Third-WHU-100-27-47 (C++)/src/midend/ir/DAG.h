#pragma once

#include "Common.h"
#include "Graph.h"

namespace ir {
    class DAGEdge : public std::enable_shared_from_this<DAGEdge> {
        friend class GraphNode<Instruction, DAGEdge>;

    public:
        DAGEdge(const InstPtr &src, const InstPtr &dest) : _src(src), _dest(dest) {
            dbgassert(src != nullptr && dest != nullptr, "Invalid null argument");
        }
        DAGEdge(const DAGEdge &other) = default;

    public: /* TODO */
        InstPtr _src;
        InstPtr _dest;
        static Ptr<DAGEdge> _create(const InstPtr &src, const InstPtr &dest) {
            return makePtr<DAGEdge>(src, dest);
        }
        static Ptr<DAGEdge> _copy(const Ptr<DAGEdge> &edge) {
            return makePtr<DAGEdge>(*edge);
        }

    public:
        const InstPtr &src() const { return _src; }
        const InstPtr &dest() const { return _dest; }
        void redirectSrc(const InstPtr &newSrc);
        void redirectDest(const InstPtr &newDest);
    };
}
