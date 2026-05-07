#ifndef GEECEECEE_RV_GLOBAL_VAR_HPP
#define GEECEECEE_RV_GLOBAL_VAR_HPP

#pragma once
#include <string>

#include "../../ir/value.hpp"

namespace backend::riscv {

class RVGlobalVar {
public:
    std::string to_string() const;
    bool is_zero_initialized() const;

    // 构造函数和析构函数公开
    explicit RVGlobalVar(ir::GlobalVariable *gv, bool is_zero_init);
    ~RVGlobalVar();

private:
    ir::GlobalVariable *gv = nullptr;
    bool is_zero_init = false;
};

}  // namespace backend::riscv

#endif  // GEECEECEE_RV_GLOBAL_VAR_HPP
