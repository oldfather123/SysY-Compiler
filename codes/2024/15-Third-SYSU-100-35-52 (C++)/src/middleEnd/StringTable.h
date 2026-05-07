#pragma once

#include "Variable.h"
#include <unordered_map>
#include <utility>

using std::unordered_map;

struct StringTableNode {
    unordered_map<string, Value*> variableTable;
    StringTableNode* father;
    StringTableNode(StringTableNode* father = nullptr) : father{ father } {};

    void insert(Value* allocaInst, string trueName);

    Value* lookUp(string name);
    static int tableNum;
    void print();
};
using StringTableNodePtr = StringTableNode*;
