#pragma once

#include "middleend/ir.hpp"

namespace middleend {

/// @brief mem2reg need to be done before this
class RemoveUselessInst {
private:
    ir::Function *func_;
    bool remove_useless_definition();
    bool remove_useless_loop();
public:
    RemoveUselessInst(ir::Function *func) : func_(func) {
        while(remove_useless_loop());
        while(remove_useless_definition());
    }
};

} // namespace middleend