#pragma once
#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

using std::string;

class ValueNumbering {
private:
    ir::Module *module_;
    void run();
    string get_expr_str(ir::Instruction *inst);
    bool need_swap(ir::instruction::Binary *binary);
    bool name_compress();
    bool lvn();
    void vn();
public:
    ValueNumbering(ir::Module *module) : module_(module) { run(); }
};

} // namespace middleend