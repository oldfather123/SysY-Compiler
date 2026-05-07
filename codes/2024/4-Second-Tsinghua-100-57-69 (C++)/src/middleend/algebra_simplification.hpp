#pragma once
#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"

namespace middleend {

using std::string;

class AlgebraSimplification {
private:
    ir::Module *module_;
    void run();
public:
    AlgebraSimplification(ir::Module *module) : module_(module) { run(); }
};

} // namespace middleend