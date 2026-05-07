#ifndef COLORING_ALLOCATOR
#define COLORING_ALLOCATOR

#include "../ir/ir.hpp"
#include "arch.hpp"
#include "register_allocator.hpp"
#include "../parser/SyntaxTree.hpp"
#include <set>
#include <unordered_map>
#include <unordered_set>
namespace LoongArch {

class ColoringAllocator : public RegisterAllocator {
private:
    ptr<ir::ir_userfunc> fun;
    std::unordered_map<ptr<ir::ir_instr>, std::unordered_set<ptr<ir::ir_reg>>> live_in;
    std::unordered_map<ptr<ir::ir_instr>, std::unordered_set<ptr<ir::ir_reg>>> live_out;
    std::unordered_map<std::shared_ptr<ir::ir_reg>, Reg> mapping_to_reg;
    std::set<ptr<ir::ir_reg>> spill_set;
    std::unordered_map<ptr<ir::ir_reg>, std::unordered_set<ptr<ir::ir_reg>>> ig;
    vector<Reg> i_color = {Reg{28}, Reg{29}, Reg{30}, Reg{11}, Reg{12}, Reg{13}, Reg{14}, Reg{15}, Reg{16}, Reg{17}};
    // vector<Reg> i_color = {Reg{12}, Reg{13}, Reg{14}};
    vector<Reg> f_color = {
        Reg(0, FLOAT),
        Reg(1, FLOAT),
        Reg(2, FLOAT),
        Reg(3, FLOAT),
        Reg(4, FLOAT),
        // Reg(5, FLOAT),
        // Reg(6, FLOAT),
        // Reg(7, FLOAT),
        Reg(28, FLOAT),
        Reg(29, FLOAT),
        Reg(30, FLOAT),
        // Reg(31, FLOAT),
        // Reg(20, FLOAT),
        // Reg(21, FLOAT),
        // Reg(22, FLOAT),
        // Reg(23, FLOAT)
    };
    // vector<Reg> fb_color {
    //     Reg(0, FBOOL),
    //     Reg(1, FBOOL),
    //     Reg(2, FBOOL),
    //     Reg(3, FBOOL),
    //     Reg(4, FBOOL),
    //     Reg(5, FBOOL),
    //     Reg(6, FBOOL),
    //     Reg(7, FBOOL),
    // };
    vector<Reg> using_color;
    std::function<bool(const ptr<ir::ir_reg>)> is_target;
    Rtype dealing;
    // std::vector<std::shared_ptr<ir::ir_reg>> mapping_to_spill;
    // std::vector<std::shared_ptr<ir::ir_memobj>> arrobj;
    void analyse_cfg_flow();
    void clear();
    void analyse_live();
    void build_ig();
    // bool kempe_opt();
    bool kempe();
    ptr<ir::ir_reg> remove(std::unordered_map<ptr<ir::ir_reg>, int> &g, ptr<ir::ir_reg> del_item);
    bool assign_color(ptr<ir::ir_reg> node);
    bool rewrite();
    
public:
    ColoringAllocator(ptr<ir::ir_userfunc> fun, int base_reg, ptr_list<ir::global_def> global_var);
    alloc_res run(Rtype target) override final;
};

};

#endif