#include "DAG.h"
#include "Instruction.h"
using namespace ir;

void DAGEdge::redirectSrc(const InstPtr &newSrc) {
    auto self = thisPtr(DAGEdge);
    Instruction::removeEdge(self);
    self->_src = newSrc;
    Instruction::addEdge(self);
}
void DAGEdge::redirectDest(const InstPtr &newDest) {
    auto self = thisPtr(DAGEdge);
    Instruction::removeEdge(self);
    self->_dest = newDest;
    Instruction::addEdge(self);
}
