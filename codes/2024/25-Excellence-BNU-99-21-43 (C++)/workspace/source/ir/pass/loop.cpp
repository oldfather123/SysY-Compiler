#include "loop.h"

namespace IR
{
    void Loop::setParentLoop(Loop *parentLoop) { this->parentLoop = parentLoop; }

    void Loop::setLoopDepth(int loopDepth) { this->loopDepth = loopDepth; }

    BasicBlock *Loop::getHeaderBasicBlock() { return headerBasicBlock; }

    Loop *Loop::getParentLoop() { return parentLoop; }

    std::set<Loop *> Loop::getSubLoops() { return subLoops; }

    int Loop::getLoopDepth() { return loopDepth; }

    std::set<BasicBlock *> Loop::getBasicBlocks() { return basicBlocks; }
}