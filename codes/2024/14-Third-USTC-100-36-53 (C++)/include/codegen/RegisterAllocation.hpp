#pragma once

#include "MachinePass.hpp"
#include "Operand.hpp"

#include <cstddef>
#include <memory>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>

class MachineInstr;
class Register;
class PhysicalRegister;
class VirtualRegister;
class MachineFunction;
class LivenessAnalysis;
using MoveSet = std::unordered_set<std::shared_ptr<MachineInstr>>;
using RegisterSet = std::unordered_set<std::shared_ptr<Register>>;

class RegisterAllocation : public MachinePass {
  public:
    RegisterAllocation(std::shared_ptr<MachineModule> MM) : MachinePass(MM) {
        general_K = PhysicalRegister::allocatable_general().size();
        float_K = PhysicalRegister::allocatable_float().size();
    }
    ~RegisterAllocation() override = default;
    void run_on_func(std::shared_ptr<MachineFunction> func);
    void run() override;

  private:
    void build_graph();
    void add_edge(std::shared_ptr<Register> u, std::shared_ptr<Register> v);
    void make_worklist();
    void simplify();
    void coalesce();
    void freeze();
    void select_spill();
    void assign_colors();
    void rewrite_program();

    size_t type_k(Register::RegisterType type);
    RegisterSet adjacent(std::shared_ptr<Register> reg);
    MoveSet node_moves(std::shared_ptr<Register> reg);
    bool move_related(std::shared_ptr<Register> reg);
    void decrement_degree(std::shared_ptr<Register> reg);
    void enable_moves(RegisterSet &nodes);
    std::shared_ptr<Register> get_alias(std::shared_ptr<Register> reg);
    void add_worklist(std::shared_ptr<Register> reg);
    bool check_colored_adj(std::shared_ptr<Register> u, std::shared_ptr<Register> v);
    void combine(std::shared_ptr<Register> u, std::shared_ptr<Register> v);
    void freeze_moves(std::shared_ptr<Register> u);
    bool conservative(std::shared_ptr<Register>, std::shared_ptr<Register>);
    void rewrite_prologue();
    
    RegisterSet initial;
    RegisterSet simplify_worklist;
    RegisterSet freeze_worklist;
    RegisterSet spill_worklist;
    RegisterSet spilled_nodes;
    RegisterSet coalesced_nodes;
    RegisterSet colored_nodes;
    std::vector<std::shared_ptr<Register>> select_stack;
    RegisterSet in_select_stack;

    MoveSet coalesced_moves;
    MoveSet constrained_moves;
    MoveSet frozen_moves;
    MoveSet worklist_moves;
    MoveSet active_moves;
    std::unordered_map<std::shared_ptr<Register>, MoveSet> move_list;

    template <typename T1, typename T2> struct pair_hash {
        std::size_t operator()(const std::pair<T1, T2> &pair) const {
            auto hash1 = std::hash<T1>{}(pair.first);
            auto hash2 = std::hash<T2>{}(pair.second);
            return hash1 ^ (hash2 << 1); // Shift and XOR
        }
    };

    std::unordered_set<
        std::pair<std::shared_ptr<Register>, std::shared_ptr<Register>>,
        pair_hash<std::shared_ptr<Register>, std::shared_ptr<Register>>>
        edges;

    std::unordered_map<std::shared_ptr<Register>, RegisterSet> graph;
    std::unordered_map<std::shared_ptr<Register>, size_t> degree;
    std::unordered_map<std::shared_ptr<Register>, std::shared_ptr<Register>>
        alias;

    std::shared_ptr<MachineFunction> func;
    std::shared_ptr<LivenessAnalysis> liveness_analysis;

    // use for heuristic in spill
    std::unordered_map<std::shared_ptr<Register>, size_t> use_times;

    size_t float_K;
    size_t general_K;
};
