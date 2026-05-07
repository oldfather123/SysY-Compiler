#include "listnode.h"

namespace IR
{
    ListNode *ListNode::nextNode()
    {
        return next;
    }

    ListNode *ListNode::prevNode()
    {
        return prev;
    }

    void ListNode::insertAfter(ListNode *node)
    {
        if (node == nullptr)
            return;
        next = node->next;
        prev = node;
        if (node->next != nullptr)
        {
            node->next->prev = this;
        }
        node->next = this;
    }
    // 在 node 前插入当前节点
    void ListNode::insertBefore(ListNode *node)
    {
        if (node == nullptr)
            return;
        next = node;
        prev = node->prev;
        if (node->prev != nullptr)
        {
            node->prev->next = this;
        }
        node->prev = this;
    }

    void ListNode::removeNode()
    {
        if (prev != nullptr)
        {
            prev->next = next;
        }
        if (next != nullptr)
        {
            next->prev = prev;
        }
    }
};