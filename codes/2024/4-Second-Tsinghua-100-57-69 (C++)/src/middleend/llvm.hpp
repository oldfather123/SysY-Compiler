#pragma once

#include "middleend/ir.hpp"

namespace middleend {

class LLVM {
private:
    ir::Module *module_;
    int llvm_temp_cnt = 0;

public:
    LLVM(ir::Module *module) : module_(module) {}
    void print_llvm(std::ostream &out);
};

} // namespace middleend
