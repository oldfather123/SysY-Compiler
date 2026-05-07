#include "Block.h"



Block::Block() : regNum{0}
{
    basicBlocks.emplace_back(BasicBlockPtr(new BasicBlock()));
}

void Block::getLabelBBMap(){
    for(int i=0;i<basicBlocks.size();i++){
        LabelBBMap[basicBlocks[i]->label] = basicBlocks[i];
    }
}

void Block::pushVariable(VariablePtr variable)
{
    variables[variable->name]= variable;
}

VariablePtr Block::findVariable(string name)
{
    if (variables.find(name)!=variables.end()){
        return variables[name];
    }

    return nullptr;
}

void Block::allocLocalVariable()
{
    auto tmp = basicBlocks[0]->instructions;
    basicBlocks[0]->instructions.clear();
    for (auto &var : variables) basicBlocks[0]->instructions.emplace_back(InstructionPtr(new AllocaInstruction(var.second, basicBlocks[0])));
    for(auto &ins: tmp) basicBlocks[0]->instructions.emplace_back(ins);
}

void Block::solveEndInstruction()
{
    for (auto &basicBlock : basicBlocks)
    {
        vector<shared_ptr<Instruction>> newIns;
        int i  =0;
        for(;i<basicBlock->instructions.size();i++){
            // cerr<<i<<"  "<<basicBlock->instructions[i]->type<<endl;
            newIns.push_back(basicBlock->instructions[i]);
            if(basicBlock->instructions[i]->type==Store&&dynamic_cast<StoreInstruction*>(basicBlock->instructions[i].get())->des->name=="retval"){
                break;
            }
            
        }
        basicBlock->instructions = newIns;

        assert(basicBlock->endInstruction);
        basicBlock->instructions.emplace_back(basicBlock->endInstruction);
    }
}

void Block::pushBasicBlock(BasicBlockPtr basicblock)
{
    basicBlocks.emplace_back(basicblock);
}

void Block::print()
{
    for (int i = 0; i < basicBlocks.size(); i++)
    {
        basicBlocks[i]->print();
        if (i != basicBlocks.size() - 1)
            cout << endl;
    }
}

void BasicBlock::pushInstruction(InstructionPtr instruction)
{
    if(!endInstruction) instructions.emplace_back(instruction);
}

void BasicBlock::pushEndInstruction(InstructionPtr instruction)
{
    if(!endInstruction) endInstruction = instruction;
}

void BasicBlock::print()
{
    label->print();

    int cnt = 0,n=predBasicBlocks.size();
    for(auto &pred : predBasicBlocks){
        if(cnt==0){
            cout<<"\t\t\t; preds = ";
        }
        cout<<"%"+pred.first->label->name;
        if(cnt<n-1){
            cout<<", ";
        }
        cnt++;
    }
    cout<<endl;
    for (auto &instruction : instructions)
    {
        instruction->print();
    }
}