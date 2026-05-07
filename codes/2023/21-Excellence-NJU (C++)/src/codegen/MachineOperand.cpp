#include "codegen/MachineOperand.hpp"
#include "codegen/utils/NotImplemented.hpp"
#include "magic_enum.hpp"

#include <cctype>
#include <ostream>
#include <string>
#include <cassert>
#include <string_view>

MachineOperand::MachineOperand(MachineOperandType type_p, int val_p, bool fpu_p)
    : type(type_p), fpu(fpu_p)
{
    if (type_p == IMM) {
        val = val_p;
    } else {
        reg_no = val_p;
    }
}

MachineOperand::MachineOperand(MachineOperandType type_p, std::string val_p)
    : type(type_p)
{
    if (type_p == LABEL) {
        label = val_p;
    } else {
        vreg_name = val_p;
    }
}

void MachineOperand::print_reg(std::ostream &os) const {
    switch (type) {
        case REG:
            {
                if(!this->fpu){
                    std::string reg_name(magic_enum::enum_name(static_cast<IREGISTER>(reg_no)));
                    std::transform(reg_name.begin(), reg_name.end(), reg_name.begin(), ::tolower);
                    os << reg_name;
                }
                else{
                    std::string reg_name(magic_enum::enum_name(static_cast<Fregister>(reg_no)));
                    std::transform(reg_name.begin(), reg_name.end(), reg_name.begin(), ::tolower);
                    os << reg_name;
                }
            }
            break;
        case VREG: case FVREG:
            os << vreg_name;
            break;
        default:
            assert(false);
    }
}

void MachineOperand::output(std::ostream &os) const {
    switch (this->type) {
        case IMM:
            os << val;
            break;
            case REG: case VREG: case FVREG:
            print_reg(os);
            break;
        case LABEL:
//            throw NotImplemented("print operand of label");
//            TODO: to check
            os << label;
            break;
        default:
            assert(false);
    }
}