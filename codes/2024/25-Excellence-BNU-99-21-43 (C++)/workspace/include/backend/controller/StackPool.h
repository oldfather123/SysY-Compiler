#pragma once

#include "AsmStackAllocator.h"

#include <queue>
#include <memory>

namespace Backend
{

    class StackPool
    {
    private:
        AsmStackAllocator *allocator;
        std::queue<AsmOperandStackVariable *> queue;

    public:
        explicit StackPool(AsmStackAllocator *allocator) : allocator(allocator) {}

        void push(AsmOperandStackVariable *stackVar)
        {
            queue.push(stackVar);
        }

        void clear()
        {
            while (!queue.empty())
            {
                // delete queue.front(); // NOTE: maybe this should be reserved for the AsmStackAllocator
                queue.pop();
            }
        }

        AsmOperandStackVariable *pop()
        {
            if (queue.empty())
            {
                return allocator->allocateForRegPreserve();
            }
            AsmOperandStackVariable *var = queue.front();
            queue.pop();
            return var;
        }

        ~StackPool()
        {
            // clear(); // NOTE: maybe this should be reserved for the AsmStackAllocator
        }
    };

}
