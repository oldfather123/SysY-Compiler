#ifndef PASSES_LOOP_ANALYSE
#define PASSES_LOOP_ANALYSE

#include "../ir/ir.hpp"
#include "../parser/SyntaxTree.hpp"
#include "../passes/pass.hpp"
#include <unordered_set>
namespace Passes {

class LoopInvariantCodeMotion : public Pass {
    ptr_list<ir::ir_instr> analyse_invariant(ptr<ir::while_loop>);
    void motion(ptr<ir::while_loop>, ptr_list<ir::ir_instr>);
    std::unordered_set<ptr<ir::ir_basicblock>> get_body(ptr<ir::while_loop>);
public:
    LoopInvariantCodeMotion(ptr<ir::ir_module> compunit) : Pass(compunit) {}
    virtual std::string get_name() override final {return "LICM";}
    virtual void run() override final;
};

}

#endif