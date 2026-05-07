#include "MachineFunction.hpp"
#include "Operand.hpp"
#include <string>

std::string MachineFunction::get_name() const { return function->get_name(); }

std::string MachineFunction::print() const {
    std::string ret;
    ret += "\t.globl " + function->get_name() + "\n";
    ret += "\t.type " + function->get_name() + ", @function\n";
    for (auto &bb : basic_blocks) {
        ret += bb->print();
    }
    return ret;
}
void MachineFunction::calc_params_schedule() {
    auto &args = function->get_args();
    auto align = [] (int offset, int align) {
        return (offset + align - 1) & ~(align - 1);
    };
    params_size = 0;
    unsigned garg_cnt = 0;
    unsigned farg_cnt = 0;
    for (auto &arg : args) {
        auto ty = arg.get_type();
        auto size = ty->get_size();
        if(ty->is_float_type()) {
            if(farg_cnt < 8) {
                params_schedule_map[&arg] = {false, 0, PhysicalRegister::fa(farg_cnt)};
                farg_cnt++;
            }
            else {
                params_schedule_map[&arg] = {true, align(params_size, size), nullptr};
                params_size = align(params_size + size, size);
            }
        }
        else if(ty->is_integer_type() || ty->is_pointer_type()) {
            if(garg_cnt < 8) {
                params_schedule_map[&arg] = {false, 0, PhysicalRegister::a(garg_cnt)};
                garg_cnt++;
            }
            else {
                params_schedule_map[&arg] = {true, align(params_size, size), nullptr};
                params_size = align(params_size + size, size);
            }
        }
        else ASSERT(false);
    }
    params_size = align(params_size, 16);
}
