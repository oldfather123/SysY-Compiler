#ifndef GEECEECEE_RV_MODULE_HPP
#define GEECEECEE_RV_MODULE_HPP

#pragma once
#include <string>
#include <vector>

#include "rv_const_float.hpp"
#include "rv_function.hpp"
#include "rv_global_var.hpp"

namespace backend {
namespace riscv {
class RVReg;
}
}  // namespace backend

template <>
struct std::hash<std::pair<backend::riscv::RVReg *, backend::riscv::RVReg *>> {
    std::size_t operator()(const std::pair<backend::riscv::RVReg *, backend::riscv::RVReg *> &p) const noexcept {
        return std::hash<backend::riscv::RVReg *>()(p.first) ^ (std::hash<backend::riscv::RVReg *>()(p.second) << 1);
    }
};

namespace backend::riscv {

class RVModule {
public:
    RVModule();
    ~RVModule();
    RVModule(const RVModule &) = delete;
    RVModule &operator=(const RVModule &) = delete;
    RVModule(RVModule &&) = delete;
    RVModule &operator=(RVModule &&) = delete;

    RVFunction *new_function(const std::string &name);
    std::string emit() const;  // 生成完整 .S 文本

    void add_data_variable(RVGlobalVar *var);
    void add_bss_variable(RVGlobalVar *var);
    RVConstFloat *get_or_create_float_const(float val) {
        if (float_constants.find(val) == float_constants.end()) {
            auto name = "float" + std::to_string(float_constants.size());
            float_constants[val] = RVConstFloat::create(val, name);
        }
        return float_constants[val];
    }

    RVFunction *get_function(const std::string &name) const;
    const std::map<std::string, RVFunction *> &get_functions() const;

    // 检查load和store指令的偏移量是否正确
    void check_memory_offsets() const;

    // 模拟随机情况：在main函数的第一个基本块开头插入li指令给寄存器赋随机值
    void simulate_random_context();

private:
    std::vector<RVGlobalVar *> data_variables = std::vector<RVGlobalVar *>();
    std::vector<RVGlobalVar *> bss_variables = std::vector<RVGlobalVar *>();
    std::map<std::string, RVFunction *> functions = std::map<std::string, RVFunction *>();
    std::map<float, RVConstFloat *> float_constants = std::map<float, RVConstFloat *>();
};

}  // namespace backend::riscv

#endif
