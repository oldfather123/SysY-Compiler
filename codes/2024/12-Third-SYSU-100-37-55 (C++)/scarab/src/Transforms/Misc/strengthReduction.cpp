#include "strengthReduction.h"

bool isPowerOfTwo(int value) {
    return value > 0 && (value & (value - 1)) == 0;
}

void replaceMulWithShift(shared_ptr<Instruction>& mulInstr, ValuePtr operand, int shiftAmount, int& regCounter) {
    auto shiftConstant = Const::createConstant(Type::getInt(), shiftAmount, "strengthReduction%" + to_string(regCounter++));
    auto shiftInstr = new BinaryInstruction(operand, shiftConstant, ',', mulInstr->basicblock);
    replaceVarByVar(dynamic_cast<BinaryInstruction*>(mulInstr.get())->reg, shiftInstr->reg);
    mulInstr = shared_ptr<Instruction>(shiftInstr);
}

void strengthReduction(FunctionPtr func) {
    int regCounter = 0;

    for (auto& basicBlock : func->basicBlocks) {
        unordered_set<InstructionPtr> instructionsToRemove;

        for (size_t i = 0; i < basicBlock->instructions.size(); i++) {
            auto& instr = basicBlock->instructions[i];
            if (instr->type == Binary) {
                auto binaryInstr = dynamic_cast<BinaryInstruction*>(instr.get());
                auto leftOperand = binaryInstr->a;
                auto rightOperand = binaryInstr->b;

                if (binaryInstr->op == '*') {
                    if (leftOperand->isConst && rightOperand->type->ID == IntID) {
                        auto leftConst = dynamic_cast<Const*>(leftOperand.get());
                        if (isPowerOfTwo(leftConst->intVal)) {
                            int shiftAmount = int(log2(leftConst->intVal));
                            replaceMulWithShift(instr, rightOperand, shiftAmount, regCounter);
                        }
                    }
                    else if (rightOperand->isConst && leftOperand->type->ID == IntID) {
                        auto rightConst = dynamic_cast<Const*>(rightOperand.get());
                        if (i + 1 < basicBlock->instructions.size() && basicBlock->instructions[i + 1]->type == InsID::Binary) {
                            auto nextInstr = dynamic_cast<BinaryInstruction*>(basicBlock->instructions[i + 1].get());
                            if (nextInstr->op == '/' && nextInstr->b == rightOperand) {
                                replaceVarByVar(nextInstr->reg, binaryInstr->a);
                                instructionsToRemove.insert(basicBlock->instructions[i]);
                                instructionsToRemove.insert(basicBlock->instructions[i + 1]);
                                i++;
                                continue;
                            }
                        }
                        if (isPowerOfTwo(rightConst->intVal)) {
                            int shiftAmount = int(log2(rightConst->intVal));
                            replaceMulWithShift(instr, leftOperand, shiftAmount, regCounter);
                        }
                    }
                }
            }
        }
        vector<InstructionPtr> optimizedInstructions;
        for (auto& instr : basicBlock->instructions) {
            if (instructionsToRemove.find(instr) == instructionsToRemove.end()) {
                optimizedInstructions.push_back(instr);
            }
        }
        basicBlock->instructions = std::move(optimizedInstructions);
    }
}