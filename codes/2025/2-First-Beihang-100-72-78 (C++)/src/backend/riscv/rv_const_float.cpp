#include "rv_const_float.hpp"

#include <cstring>
#include <utility>

namespace backend::riscv {

RVConstFloat *RVConstFloat::create(float val, std::string name) { return new RVConstFloat(val, std::move(name)); }

RVConstFloat::RVConstFloat(float val, std::string name) : val(val), name(std::move(name)) {}

std::string RVConstFloat::getName() const { return name; }

std::string RVConstFloat::printAsm() const {
    std::string ret;
    ret += getName() + ":\n";
    int i_val;
    std::memcpy(&i_val, &val, sizeof(int));
    ret += "\t.word\t" + std::to_string(i_val) + "\n";
    return ret;
}

}  // namespace backend::riscv
