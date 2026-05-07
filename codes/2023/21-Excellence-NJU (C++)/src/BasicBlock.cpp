//
// Created by q1w2e3r4 on 23-7-9.
//

#include "BasicBlock.h"
#include "IRInstruction.h"
#include "Type.h"
#include "Utils.h"
#include "codegen/AsmBuilder.hpp"
#include "codegen/MachineBlock.hpp"
#include "codegen/MachineInstruction.hpp"
#include "codegen/MachineOperand.hpp"
#include <memory>
#include <string>
#include "IRInstruction.h"

using namespace std;

BasicBlock::BasicBlock(std::string& label,Function * func) {
    this->label = Utils::getNewLabel(label);
    this->func_belong = func;
    this->type = Block;
}

void BasicBlock::appendCode(Instruction* instruction) {
    this->local_instr.push_back(instruction);
}

void BasicBlock::insertCodeAtFront(Instruction *instruction) {
    auto& v = this->local_instr;
    for(auto it = v.begin(); it != v.end(); it++){
        if((*it)->instType != PHI){
            v.insert(it,instruction);
            return;
        }
    }

    v.insert(v.end(),instruction);
}

void BasicBlock::Emit(std::ostream &os) {
    os << this->label + ":            ";
    if(this->pred.empty()){
        os << ";No predecessor!!";
    }
    else{
        os << ";preds: ";
        for(BasicBlock* block : pred) {
            os << block->label << ',';
        }
    }

    os << "    succs:";
    for(BasicBlock* block : succ){
        os << block->label << ',';
    }
    os << endl;
    for(auto instr : this->local_instr){
        os << "  ";
        instr->outPut(os);
    }
}

void BasicBlock::codegen(AsmBuilder *builder,
                         std::map<std::string, int> &offset_table,
                         std::map<std::string, int> &size_table,
                         int framesize,
                         vector<Type*> &arguments,
                         bool is_entry) {
    if (this->local_instr.empty()) this->local_instr.push_back(new RetInstruction(nullptr));
    auto cur_func = builder->getFunction();
    auto cur_block = new MachineBlock(cur_func, this);
    mBlock = cur_block, cur_block->bb = this;
    builder->setBlock(cur_block);
    if (is_entry) {
        auto sd1 = new StoreMInstruction(cur_block,
                                                StoreMInstruction::SD,
                                                new MachineOperand(MachineOperand(MachineOperand::REG, IREGISTER::RA)),
                                                new MachineOperand(MachineOperand::IMM, - 8),
                                                new MachineOperand(MachineOperand::REG, IREGISTER::SP));
        cur_block->insertInst(sd1);
        auto sd2 = new StoreMInstruction(cur_block,
                                                StoreMInstruction::SD,
                                                new MachineOperand(MachineOperand(MachineOperand::REG, IREGISTER::FP)),
                                                new MachineOperand(MachineOperand::IMM, - 16),
                                                new MachineOperand(MachineOperand::REG, IREGISTER::SP));
        cur_block->insertInst(sd2);
        auto adds0 = new BinaryMInstruction(cur_block,
                                           BinaryMInstruction::ADD,
                                           new MachineOperand(MachineOperand::REG, IREGISTER::FP), //x2(sp)
                                           new MachineOperand(MachineOperand::REG, IREGISTER::SP),//x2(sp)
                                           new MachineOperand(MachineOperand::REG, IREGISTER::ZERO));
        cur_block->insertInst(adds0);
        auto addi = new BinaryMInstruction(cur_block,
                                                BinaryMInstruction::ADDI,
                                                new MachineOperand(MachineOperand::REG, IREGISTER::SP), //x2(sp)
                                                new MachineOperand(MachineOperand::REG, IREGISTER::SP),//x2(sp)
                                                new MachineOperand(MachineOperand::IMM, -framesize));
        cur_block->insertInst(addi);



//        arguments
        for(int i=0; i<arguments.size() && i <=7; i++){
            if(Type_Enum(arguments[i]->type) == INT32TYPE){
                auto addArgument = new BinaryMInstruction(cur_block,
                                                    BinaryMInstruction::ADDI,
                                                    new MachineOperand(MachineOperand::VREG, "%" + to_string(i)), //x2(sp)
                                                    new MachineOperand(MachineOperand::REG, IREGISTER::A0+i),//x2(sp)
                                                    new MachineOperand(MachineOperand::IMM, 0));
                cur_block->insertInst(addArgument);
            }
            else if(Type_Enum(arguments[i]->type) == POINTERTYPE){
                auto addArgument = new BinaryMInstruction(cur_block,
                                                          BinaryMInstruction::ADDI,
                                                          new MachineOperand(MachineOperand::VREG, "%" + to_string(i)),
                                                          new MachineOperand(MachineOperand::REG, IREGISTER::A0+i),
                                                          new MachineOperand(MachineOperand::IMM, 0));
                cur_block->insertInst(addArgument);
            }else if(Type_Enum(arguments[i]->type) == FLOATTYPE){
                auto fmv = new MoveMInstruction(cur_block,
                                                MoveMInstruction::FMV,
                                                new MachineOperand(MachineOperand::FVREG, "%" + to_string(i)),
                                                new MachineOperand(MachineOperand::REG, Fregister::fa0+i,true));
                cur_block->insertInst(fmv);
            }
        }
        if(arguments.size() >= 8){
            int offset = 0;
            for(int i = 8; i < arguments.size(); i++){
                if(Type_Enum(arguments[i]->type) == INT32TYPE){
                    if (offset <= 2032 && false) {
                        auto lw1 = new LoadMInstruction(cur_block,
                                                    LoadMInstruction::LW,
                                                    new MachineOperand(MachineOperand::VREG, "%" + to_string(i)),
                                                    new MachineOperand(MachineOperand::IMM, offset),
                                                    new MachineOperand(MachineOperand::REG, IREGISTER::FP));
                        cur_block->insertInst(lw1);
                    } else {
                        auto li = new LoadImmInstruction(cur_block, LoadImmInstruction::LI,
                                        new MachineOperand(MachineOperand::VREG, "%" + to_string(i)),
                                        new MachineOperand(MachineOperand::IMM, offset));
                        cur_block->insertInst(li);
                        auto add = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD,
                                            new MachineOperand(MachineOperand::VREG, "%" + to_string(i)),
                                            new MachineOperand(MachineOperand::VREG, "%" + to_string(i)),
                                            new MachineOperand(MachineOperand::REG, IREGISTER::FP));
                        cur_block->insertInst(add);
                        auto lw1 = new LoadMInstruction(cur_block,
                                                    LoadMInstruction::LW,
                                                    new MachineOperand(MachineOperand::VREG, "%" + to_string(i)),
                                                    new MachineOperand(MachineOperand::IMM, 0),
                                                    new MachineOperand(MachineOperand::VREG, "%" + to_string(i)));
                        cur_block->insertInst(lw1);
                    }
                    offset += 8;
                }
                else if(Type_Enum(arguments[i]->type) == POINTERTYPE){
                    if (offset <= 2032 && false) {
                        auto ld1 = new LoadMInstruction(cur_block,
                                                    LoadMInstruction::LD,
                                                    new MachineOperand(MachineOperand::VREG, "%" + to_string(i)),
                                                    new MachineOperand(MachineOperand::IMM, offset),
                                                    new MachineOperand(MachineOperand::REG, IREGISTER::FP));
                        cur_block->insertInst(ld1);
                    } else {
                        auto li = new LoadImmInstruction(cur_block, LoadImmInstruction::LI,
                                        new MachineOperand(MachineOperand::VREG, "%"s + to_string(i)),
                                        new MachineOperand(MachineOperand::IMM, offset));
                        cur_block->insertInst(li);
                        auto add = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD,
                                            new MachineOperand(MachineOperand::VREG, "%"s + to_string(i)),
                                            new MachineOperand(MachineOperand::VREG, "%"s + to_string(i)),
                                            new MachineOperand(MachineOperand::REG, IREGISTER::FP));
                        cur_block->insertInst(add);
                        auto ld1 = new LoadMInstruction(cur_block,
                                                    LoadMInstruction::LD,
                                                    new MachineOperand(MachineOperand::VREG, "%"s + to_string(i)),
                                                    new MachineOperand(MachineOperand::IMM, 0),
                                                    new MachineOperand(MachineOperand::VREG, "%"s + to_string(i)));
                        cur_block->insertInst(ld1);
                    }
                    offset += 8;
                }
                else if (Type_Enum(arguments[i]->type) == FLOATTYPE) {
                        if (offset <= 2032) {
                        auto flw1 = new LoadMInstruction(cur_block,
                                                    LoadMInstruction::FLW,
                                                    new MachineOperand(MachineOperand::FVREG, "%" + to_string(i)),
                                                    new MachineOperand(MachineOperand::IMM, offset),
                                                    new MachineOperand(MachineOperand::REG, IREGISTER::FP));
                        cur_block->insertInst(flw1);
                    } else {
                        auto li = new LoadImmInstruction(cur_block, LoadImmInstruction::LI,
                                        new MachineOperand(MachineOperand::VREG, IRInstruction::getVRegister()),
                                        new MachineOperand(MachineOperand::IMM, offset));
                        cur_block->insertInst(li);
                        auto add = new BinaryMInstruction(cur_block, BinaryMInstruction::ADD,
                                            new MachineOperand(MachineOperand::VREG, IRInstruction::getLastVRegister()),
                                            new MachineOperand(MachineOperand::VREG, IRInstruction::getLastVRegister()),
                                            new MachineOperand(MachineOperand::REG, IREGISTER::FP));
                        cur_block->insertInst(add);
                        auto flw1 = new LoadMInstruction(cur_block,
                                                    LoadMInstruction::FLW,
                                                    new MachineOperand(MachineOperand::FVREG, "%" + to_string(i)),
                                                    new MachineOperand(MachineOperand::IMM, 0),
                                                    new MachineOperand(MachineOperand::VREG, IRInstruction::getLastVRegister()));
                        cur_block->insertInst(flw1);
                    }
                    offset += 8;
                }
            }
        }

    }

    for (auto inst : local_instr) {
        inst->codegen(builder,offset_table, size_table, framesize);
    }

    cur_func->insert_block(cur_block);
}

