#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/reverse_postorder.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/loop_analysis.hpp"
#include "middleend/use_def_analysis.hpp"
#include "middleend/copy_propagation.hpp"

namespace middleend {

class ElementPtrDeconstruct {
private:
    CFG *cfg_;
    ir::Function *func;
    void run();
   
public:
    ~ElementPtrDeconstruct() {
        delete cfg_;
    }

    ElementPtrDeconstruct(ir::Function *func_) : func(func_) {
        cfg_ = new CFG(func);
        run(); 
    }
};

}