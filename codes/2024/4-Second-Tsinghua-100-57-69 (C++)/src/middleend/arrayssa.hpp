#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

class ArraySSA {
private:
    ir::Module *module_;

    std::unordered_map<ir::Function *, std::unordered_set<std::string>> func_global_use;
    std::unordered_map<ir::Function *, std::unordered_set<Temp *>> func_param_use;

    std::unordered_map<Temp *, ir::BasicBlock *> alloc_bb; // ptr声明出现的bb
    std::unordered_map<ir::BasicBlock *, std::unordered_map<Temp *, Temp *>> alloc_map;
    std::unordered_map<Temp *, std::unordered_set<ir::BasicBlock *>> def_bbs; // ptr定义出现的bb
    std::unordered_map<Temp *, Temp *> reg2base; // ptr到数组初始指针
    std::unordered_map<std::string, Temp *> name2base; // 全局变量名字到ptr
    std::unordered_map<ir::instruction::Phi *, Temp *> phi2base; // phi指令到ptr

    void find_func_use(ir::Function *func);
    void build_analysis(DominatorTree_ *dom_tree, int &next_temp_id_);
    void insert_phi(DominatorTree_ *dom_tree, int &new_temp);
    void rename(DominatorTree_ *dom_tree, int &next_temp_id_);
    void remove_useless_phi(DominatorTree_ *dom_tree);
public:
    ArraySSA(ir::Module *module);
    void array2reg();
    void reg2array();
};

} // namespace middleend