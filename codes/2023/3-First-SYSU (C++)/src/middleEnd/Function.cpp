#include "Function.h"

Function::Function(TypePtr returnType, string name, vector<ValuePtr> formArguments, shared_ptr<Block> block) : retVal{ValuePtr(new Reg(returnType, "retval"))}, name{name}, formArguments{formArguments}, block{block}, isLib{false}, isReenterable{true}
{
    returnBasicBlock = BasicBlockPtr(new BasicBlock(LabelPtr(new Label("return"))));
};

void Function::solveReturnBasicBlock()
{
    if (retVal->type->isVoid())
    {
        returnBasicBlock->pushEndInstruction(InstructionPtr(new ReturnInstruction(retVal, returnBasicBlock)));
        block->pushBasicBlock(returnBasicBlock);
    }
    else
    {
        auto ins = shared_ptr<LoadInstruction>(new LoadInstruction(retVal, block->getReg(retVal->type), returnBasicBlock));
        returnBasicBlock->pushInstruction(ins);
        returnBasicBlock->pushEndInstruction(InstructionPtr(new ReturnInstruction(ins->to, returnBasicBlock)));
        block->pushBasicBlock(returnBasicBlock);
    }
}

void Function::print()
{
    if (isLib)
    {
        cout << "declare " << getTypeStr() << "(";
        for (int i = 0; i < formArguments.size(); i++)
        {
            if (i)
                cout << ", ";
            cout << formArguments[i]->type->getStr();
        }
        cout << ")" << endl;
    }
    else
    {
        // std::cerr<<name<<": "<<isReenterable<<endl;
        cout << "define " << retVal->type->getStr() << " @" << this->name << "(";
        for (int i = 0; i < formArguments.size(); i++)
        {
            if (i)
                cout << ", ";
            cout << formArguments[i]->getTypeStr();
        }
        cout << ") {" << endl;
        block->print();
        cout << "}" << endl
             << endl;
    }
}
