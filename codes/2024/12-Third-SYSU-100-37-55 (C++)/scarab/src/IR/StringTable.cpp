#include "StringTable.h"

shared_ptr<Variable> StringTableNode::lookUp(string name)
{
    if (varMap.find(name) != varMap.end()){
        return varMap[name];
    }

    if (father){
        return father->lookUp(name);
    }
    else{
        return nullptr;
    }
    
}