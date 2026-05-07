#pragma once

#include "value.h"
#include "labeltype.h"
#include "instruction.h"
#include "listnode.h"
#include "list.h"
#include <algorithm>
#include <vector>
#include <set>

namespace IR
{
    struct Function;
    struct BasicBlock : public Value, public ListNode
    {
        bool hasNoLoopUnrollTag = false;

        Function *parent = nullptr;

        virtual ~BasicBlock() = default;

        List<Instruction *> instructions;

        static int totBlocks;

        BasicBlock()
            : Value(LabelType::getInstance(), Value::BasicBlockVal), ListNode(1)
        {
            number = totBlocks++;
        };

        BasicBlock(std::string name)
            : Value(LabelType::getInstance(), name, Value::BasicBlockVal), ListNode(1)
        {
            number = totBlocks++;
        };

        void waste() override;

        std::string getIRName() const override
        {
            return "Label_" + std::to_string(number);
        }

        void InsertInstructionBack(Instruction *instr);

        void InsertInstructionFront(Instruction *instr);

        // 在who指令之前插入instr指令
        void InsertInstruction(Instruction *instr, Instruction *who);

        void emitIR(std::ostream &os) override;

        void emitUse(std::ostream &os) override;

        std::set<BasicBlock *> getPredBlock();

        std::set<BasicBlock *> getSuccBlock();

        List<Instruction *> &getInstruction()
        {
            return instructions;
        }

        std::vector<Instruction *> getVectorInstructions();

        static BasicBlock *get()
        {
            return new BasicBlock();
        }

        static BasicBlock *get(std::string name);

        static bool checkNeignbour(BasicBlock *a, BasicBlock *b);

        bool isReturnBlock();

        bool isCondBrBlock();

        bool isDirectBrBlock();

        void emitSuss(std::ostream &os);

        void setNoLoopUnroll() { hasNoLoopUnrollTag = true; }

        void setLoopUnroll() { hasNoLoopUnrollTag = false; }

        void removeEntryFromPhi(BasicBlock *toRemoveBlock);
    };

}