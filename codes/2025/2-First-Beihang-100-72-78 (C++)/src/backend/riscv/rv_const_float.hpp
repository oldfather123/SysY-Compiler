#ifndef GEECEECEE_RV_CONST_FLOAT_HPP
#define GEECEECEE_RV_CONST_FLOAT_HPP

#pragma once
#include <string>

namespace backend::riscv {

class RVConstFloat {
public:
    static RVConstFloat *create(float val, std::string name);

    std::string getName() const;
    std::string printAsm() const;

private:
    RVConstFloat(float val, std::string name);

    float val;
    std::string name;
};

}  // namespace backend::riscv

#endif  // GEECEECEE_RV_CONST_FLOAT_HPP
