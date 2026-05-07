#include "CFG.h"
#include "Scopes.h"
using namespace ir;

void CFGEdge::redirectSrc(const BBPtr &newSrc) {
    auto self = thisPtr(CFGEdge);
    BasicBlock::removeEdge(self);
    self->_src = newSrc;
    BasicBlock::addEdge(self);
}
void CFGEdge::redirectDest(const BBPtr &newDest) {
    auto self = thisPtr(CFGEdge);
    BasicBlock::removeEdge(self);
    self->_dest = newDest;
    BasicBlock::addEdge(self);
}
