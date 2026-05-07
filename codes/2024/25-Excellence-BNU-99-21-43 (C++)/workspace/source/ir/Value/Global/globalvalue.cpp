#include "globalvalue.h"

namespace IR
{
    Function::Function(BasicType *retType, std::string name)
        : GlobalValue(nullptr, name, Value::FunctionVal)
    {
        this->type = FunctionType::get(retType);
    }

    Function::Function(BasicType *retType, std::string name, std::vector<BasicType *> argtypes)
        : GlobalValue(nullptr, name, Value::FunctionVal)
    {
        this->type = FunctionType::get(retType, argtypes);
        for (unsigned int i = 0; i < argtypes.size(); i++)
        {
            BasicType *argtype = argtypes[i];
            funArgs.push_back(new Argument(argtype, i));
        }
    }

    std::vector<BasicBlock *> Function::getVectorBlocks()
    {
        std::vector<BasicBlock *> blocks;
        for (ListNode *i = funBlocks.begin(); i != funBlocks.end(); i = i->nextNode())
        {
            blocks.push_back(static_cast<BasicBlock *>(i));
        }
        return blocks;
    }

    void Function::waste()
    {
        for (ListNode *i = funBlocks.begin(); i != funBlocks.end(); i = i->nextNode())
        {
            BasicBlock *idx = static_cast<BasicBlock *>(i);
            idx->waste();
        }
    }

    void Function::emitIR(std::ostream &os)
    {
        os << "define " << type->getTypeName() << " " << getIRName() << " {\n";
        for (ListNode *i = funBlocks.begin(); i != funBlocks.end(); i = i->nextNode())
        {
            BasicBlock *BB = static_cast<BasicBlock *>(i);
            BB->emitIR(os);
        }
        os << "}\n";
    }

    void Function::addBlock(BasicBlock *block, bool entry)
    {
        // 更新尾节点的 susccsor, （不需要了）
        block->parent = this;
        funBlocks.pushBack(block);
        if (funBlocks.begin() == block)
            entryBlock = block;
        if (entry)
            entryBlock = block;
    }

    void Function::insertBlock(BasicBlock *block, BasicBlock *prev)
    {
        // 首先先更新前一个块的后继。（不需要了）
        block->parent = this;
        funBlocks.insertAfter(prev, block);

        // 更新 block 的后继。（不需要了）
    }

    void Function::removeBlock(BasicBlock *block)
    {
        block->waste();
    }

    void Function::emitUse(std::ostream &os)
    {
        os << getIRName() << " used by:" << std::endl;
        for (ListNode *i = useList.begin(); i != useList.end(); i = i->nextNode())
        {
            Use *use = static_cast<Use *>(i);
            os << '\t' << use->val->getIRName() << std::endl;
        }
        for (ListNode *i = funBlocks.begin(); i != funBlocks.end(); i = i->nextNode())
        {
            BasicBlock *idx = static_cast<BasicBlock *>(i);
            idx->emitUse(os);
        }
    }

    std::map<BasicBlock *, std::set<BasicBlock *>> Function::cfg()
    {
        std::map<BasicBlock *, std::set<BasicBlock *>> cfg;
        if (isBuiltinFunction())
            return cfg;
        for (ListNode *i = funBlocks.begin(); i != funBlocks.end(); i = i->nextNode())
        {
            BasicBlock *BB = static_cast<BasicBlock *>(i);
            auto succs = BB->getSuccBlock();
            cfg[BB] = std::set<BasicBlock *>();
            for (auto &succ : succs)
                cfg[BB].insert(succ);
        }

        return cfg;
    }

    void Function::mergeBlock(BasicBlock *prev, BasicBlock *nxt)
    {
        assert(BasicBlock::checkNeignbour(prev, nxt));

        nxt->replaceAllUsageTo(prev);
        // 更新 prev 的指令
        for (ListNode *i = nxt->getInstruction().begin(); i != nxt->getInstruction().end();)
        {
            Instruction *instr = static_cast<Instruction *>(i);
            i = i->nextNode();
            nxt->getInstruction().remove(instr);
            prev->InsertInstructionBack(instr);
        }
        assert(nxt->instructions.empty());
        nxt->waste();
    }

    BasicBlock *Function::splitBlock(BasicBlock *block, Instruction *pos)
    {
        for (ListNode *i = block->getInstruction().begin(); i != block->getInstruction().end(); i = i->nextNode())
        {
            Instruction *idx = static_cast<Instruction *>(i);
            if (idx == pos)
            {
                BasicBlock *newBlock = new BasicBlock("split");
                insertBlock(newBlock, block);
                for (ListNode *i = pos->nextNode(); i != block->getInstruction().end();)
                {
                    Instruction *instr = static_cast<Instruction *>(i);
                    i = i->nextNode();
                    block->getInstruction().remove(instr);
                    newBlock->InsertInstructionBack(instr);
                }
                return newBlock;
            }
        }
        Error::Error("Function::splitBlock", "pos not found");
        return nullptr;
    }

    ExternalFunction::ExternalFunction(BasicType *retType, std::string name, std::vector<BasicType *> argtypes)
        : Function(retType, name, argtypes)
    {
        type = FunctionType::get(retType, argtypes);
        isBuiltin = true;
    }

    void ExternalFunction::emitIR(std::ostream &os)
    {
        os << "declare " << type->getTypeName() << " " << getIRName() << std::endl;
    }

    GlobalVariable::GlobalVariable(BasicType *type, std::string name, bool isConstant)
        : GlobalValue(type, name, Value::GlobalVariableVal)
    {
        this->isConstant = isConstant;
        operands.push_back(Constant::getZeroValueForType(type->getBaseType()));
    }

    GlobalVariable::GlobalVariable(BasicType *type, std::string name, Constant *initializer, bool isConstant) : GlobalValue(type, name, Value::GlobalVariableVal)
    {
        isInit = true;
        this->initializer = initializer;
        this->isConstant = isConstant;
        operands.push_back(initializer);
    }

    void GlobalVariable::emitIR(std::ostream &os)
    {
        os << "define global " << type->getBaseType()->getTypeName()
           << " " << getIRName() << std::endl;
    }
}