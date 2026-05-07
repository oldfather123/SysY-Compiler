//
// Created by q1w2e3r4 on 23-7-25.
//

#include "BasicBlock.h"

#include <vector>
#ifndef COMPILER_DOMNODE_H
#define COMPILER_DOMNODE_H

class DomNode {
public:
    int nodeId;
    BasicBlock * bb;

    std::vector<DomNode*> DOM; // other nodes dominate this
    std::vector<DomNode*> child; // in DOM tree,this dominates others
    DomNode* IDOM;
    std::vector<DomNode*> DF;

    int depth;

    DomNode(int id,BasicBlock * block);

    bool dominate(DomNode * d){
        return std::find(DOM.begin(), DOM.end(),d) != DOM.end();
    }
};


#endif //COMPILER_DOMNODE_H
