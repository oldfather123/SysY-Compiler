#include "foldConstantBranches.h"

void foldConstantBranches(FunctionPtr func) {
    for (auto& basicBlock : func->basicBlocks) {
        auto terminator = basicBlock->terminator;
        if (terminator->type != InsID::Br) {
            continue;
        }

        auto brInst = dynamic_cast<BrInstruction*>(terminator.get());
        if (!brInst->exp) {
            continue;
        }

        auto constExpr = dynamic_cast<Const*>(brInst->exp.get());
        if (!constExpr) {
            continue;
        }
        bool condition = constExpr->intVal != 0;
        auto& target = condition ? func->LabelToBBMap[brInst->label1] : func->LabelToBBMap[brInst->label2];
        auto& nottarget = (!condition) ? func->LabelToBBMap[brInst->label1] : func->LabelToBBMap[brInst->label2];
        if (func->LabelToBBMap[brInst->label1] == func->LabelToBBMap[brInst->label2]) {
            continue;
        }

        basicBlock->instructions.pop_back();
        auto newBranch = std::make_shared<BrInstruction>(target->label, basicBlock);
        basicBlock->instructions.push_back(newBranch);
        basicBlock->succBasicBlocks.erase(nottarget);
        nottarget->predBasicBlocks.erase(basicBlock);
    }
}