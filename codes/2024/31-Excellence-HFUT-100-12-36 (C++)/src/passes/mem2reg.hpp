#ifndef PASSES_MEM2REG
#define PASSES_MEM2REG

#include "../ir/ir.hpp"
#include "../parser/SyntaxTree.hpp"
#include "pass.hpp"
#include <set>
#include <unordered_map>
namespace Passes {

class Mem2Reg : public Pass {
    std::list<std::pair<std::string, ptr<ir::ir_userfunc>>> func_lst;
    std::unordered_map<ptr<ir::ir_userfunc>, std::unordered_map<ptr<ir::ir_basicblock>, std::set<ptr<ir::ir_basicblock>>>> dom;
    std::unordered_map<ptr<ir::ir_userfunc>, std::unordered_map<ptr<ir::ir_basicblock>, ptr_list<ir::ir_basicblock>>> dominator_tree;
    std::unordered_map<ptr<ir::ir_userfunc>, std::unordered_map<ptr<ir::ir_basicblock>, ptr<ir::ir_basicblock>>> idom;
    std::unordered_map<ptr<ir::ir_userfunc>, ptr_list<ir::ir_basicblock>> postorders;
    std::unordered_map<ptr<ir::ir_userfunc>, std::unordered_map<ptr<ir::ir_basicblock>, std::set<ptr<ir::ir_basicblock>>>> df;
    std::unordered_map<ptr<ir::ir_userfunc>, std::unordered_map<ptr<ir::ir_memobj>, ptr_list<ir::ir_basicblock>>> defs;
    void remove_empty_block(ptr<ir::ir_userfunc> fun);
    void analyse_cfg_flow_and_defs(ptr<ir::ir_userfunc> fun);
    void calc_postorder(ptr<ir::ir_basicblock> node, ptr<ir::ir_userfunc> fun, ptr_list<ir::ir_basicblock> &rp, std::unordered_map<ptr<ir::ir_basicblock>, bool> &visited);
    void analyse_dom_relation(ptr<ir::ir_userfunc> fun);
    void analyse_df(ptr<ir::ir_userfunc> fun);
    std::unordered_map<ptr<ir::ir_reg>, ptr<ir::ir_memobj>> insert_phi_ins(ptr<ir::ir_userfunc> fun);
    void rename_variables(ptr<ir::ir_userfunc> fun, std::unordered_map<ptr<ir::ir_reg>, ptr<ir::ir_memobj>> phi_r_m);
    void rename(ptr<ir::ir_userfunc> fun, ptr<ir::ir_basicblock> block, std::unordered_map<ptr<ir::ir_memobj>, ptr_list<ir::ir_value>> &stack, std::unordered_map<ptr<ir::ir_reg>, ptr<ir::ir_memobj>> phi_r_m, std::unordered_map<ptr<ir::ir_reg>, ptr<ir::ir_memobj>> var_mem, std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map, std::unordered_map<ptr<ir::ir_basicblock>, bool> &visited);
public:
    Mem2Reg(ptr<ir::ir_module> compunit) : Pass(compunit) {}
    virtual std::string get_name() override final {return "MEM2REG";}
    virtual void run() override final;
};

}

#endif