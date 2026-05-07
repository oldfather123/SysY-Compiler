#include "list.h"
#include "use.h"
#include "instruction.h"
#include "basicblock.h"

namespace IR
{
    template <typename T>
    int List<T>::size()
    {
        return sz;
    }

    template <typename T>
    void List<T>::insertAfter(ListNode *who, T node)
    {
        sz++;
        assert(node->id != 0);
        node->insertAfter(who);
    }

    template <typename T>
    void List<T>::insertBefore(ListNode *who, T node)
    {
        sz++;
        assert(node->id != 0);
        node->insertBefore(who);
    }

    template <typename T>
    void List<T>::pushBack(T node)
    {
        sz++;
        assert(node->id != 0);
        if (node == nullptr)
            return;
        node->insertBefore(tailEmptyNode);
    }

    template <typename T>
    void List<T>::pushFront(T node)
    {
        sz++;
        assert(node->id != 0);
        if (node == nullptr)
            return;
        node->insertAfter(headEmptyNode);
    }

    template <typename T>
    void List<T>::remove(T node)
    {
        sz--;
        node->removeNode();
    }

    template struct List<IR::Instruction *>;
    template struct List<IR::Use *>;
    template struct List<IR::BasicBlock *>;
}