#include "Block.h"

void BasicBlock::appendInstruction(shared_ptr<Instruction> instruction)
{
    if (!terminator) {
        instructions.emplace_back(instruction);
    }
    instruction->basicblock = this->instructions.front()->basicblock;
}

void BasicBlock::appendInstruction(vector<shared_ptr<Instruction>> instructionsToInsert)
{
    shared_ptr<Instruction> lastInstruction = instructions.empty() ? nullptr : instructions.back();
    auto basicBlockPtr = instructions.empty() ? nullptr : instructions.front()->basicblock;

    if (lastInstruction) {
        assert(lastInstruction->type == Br || lastInstruction->type == Return);
        instructions.pop_back();
        terminator = nullptr;
    }

    for (auto& instr : instructionsToInsert) {
        instr->basicblock = basicBlockPtr;
        instructions.emplace_back(instr);
    }

    if (lastInstruction) {
        instructions.emplace_back(lastInstruction);
        terminator = lastInstruction;
    }
}

void BasicBlock::prependInstructions(vector<shared_ptr<Instruction>> instructionsToInsert)
{
    assert(!instructions.empty());
    auto basicBlockPtr = instructions.front()->basicblock;

    for (auto& instr : instructionsToInsert) {
        instr->basicblock = basicBlockPtr;
    }

    instructions.insert(instructions.begin(), instructionsToInsert.begin(), instructionsToInsert.end());
}

void BasicBlock::insertInstructionBefore(shared_ptr<Instruction> instruction, shared_ptr<Instruction> before)
{
    auto it = find(instructions.begin(), instructions.end(), before);
    
    assert(it != instructions.end());
    
    instructions.insert(it, instruction);
    instruction->basicblock = before->basicblock;
}

void BasicBlock::removeInstruction(shared_ptr<Instruction> instruction)
{
    assert(instruction != terminator);

    auto it = find(instructions.begin(), instructions.end(), instruction);

    assert(it != instructions.end());
    
    instructions.erase(it);
}

void BasicBlock::setTerminator(shared_ptr<Instruction> instruction)
{
    if (!terminator) {
        terminator = instruction;
    }
}

void BasicBlock::print()
{
    label->print();

    if (!predBasicBlocks.empty()) {
        cout << "\t\t\t; preds = ";
        for (auto it = predBasicBlocks.begin(); it != predBasicBlocks.end(); ++it) {
            cout << "%" << (*it)->label->name;
            if (next(it) != predBasicBlocks.end()) {
                cout << ", ";
            }
        }
        cout << endl;
    }

    for (const auto& instruction : instructions) {
        instruction->print();
    }
}