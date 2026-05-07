#include "StringTable.h"
// NOLINTBEGIN
void StringTableNode::insert(Value* allocaInst,string trueName) {
    if(variableTable[trueName]) {
        // std::cerr<<"variable "<<trueName<<" insert error!"<<endl;
        assert(false&&"variable insert error!");
        return;
    }
    variableTable[trueName] = allocaInst;
}

Value* StringTableNode::lookUp(string name) {
    if(variableTable.find(name) != variableTable.end()) {
        return variableTable[name];
    }
    if(father) {
        return father->lookUp(name);
    }  // std::cerr<<"variable "<<name<<" find error!"<<endl;
    return nullptr;
}

int StringTableNode::tableNum = 0;

// void StringTableNode::print() {
//     std::cerr << tableNum++ << endl;
//     for(auto& it : variableTable) {
//         std::cerr << "  " << it.first << ": " << it.second->getTypeStr() << endl;
//     }
//     std::cerr << endl << endl;
// }
// NOLINTEND