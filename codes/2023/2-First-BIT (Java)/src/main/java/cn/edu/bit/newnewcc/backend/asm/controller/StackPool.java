package cn.edu.bit.newnewcc.backend.asm.controller;

import cn.edu.bit.newnewcc.backend.asm.allocator.StackAllocator;
import cn.edu.bit.newnewcc.backend.asm.operand.StackVar;

import java.util.ArrayDeque;
import java.util.Queue;

public class StackPool {
    private final StackAllocator allocator;
    private final Queue<StackVar> queue = new ArrayDeque<>();

    public StackPool(StackAllocator allocator) {
        this.allocator = allocator;
    }

    public void push(StackVar stackVar) {
        queue.add(stackVar);
    }

    public void clear() {
        queue.clear();
    }

    public StackVar pop() {
        if (queue.isEmpty()) {
            return allocator.pushEx();
        }
        return queue.remove();
    }
}
