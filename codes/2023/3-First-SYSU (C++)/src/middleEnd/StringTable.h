#pragma once

#include<unordered_map>
#include "Variable.h"

using std::unordered_map;

struct StringTableNode
{
    unordered_map<string, shared_ptr<Variable>> variableTable;
    shared_ptr<StringTableNode> father;
    StringTableNode(shared_ptr<StringTableNode> father=nullptr): father{father}{};

    void insert(shared_ptr<Variable> variable);

    shared_ptr<Variable> lookUp(string name);
    static int tableNum;
    void print();
};
typedef shared_ptr<StringTableNode> StringTableNodePtr;
