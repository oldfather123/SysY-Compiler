#pragma once

#include "Common.h"
#include "Values.h"

namespace ir {
    class BasicBlock;
    class Function;
    class Instruction;
    class PhiInst;
    class CFGEdge;

    /// @brief Print orderable interface, for ordered printed IR
    template <typename T>
    class IPrintOrderable {
    protected:
        static unsigned _printOrderCounter;
        unsigned _printOrder; // Auto incremental using the counter

    public:
        static Vector<Ptr<T>> sortByPrintOrder(Set<Ptr<T>> tSet) {
            auto comp = [](const Ptr<T> &a, const Ptr<T> &b) {
                return a->_printOrder < b->_printOrder;
            };

            // Create a new set with custom comparator
            std::set<Ptr<T>, decltype(comp)> sortedSet(comp);
            sortedSet.insert(tSet.begin(), tSet.end());

            // Convert the sorted set to list
            Vector<Ptr<T>> sortedList;
            for (const Ptr<T> &t : sortedSet) {
                sortedList.push_back(t);
            }

            return sortedList;
        }
    };

    template <typename T, typename E>
    class GraphNode {
        friend class DAGEdge;
        friend class CFGEdge;

    private:
        // Helper function to perform DFS
        static void _topoSortDFS(Ptr<T> node, Set<Ptr<T>> &visited, std::stack<Ptr<T>> &Stack) {
            // Mark the current node as visited
            visited.insert(node);

            // Visit all the successors of node node
            for (auto &succ : node->succs()) {
                if (visited.find(succ) == visited.end()) {
                    _topoSortDFS(succ, visited, Stack);
                }
            }

            // All successors finished, push the current node to the stack
            Stack.push(node);
        }

    public:
        Set<Ptr<E>> _inEdges;
        Set<Ptr<E>> _outEdges;
        static Vector<Ptr<T>> _topoSort(Set<Ptr<T>> tSet) {
            std::stack<Ptr<T>> Stack;
            std::unordered_set<Ptr<T>> visited;

            // Call the recursive helper function for all nodes that have not been visited yet
            for (auto t : tSet) {
                if (visited.find(t) == visited.end()) {
                    _topoSortDFS(t, visited, Stack);
                }
            }

            // Pop elements from stack to get the topological order
            std::vector<Ptr<T>> order;
            while (!Stack.empty()) {
                order.push_back(Stack.top());
                Stack.pop();
            }

            return order;
        }

    public: /* constructors */
        GraphNode() { }

    public: /* properties */
        const Set<Ptr<E>> &inEdges() const { return _inEdges; }
        const Set<Ptr<E>> &outEdges() const { return _outEdges; }
        const Set<Ptr<T>> preds() const {
            Set<Ptr<T>> ret;
            for (auto &inEdge : _inEdges) {
                ret.insert(inEdge->src());
            }
            return ret;
        }
        const Set<Ptr<T>> succs() const {
            Set<Ptr<T>> ret;
            for (auto &outEdge : _outEdges) {
                ret.insert(outEdge->dest());
            }
            return ret;
        }
        const Ptr<T> &firstPred() const {
            dbgassert(!_inEdges.empty(), "No predecessor");
            return (*_inEdges.begin())->src();
        }
        const Ptr<T> &firstSucc() const {
            dbgassert(!_outEdges.empty(), "No successor");
            return (*_outEdges.begin())->dest();
        }
        const Ptr<E> &firstInEdge() const {
            dbgassert(!_inEdges.empty(), "No in edge");
            return *(_inEdges.begin());
        }
        const Ptr<E> &firstOutEdge() const {
            dbgassert(!_outEdges.empty(), "No out edge");
            return *(_outEdges.begin());
        }
        bool hasPred() const {
            return !_inEdges.empty();
        }
        bool hasSucc() const {
            return !_outEdges.empty();
        }
        unsigned predCount() const {
            return _inEdges.size();
        }
        unsigned succCount() const {
            return _outEdges.size();
        }

    public: /* methods */
        static Ptr<E> addEdge(const Ptr<E> &edge) {
            edge->_src->_outEdges.insert(edge);
            edge->_dest->_inEdges.insert(edge);
            return edge;
        }
        static Ptr<E> addEdge(const Ptr<T> &src, const Ptr<T> &dest) {
            return addEdge(E::_create(src, dest));
        }
        static Ptr<E> duplicateEdge(const Ptr<E> &edge) {
            return addEdge(E::_copy(edge));
        }
        static void removeEdge(const Ptr<E> &edge) {
            edge->_src->_outEdges.erase(edge);
            edge->_dest->_inEdges.erase(edge);
        }
        static void removeEdge(const Ptr<T> &src, const Ptr<T> &dest) {
            // Erase the edge whose dest is dest from src's out edges
            for (auto edge : src->_outEdges) {
                if (edge->dest() == dest) {
                    removeEdge(edge);
                    break;
                }
            }
        }
        static void replace(const Ptr<T> &node, const Ptr<T> &newNode) {
            auto inEdges = node->_inEdges;
            auto outEdges = node->_outEdges;
            for (auto inEdge : inEdges) {
                inEdge->redirectDest(newNode);
            }
            for (auto outEdge : outEdges) {
                outEdge->redirectSrc(newNode);
            }
        }
        static void remove(const Ptr<T> &node) {
            auto inEdges = node->inEdges();
            auto outEdges = node->outEdges();
            for (auto &inEdge : inEdges) {
                removeEdge(inEdge);
            }
            for (auto &outEdge : outEdges) {
                removeEdge(outEdge);
            }
        }
        static std::pair<Ptr<E>, Ptr<E>> splitEdge(const Ptr<E> &edge, const Ptr<T> &newNode) {
            auto src = edge->_src;
            auto dest = edge->_dest;
            auto e1 = edge;
            auto e2 = duplicateEdge(edge);
            e1->redirectDest(newNode);
            e2->redirectSrc(newNode);
            return {e1, e2};
        }
        static void insertAfter(const Ptr<T> &node, const Ptr<T> &newNode) {
            auto outEdges = node->_outEdges;
            for (auto &outEdge : outEdges) {
                outEdge->redirectSrc(newNode);
            }
            addEdge(node, newNode);
        }
        static void insertBefore(const Ptr<T> &node, const Ptr<T> &newNode) {
            auto inEdges = node->_inEdges;
            for (auto &inEdge : inEdges) {
                inEdge->redirectDest(newNode);
            }
            addEdge(newNode, node);
        }
    };
    template <>
    unsigned IPrintOrderable<BasicBlock>::_printOrderCounter;
    template <>
    unsigned IPrintOrderable<Function>::_printOrderCounter;
    template <>
    unsigned IPrintOrderable<Instruction>::_printOrderCounter;
} // namespace ir
