#include "Loop.h"

Loop::Loop(shared_ptr<BasicBlock> header, int id): header(header), id(id), parentFunction(header->parentFunction), parentLoop(nullptr), inductionCondition(nullptr),
    inductionEnd(nullptr), inductionPhi(nullptr), iterationCount(0) {
    addBasicBlock(header);
}

bool Loop::dominates(BasicBlockPtr dominator, BasicBlockPtr dominated, BasicBlockPtr entryBlock) {
    if(dominator == entryBlock) {
        return true;
    }
    while(dominated != entryBlock) {
        if(dominated == dominator) {
            return true;
        }
        dominated = dominated->immediateDominator;
    }
    return false;
}

bool Loop::isLoopInvariant(ValuePtr value) {
    if(auto instr = value->I) {
        auto bb = instr->basicblock;
        return !this->contains(bb);
    }
    return true;
}

SCEV::SCEV(ValuePtr initial, ValuePtr step)
{
    scevValues.push_back(initial);
    scevValues.push_back(step);
}

SCEV::SCEV(ValuePtr initial, const SCEV& innerSCEV)
{
    scevValues.push_back(initial);
    scevValues.insert(scevValues.end(), innerSCEV.scevValues.begin(), innerSCEV.scevValues.end());

    std::set_union(calculatedInstructions.begin(), calculatedInstructions.end(),
                    innerSCEV.calculatedInstructions.begin(), innerSCEV.calculatedInstructions.end(),
                    std::inserter(calculatedInstructions, calculatedInstructions.begin()));
}

SCEV::SCEV(const vector<ValuePtr>& valueVector)
{
    scevValues.insert(scevValues.end(), valueVector.begin(), valueVector.end());
}

SCEV::SCEV(ValuePtr initial, ValuePtr step, std::set<BinaryInstruction*>&& binarys) 
    : calculatedInstructions(binarys)
{
    scevValues.push_back(initial);
    scevValues.push_back(step);
}

SCEV operator+(ValuePtr lhs, const SCEV& rhs)
{
    auto addInstr = new BinaryInstruction(lhs,rhs.at(0),'+',BasicBlockPtr(nullptr));
    std::vector<ValuePtr> initialValues = {addInstr->reg};
    initialValues.insert(initialValues.end(), rhs.scevValues.begin() + 1, rhs.scevValues.end());
    auto result = SCEV(initialValues);
    result.calculatedInstructions = rhs.calculatedInstructions;
    result.calculatedInstructions.insert(addInstr);
    return result;
}

SCEV operator+(const SCEV& lhs, const SCEV& rhs)
{
    auto maxSize = std::max(lhs.size(), rhs.size());
    vector<BinaryInstruction*> createdInstructions;
    vector<ValuePtr> initialValues;
    for(auto i = 0; i < maxSize; i++) {
        if(i < lhs.size() && i < rhs.size()) {
            auto lhsVal = lhs.at(i);
            auto rhsVal = rhs.at(i);

            if (lhsVal->isConst && rhsVal->isConst) {
                auto constLhs = dynamic_cast<Const*>(lhsVal.get());
                auto constRhs = dynamic_cast<Const*>(rhsVal.get());
                initialValues.push_back(make_shared<Const>(constLhs->intVal + constRhs->intVal));
                continue;
            }

            auto addInstr = make_shared<BinaryInstruction>(lhsVal, rhsVal, '+', BasicBlockPtr(nullptr));
            createdInstructions.push_back(addInstr.get());
            initialValues.push_back(addInstr->reg);
        }
        else {
            initialValues.push_back(i < lhs.size() ? lhs.at(i) : rhs.at(i));
        }
    }

    auto result = SCEV(initialValues);
    std::set_union(lhs.calculatedInstructions.begin(), lhs.calculatedInstructions.end(),
                    rhs.calculatedInstructions.begin(), rhs.calculatedInstructions.end(),
                    std::inserter(result.calculatedInstructions, result.calculatedInstructions.begin()));
    for(auto instr : createdInstructions)
        result.calculatedInstructions.insert(instr);
    return result;
}

SCEV operator-(const SCEV& lhs, ValuePtr rhs)
{
    auto subInstr = new BinaryInstruction(lhs.at(0),rhs,'-',BasicBlockPtr(nullptr));
    std::vector<ValuePtr> initialValues = {subInstr->reg};
    initialValues.insert(initialValues.end(), lhs.scevValues.begin() + 1, lhs.scevValues.end());
    auto result = SCEV(initialValues);
    result.calculatedInstructions = lhs.calculatedInstructions;
    result.calculatedInstructions.insert(subInstr);
    return result;
}

SCEV operator-(const SCEV& lhs, const SCEV& rhs)
{
    auto maxSize = std::max(lhs.size(), rhs.size());
    std::vector<BinaryInstruction*> createdInstructions;
    std::vector<ValuePtr> initialValues;
    for (size_t i = 0; i < maxSize; i++) {
        if (i < lhs.size() && i < rhs.size()) {
            auto lhsVal = lhs.at(i);
            auto rhsVal = rhs.at(i);

            if (lhsVal->isConst && rhsVal->isConst) {
                auto constLhs = dynamic_cast<Const*>(lhsVal.get());
                auto constRhs = dynamic_cast<Const*>(rhsVal.get());
                initialValues.push_back(make_shared<Const>(constLhs->intVal - constRhs->intVal));
                continue;
            }

            auto subInstr = make_shared<BinaryInstruction>(lhsVal, rhsVal, '-', BasicBlockPtr(nullptr));
            createdInstructions.push_back(subInstr.get());
            initialValues.push_back(subInstr->reg);
        } else {
            initialValues.push_back(i < lhs.size() ? lhs.at(i) : rhs.at(i));
        }
    }

    auto result = SCEV(initialValues);
    std::set_union(lhs.calculatedInstructions.begin(), lhs.calculatedInstructions.end(),
                    rhs.calculatedInstructions.begin(), rhs.calculatedInstructions.end(),
                    std::inserter(result.calculatedInstructions, result.calculatedInstructions.begin()));
    for(auto instr : createdInstructions)
        result.calculatedInstructions.insert(instr);

    return result;
}

SCEV operator*(ValuePtr lhs, const SCEV& rhs)
{
    std::vector<BinaryInstruction*> createdInstructions;
    std::vector<ValuePtr> initialValues;
    for(auto val : rhs.scevValues) {
        if (lhs->isConst && val->isConst) {
            auto constLhs = dynamic_cast<Const *>(lhs.get());
            auto constRhs = dynamic_cast<Const *>(val.get());
            initialValues.push_back(ValuePtr(new Const(constLhs->intVal * constRhs->intVal)));
            continue;
        }
        auto mulInstr = new BinaryInstruction(lhs, val, '*', BasicBlockPtr(nullptr));
        createdInstructions.push_back(mulInstr);
        initialValues.push_back(mulInstr->reg);
    }

    auto result = SCEV(initialValues);
    result.calculatedInstructions = rhs.calculatedInstructions;

    for(auto instr : createdInstructions)
        result.calculatedInstructions.insert(instr);
    
    return result;

}

void SCEV::clear()
{
    for (auto instr : calculatedInstructions) {
        if (!instr->basicblock) {
            for (int i = 0; i < instr->getOperandCount(); ++i) {
                auto operand = instr->getOperand(i);
                if (operand) {
                    deleteUser(operand);
                }
            }
        }
    }
    calculatedInstructions.clear();
    scevValues.clear();
}

SCEV& Loop::getSCEV(Instruction* instr)
{
    return scevMap[instr];
}

bool Loop::hasSCEV(Instruction* instr)
{
    return scevMap.count(instr);
}

void Loop::registerSCEV(Instruction* instr, SCEV scev)
{
    scevMap[instr] = scev;
}