#pragma once

namespace IR
{
    template<typename T>
    struct List;
    struct Instruction;
    struct Use;
    struct BasicBlock;
    struct ListNode
    {
        const int id = 0;
        ListNode *next = nullptr;
        ListNode *prev = nullptr;
        ListNode(const int x) : id(x), next(nullptr), prev(nullptr) {}
        ListNode(ListNode *ne, ListNode *pr, const int x) : id(x), next(ne), prev(pr) {}
        ListNode *nextNode();
        ListNode *prevNode();
        friend struct List<IR::Instruction *>;
        friend struct List<IR::Use *>;
        friend struct List<IR::BasicBlock *>;
    private:
        void insertBefore(ListNode *node);
        void insertAfter(ListNode *node);
        virtual void removeNode();
    };
}