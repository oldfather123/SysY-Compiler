#pragma once

#include "use_def_analysis.hpp"
#include "reverse_postorder.hpp"

namespace middleend {

using std::string;

class GlobalValueNumbering {
private:
    CFG *cfg_;
    UseDefAnalysis *uda_;
    ReversePostOrder *rpo_;
    std::unordered_map<string, Temp *> value_numbering;
    std::unordered_map<string, Temp *> mem_numbering;
    bool need_swap(ir::instruction::Binary *binary);
    string get_expr_str(ir::Instruction *inst);
    void set_srcs(ir::Instruction *inst, Temp *oldsrc, Temp *newsrc);
    void vn_changeuses(ir::Instruction *&inst);
    void run();

public:
    ~GlobalValueNumbering() {
        delete uda_;
        delete rpo_;
    }

    GlobalValueNumbering(CFG *cfg) : cfg_(cfg) {
        uda_ = new UseDefAnalysis(cfg_);
        rpo_ = new ReversePostOrder(cfg_);

        run();
    }
};


} // namespace middleend