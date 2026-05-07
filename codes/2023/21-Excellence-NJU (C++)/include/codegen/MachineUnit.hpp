#ifndef __MACHINE_UNIT_H_
#define __MACHINE_UNIT_H_

#include "codegen/MachineFunction.hpp"

#include <memory>
#include <ostream>
#include <sstream>
#include <vector>


class MachineUnit {
public:
    std::vector<MachineFunction *> func_list;
    std::stringstream global_vars;
    std::set<std::string> global_symbol;
    std::map<std::string, Function *> IR_func_table;
    std::stringstream float_literals;

    ~MachineUnit() {
        for (auto &func : func_list) {
            delete func;
            func = nullptr;
        }
    }

    void insert_func(MachineFunction *func) {
        func_list.push_back(func);
    }

    void output(std::ostream &os) {
        os << ".globl\tmain\n";
        for (auto func : func_list) {
            func->output(os);
        }
        os << float_literals.str();

        os << global_vars.str();
    }
};

#endif