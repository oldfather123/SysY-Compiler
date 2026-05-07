#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#include "Instruction.h"
#include <deque>
#include <iostream>
#include <set>
#include <vector>

class BasicBlock {
public:
    std::string comment;    // used for debug
    int block_id = 0;
    int dfs_id = 0; // 标志是否已经dfs过了，数值代表序号
    std::deque<Instruction> Instruction_list{};

    /*
        pos = 1 -> end   pos = 0 -> begin
        inst1  <- front
        ...
        inst5  <- back
    */
    void InsertInstruction(int pos, Instruction Ins);

    void printIR(std::ostream &s);
    BasicBlock(int id) : block_id(id) {dfs_id=0;}

	bool operator==(const BasicBlock& other) const {
        return block_id == other.block_id;
    }
};
typedef BasicBlock *LLVMBlock;

// used for unordered_set<LLVMBlock>
namespace std {
    template <>
    struct hash<BasicBlock> {
        std::size_t operator()(const BasicBlock& bb) const {
            return std::hash<int>()(bb.block_id);
        }
    };
}

#endif