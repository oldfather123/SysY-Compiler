#include "Register.hpp"
#include <stdexcept>
#include <string>

std::string Reg::print() const {
    static const std::string reg_names[] = {
        "zero", "ra", "sp", "gp", "tp",  // 0 - 4
        "t0", "t1", "t2",                // 5 - 7
        "s0", "s1",                      // 8 - 9
        "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",  // 10 - 17
        "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",  // 18 - 27
        "t3", "t4", "t5", "t6"           // 28 - 31
    };

    if (id >= 0 && id <= 31) {
        return reg_names[id];
    }

    throw std::runtime_error("Invalid register id: " + std::to_string(id));
}

std::string FReg::print() const {
    if (id >= 0 && id <= 7) {
        return "ft" + std::to_string(id); // ft0 - ft7
    }
    if (id == 8 || id == 9) {
        return "fs" + std::to_string(id - 8); // fs0 - fs1
    }
    if (id >= 10 && id <= 17) {
        return "fa" + std::to_string(id - 10); // fa0 - fa7
    }
    if (id >= 18 && id <= 27) {
        return "fs" + std::to_string(id - 16); // fs2 - fs11
    }
    if (id >= 28 && id <= 31) {
        return "ft" + std::to_string(id - 20); // ft8 - ft11
    }

    throw std::runtime_error("Invalid floating-point register id: " + std::to_string(id));
}