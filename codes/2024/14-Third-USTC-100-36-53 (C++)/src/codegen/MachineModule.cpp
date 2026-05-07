#include "MachineModule.hpp"
#include "Module.hpp"
#include "MachineFunction.hpp"
#include <iomanip>

void MachineModule::add_function(std::shared_ptr<MachineFunction> MF) {
    functions.push_back(MF);
}

std::vector<std::shared_ptr<MachineFunction>>
MachineModule::get_functions() const {
    return functions;
}

static std::string floatToString(double value, int precision = 15) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

void MachineModule::create_init_val(Constant *init, std::string &s,
                                    bool &is_first) const {
    if (dynamic_cast<ConstantInt *>(init) != nullptr) {
        if (not is_first)
            s += ", ";
        else {
            s += ".word ";
            is_first = false;
        }
        s += std::to_string(dynamic_cast<ConstantInt *>(init)->get_value());
    } else if (dynamic_cast<ConstantFP *>(init) != nullptr) {
        if (not is_first)
            s += ", ";
        else {
            s += ".float ";
            is_first = false;
        }
        s += floatToString(dynamic_cast<ConstantFP *>(init)->get_value());
    } else if (dynamic_cast<ConstantArray *>(init) != nullptr) {
        auto *array = dynamic_cast<ConstantArray *>(init);
        for (unsigned int i = 0; i < array->get_size_of_array(); i++) {
            create_init_val(array->get_element_value(i), s, is_first);
        }
    } else {
        ASSERT(false);
    }
}

std::string MachineModule::global_var_print() const {
    std::string ret = "";

    ret += "\t.attribute arch, \"rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zba1p0_zbb1p0\"\n";
    ret += "\t.text\n";
    ret += "\t.section .bss, \"aw\", @nobits\n";
    for (auto &global : get_IR_module()->get_global_variable()) {
        if (global.get_init() != nullptr &&
            dynamic_cast<ConstantZero *>(global.get_init()) == nullptr)
            continue;
        auto size = global.get_type()->get_pointer_element_type()->get_size();
        ret += "\t.globl " + global.get_name() + "\n";
        ret += "\t.type " + global.get_name() + ", @object\n";
        ret +=
            "\t.size " + global.get_name() + ", " + std::to_string(size) + "\n";
        ret += global.get_name() + ":\n";
        ret += "\t.space " + std::to_string(size) + "\n";
    }
    ret += "\t.data\n";
    for (auto &global : get_IR_module()->get_global_variable()) {
        if (global.get_init() == nullptr ||
            dynamic_cast<ConstantZero *>(global.get_init()) != nullptr)
            continue;
        Type *global_type = global.get_type()->get_pointer_element_type();
        if (global_type->is_array_type()) {
            std::string init_val = "";
            bool is_first = true;
            create_init_val(global.get_init(), init_val, is_first);
            ret += global.get_name() + ":\n\t" + init_val + "\n";
        } else {
            if (global_type->is_int32_type()) {
                ret += global.get_name() + ":\n\t.word " +
                       std::to_string(
                           dynamic_cast<ConstantInt *>(global.get_init())
                               ->get_value()) +
                       "\n";
            } else if (global_type->is_float_type()) {
                ret +=
                    global.get_name() + ":\n\t.float " +
                    floatToString(dynamic_cast<ConstantFP *>(global.get_init())
                                      ->get_value()) +
                    "\n";
            } else
                ASSERT(false);
        }
    }
    return ret;
}

std::string MachineModule::print() const {
    std::string ret;
    ret += global_var_print();
    ret += "\t.text\n";
    for (auto &f : functions) {
        ret += f->print();
    }
    return ret;
}
