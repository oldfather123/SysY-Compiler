#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "backend/riscv/rv_global_var.hpp"

namespace backend::opt::gp_pooling {

class GPpool {
public:
    enum class GlobType { FLOAT };

    GPpool();
    void add(backend::riscv::RVGlobalVar *global_var);
    void init();
    int query_offset(backend::riscv::RVGlobalVar *global_var);
    std::string to_string() const;

private:
    std::unordered_map<backend::riscv::RVGlobalVar *, int> gv_to_offset;
    std::vector<backend::riscv::RVGlobalVar *> glos;
    int size;
    std::string name;
    GlobType type;
};

}  // namespace backend::opt::gp_pooling
