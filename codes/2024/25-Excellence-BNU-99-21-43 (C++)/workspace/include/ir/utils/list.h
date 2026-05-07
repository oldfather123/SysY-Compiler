#pragma once
#include "listnode.h"

namespace IR
{
    template <typename T>
    struct List
    {
        ListNode *headEmptyNode;
        ListNode *tailEmptyNode;
        int sz = 0;

        List()
        {
            sz = 0;
            headEmptyNode = new ListNode(0);
            tailEmptyNode = new ListNode(0);
            tailEmptyNode->prev = headEmptyNode;
            headEmptyNode->next = tailEmptyNode;
        }

        // 时间复杂度O(n)，慎用
        int size();

        bool empty() { return headEmptyNode->next == tailEmptyNode; }

        ListNode *begin() { return headEmptyNode->nextNode(); }

        ListNode *back() { return tailEmptyNode->prevNode(); }

        ListNode *end() { return tailEmptyNode; }

        ListNode *rbegin() { return tailEmptyNode->prevNode(); }

        ListNode *rend() { return headEmptyNode; }

        void insertAfter(ListNode *who, T node);

        void insertBefore(ListNode *who, T node);

        void pushBack(T node);

        void pushFront(T node);

        void remove(T node);
    };
}