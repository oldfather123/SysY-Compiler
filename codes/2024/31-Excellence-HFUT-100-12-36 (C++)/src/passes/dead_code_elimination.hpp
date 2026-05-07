#ifndef PASSES_DEAD_CODE_ELIMINATION
#define PASSES_DEAD_CODE_ELIMINATION

#include "../ir/ir.hpp"
#include "pass.hpp"
#include <deque>
#include <unordered_map>

namespace Passes {

class DeadCodeElimination : public Passes::Pass {
private:
    std::deque<ptr<ir::ir_instr>> work_lst;
    std::unordered_map<ptr<ir::ir_instr>, bool> marked;
public:
    DeadCodeElimination(ptr<ir::ir_module> compunit) : Pass(compunit) {}
    virtual std::string get_name() override final {return "DCE";}
    virtual void run() override final;
    void mark_by_fun(ptr<ir::ir_userfunc> fun);
    bool check_critical(ptr<ir::ir_instr> tar);
    void mark_by_ins(ptr<ir::ir_instr> ins);
    bool sweep(ptr<ir::ir_userfunc> fun);
};

};

#endif