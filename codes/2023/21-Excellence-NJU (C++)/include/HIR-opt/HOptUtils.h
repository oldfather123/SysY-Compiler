//
// Created by q1w2e3r4 on 23-7-28.
//
#include "ValueRef.h"
#include "IRInstruction.h"
#ifndef COMPILER_HOPTUTILS_H
#define COMPILER_HOPTUTILS_H

//Replace oldVal with newVal in all usages of oldVal
void replaceAllUsesOf(ValueRef* oldVal, ValueRef* newVal);

// delete this block, and update all blocks pred/succ it;
void DelBlock(BasicBlock* block);

// 删除本指令在ref中的使用(use)
void DelUseOfInstruction(Instruction *instr);

// 删除本指令在ref中的使用(use)+此指令的位置，相当于移除整个指令
void DeleteInstruction(Instruction* instr);

// 将指令从当前位置删除，但不删除用法
void Remove_LocalInstr(Instruction *instr);

// 删除在ref 的use列表中删除instr
void DelUse(ValueRef* ref, Instruction* instr);

// 解绑两个块的联系(PHI指令会对应修改)
void UnlinkBlock(BasicBlock* pred, BasicBlock* succ);

// remove itself in to(succ) block, usually used in del of this block
void OptPhi(BasicBlock* from,BasicBlock* to);

// 原来关系为from -> to,现在在中间插入一个块变成 from -> insert -> to
void Insert_Block(BasicBlock* from, BasicBlock *to, BasicBlock *insert);

// 在一个块的最前面插入指令（同时修改instr所属块的信息（最好是调这个省的你忘了））
void Insert_instr_atFront(Instruction* instr, BasicBlock* block);

// 在一个块的最末端插入指令，如果最后一句是跳转，就插在倒数第二句（同时修改instr所属块的信息（最好是调这个省的你忘了））
void Insert_instr_beforeBr(Instruction* instr, BasicBlock* block);

// 在原位置替换指令
void ReplaceInstr(Instruction* old, Instruction* now);

#endif //COMPILER_HOPTUTILS_H
