#pragma once

#include "Looplook.hpp"

using LoopSequence = std::list<std::vector<BBset_t *>>;
using LoopControl = std::unordered_map<BBset_t *, std::string>;

using LoopEnd = std::unordered_map<BBset_t *, std::tuple<Value *, int>>;

// class LoopCase {
//     public:
//         LoopTree loop_tree;
//         LoopInfo loop_info;
//         LoopArrayAccessInfo loop_array_access;
//         LoopSequence loopseq;
//         std::string compute_pattern="";
//         std::string control_pattern="";

//         LoopCase(const LoopTree& lt, const LoopInfo& li,const LoopArrayAccessInfo& la)
//             : loop_tree(lt), loop_info(li),loop_array_access(la){}

//         void analyze_compute_pattern(BBset_t* loop, const LoopArrayAccessInfo& access_info);

//         void analyze_control_pattern(BBset_t* loop, const std::vector<Value*>& current_loop_vars);

//         std::string get_compute_pattern(){return compute_pattern;}
//         std::string get_control_pattern(){return control_pattern;}
//     };

std::string analyze_control_pattern(BBset_t *loop, LoopInfo loop_info, const std::vector<Value *> &current_loop_vars);

std::string analyze_compute_pattern(BBset_t *loop, const LoopArrayAccessInfo &access_info);

void analyze_control_in_loops(const LoopTree &loop_tree, const LoopInfo &loop_info,
                              const LoopArrayAccessInfo &access_info, LoopControl &loop_con);

void match_loop_var_index(
    BBset_t *loop,
    int depth,
    const LoopTree &loop_tree,
    const LoopInfo &loop_info,
    const LoopArrayAccessInfo &access_info,
    LoopControl &loop_con,
    std::set<int> &loop_var_usage_flags);

void print_control(const LoopTree &loop_tree, LoopControl &loop_con, LoopEnd &loopend);
void print_control_loop_tree(const LoopTree &loop_tree,
                             LoopControl &loop_con,
                             LoopEnd &loopend,
                             BBset_t *loop, int level);

bool isValueUsedBefore(Instruction *inst, Value *target, std::unordered_set<Value *> &visited);

Value *resolve_loop_end(Value *end, std::vector<BBset_t *> loopset, LoopInfo &loopinfo, BBset_t *self_loop);

void analyzeLoopDependencyUsedByChild(LoopInfo &loop_info, const LoopTree &loop_tree, LoopArrayAccessInfo &loop_arr,
                                      LoopControl &loop_control, LoopEnd &loopend);

Value *resolveLoopBoundary(Value *current,
                           bool isUpperBound,
                           BBset_t *loop,
                           LoopInfo &loopinfo,
                           std::unordered_map<Value *, BBset_t *> &value_bb,
                           int &offset);

bool check_parent_loop_var_used_in_children(
    BBset_t *parent_loop,
    const LoopInfo &loop_info,
    const LoopTree &loop_tree,
    const LoopArrayAccessInfo &loop_arr);

void valueusedbychild(
    LoopInfo &loop_info,
    const LoopTree &loop_tree,
    LoopArrayAccessInfo &loop_arr,
    LoopControl &loop_control);