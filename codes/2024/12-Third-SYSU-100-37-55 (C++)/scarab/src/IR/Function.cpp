#include "Function.h"

void Function::initializeReturnBlock(){
    returnBasicBlock = std::make_shared<BasicBlock>(std::make_shared<Label>("return"));
}

Function::Function() : registerCount(0) {
    basicBlocks.emplace_back(std::make_shared<BasicBlock>());
}

Function::Function(TypePtr returnType, string name, vector<ValuePtr> formalArguments) : returnValue{ValuePtr(new Reg(returnType, "retval"))}, name{name}, formalArguments{formalArguments}, isLibraryFunction{false}, isReentrant{true}
{
};

void Function::processReturnBlock()
{
    if (returnValue->type->isVoid()) {
        returnBasicBlock->setTerminator(std::make_shared<ReturnInstruction>(returnValue, returnBasicBlock));
    }
    else {
        auto loadInst = std::make_shared<LoadInstruction>(returnValue, createRegister(returnValue->type), returnBasicBlock);
        returnBasicBlock->appendInstruction(loadInst);
        returnBasicBlock->setTerminator(std::make_shared<ReturnInstruction>(loadInst->to, returnBasicBlock));
    }
    addBasicBlock(returnBasicBlock);
}

void Function::print()
{
    if (isLibraryFunction) {
        cout << "declare " << returnTypeToString() << "(";
        for (int i = 0; i < formalArguments.size(); i++) {
            if (i > 0)
                cout << ", ";
            cout << formalArguments[i]->type->toString();
        }
        cout << ")" << endl;
    }
    else {
        cout << "define " << returnValue->type->toString() << " @" << this->name << "(";
        for (int i = 0; i < formalArguments.size(); i++) {
            if (i > 0)
                cout << ", ";
            cout << formalArguments[i]->returnTypeToString();
        }
        cout << ") {" << endl;

        for (int i = 0; i < basicBlocks.size(); i++) {
            basicBlocks[i]->print();
            if (i != basicBlocks.size() - 1)
                cout << endl;
        }
        cout << "}" << endl << endl;
    }
}

void Function::clearCallerAndCalleeInfo() {
    callerMap.clear();
    calleeMap.clear();
    callerInstructions.clear();
}

void Function::calculateCallerAndCalleeInfo(){
    auto curFunction = shared_from_this();
    for(auto bb : basicBlocks) {
        bb->parentFunction = curFunction;
        for(auto inst : bb->instructions) {
            if(inst->type == Call){
                CallInstruction* callInst = dynamic_cast<CallInstruction*>(inst.get());
                calleeMap[callInst->func]++;

                callInst->func->callerMap[curFunction]++;
                callInst->func->callerInstructions.insert(inst);
            }
        }
    }
}


void Function::assignBlocksToFunction(){
    for(auto bb:basicBlocks) {
        bb->parentFunction = shared_from_this();
    }
}

void Function::initializeLabelToBlockMap(){
    for (const auto& block : basicBlocks) {
        LabelToBBMap[block->label] = block;
    }
}

void Function::addGlobalVariable(VariablePtr variable)
{
    variableMap[variable->name]= variable;
}

VariablePtr Function::findVariable(string variableName)
{
    auto it = variableMap.find(variableName);
    return (it != variableMap.end()) ? it->second : nullptr;
}

void Function::allocateLocalVariables()
{
    auto tmp = basicBlocks[0]->instructions;
    basicBlocks[0]->instructions.clear();
    for (auto &var : variableMap) basicBlocks[0]->instructions.emplace_back(InstructionPtr(new AllocaInstruction(var.second, basicBlocks[0])));
    for(auto &ins: tmp) basicBlocks[0]->instructions.emplace_back(ins);
}

void Function::processEndInstructions()
{
    for (auto &basicBlock : basicBlocks) {
        vector<shared_ptr<Instruction>> newInstructions;
        for (const auto& instruction : basicBlock->instructions) {
            newInstructions.push_back(instruction);
            if (instruction->type == Store) {
                auto storeInst = dynamic_cast<StoreInstruction*>(instruction.get());
                if (storeInst && storeInst->des->name == "retval") {
                    break;
                }
            }
        }
        basicBlock->instructions = newInstructions;
        assert(basicBlock->terminator);
        basicBlock->instructions.emplace_back(basicBlock->terminator);
    }
}

void Function::addBasicBlock(BasicBlockPtr basicblock)
{
    basicBlocks.emplace_back(basicblock);
}

void Function::clearBBToLoopMap() {
    bbToLoopMap.clear();
    for(auto basicBlock: basicBlocks) {
        basicBlock->loop = nullptr;
        basicBlock->loopDepth = 0;
    }
}

void Function::clearLoopList() {
    loopList.clear();
    outerLoops.clear();
}

BasicBlockPtr Function::getEntryBlock() {
    assert(basicBlocks.size() > 0);
    return basicBlocks[0];
}

void preorderVisit(LoopPtr loop, set<LoopPtr>& visited, vector<LoopPtr>& preorder, int& loopCount) {
    visited.insert(loop);
    for(auto innerLoop: loop->subLoops) {
        preorderVisit(innerLoop, visited, preorder, loopCount);
    }
    preorder[loopCount++] = loop;
}

vector<LoopPtr> Function::getLoopsInPostorder() {
    set<LoopPtr> visited;
    vector<LoopPtr> preorder(loopList.size());
    int loopCount = 0;
    for(auto loop: loopList) {
        if (visited.find(loop) == visited.end()) {
            preorderVisit(loop, visited, preorder, loopCount);
        }
    }
    assert(loopCount == loopList.size());
    return preorder;
}