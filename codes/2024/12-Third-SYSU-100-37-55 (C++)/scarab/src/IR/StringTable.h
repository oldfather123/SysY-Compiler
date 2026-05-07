#pragma once

#include<unordered_map>
#include "Variable.h"

using std::unordered_map;

struct StringTableNode
{
    unordered_map<string, shared_ptr<Variable>> varMap;

    shared_ptr<StringTableNode> father;
    StringTableNode(shared_ptr<StringTableNode> father=nullptr): father{father}{};

    shared_ptr<Variable> lookUp(string name);
};
typedef shared_ptr<StringTableNode> StringTableNodePtr;