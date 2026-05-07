//
// Created by q1w2e3r4 on 23-7-17.
//

#include "Function.h"
#include "IRInstruction.h"
#include "Type.h"
#include <queue>
Function::Function(string name, FuncType *funcType) {
    this->name = std::move(name);
    this->funcType = funcType;
    this->dom_root = nullptr;
    this->entry = nullptr;
}

void Function::appendBlock(BasicBlock *block) {
    this->block_list.push_back(block);
}


void Function::Emit(std::ostream &os) {
    os << this->debugInfo;
    for(BasicBlock *block : block_list){
        block -> Emit(os);
    }
    // TODO
    if (this->debugInfo.length() != 0) os << "}" << endl;
}

void Function::PrintSymbol() {
    cout << this->name << " Symbols:" << endl;
    for(const auto& it:symbol_table){
        cout << it.first << ": " << it.second->symbolType->getSize() << endl;
    }
}

void Function::codegen(AsmBuilder *builder) {
        auto cur_unit = builder->getUnit();
        auto cur_func = new MachineFunction(cur_unit, this);
        builder->setFunction(cur_func);

//        z:offset is the size of the func?
        int offset = 0;
        std::map<std::string, int> offset_table;
        std::map<std::string, int> size_table;
        for (auto &[sym_name, sym] : symbol_table) {
            // std::cerr << sym_name << ' ' << sym->symbolType->getSize() << endl;
            offset_table[sym_name] = offset;
            int size = sym->symbolType->getSize();
            size_table[sym_name] = size;
            offset += size;
        }
        cur_func->framesize = offset + 16;
//        func->codegen call block->codegen where is the first block?
        // block_list[0]->codegen(builder, offset_table, size_table, offset + 16,this->funcType->arguments, true);

        // TODO: prepare for multi blocks
       for(int i=0;i<block_list.size();i++){
           block_list[i]->codegen(builder, offset_table, size_table, offset + 16, this->funcType->arguments, i == 0);
       }

        cur_unit->insert_func(cur_func);
    }

static void dfs(BasicBlock *cur, std::vector<BasicBlock *> &result, std::set<BasicBlock *> &visited) {
    // std::cerr << cur->label << ' ';
    visited.insert(cur);
    for (auto bb : cur->succ)
        if (visited.find(bb) == visited.end()) dfs(bb, result, visited);
    result.push_back(cur);
}

std::vector<BasicBlock *> Function::getReversePostOrder() {
    std::vector<BasicBlock *> result{};
    std::set<BasicBlock *> visited{};
    // std::cerr << "traverse func \"" << this->name << "\" ";
    dfs(block_list[0], result, visited);
    // std::cerr << std::endl;
    return std::vector<BasicBlock *>(result.rbegin(), result.rend());
}
