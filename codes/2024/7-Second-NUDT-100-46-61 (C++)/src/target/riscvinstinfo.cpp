#define NDEBUG
#include "../../include/ir/ir.hpp"

#include "../../include/mir/mir.hpp"
#include "../../include/mir/instinfo.hpp"

#include "../../include/target/InstInfoDecl.hpp"
#include "../../include/target/riscv.hpp"

#include <iostream>

namespace RISCV {

static std::ostream& operator<<(std::ostream& out, const mir::MIROperand& mop) {
    if (mop.is_reg()) {
        // if ()
    } else if (mop.is_imm()) {

    } else if (mop.is_prob()) {

    } else if (mop.is_reloc()) {

    } else {
        std::cerr << "unknown operand type" << std::endl;
    }


}

}  // namespace RISCV
