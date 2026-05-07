#include "codegen/MachineBlock.hpp"
#include "BasicBlock.h"
#include "Function.h"

void MachineBlock::output(std::ostream &os) const {
    if (IRBlock->func_belong->entry == IRBlock) {
        os << IRBlock->func_belong->name << ":\n";
    } else {
        os << IRBlock->label << ":\n";
    }
    for (auto inst : inst_list) {
        inst->output(os);
        if (inst->no != -1 ) os << "\t\t\t\t# " << inst->no;
        os << "\n";
    }
    os << "\n";
}