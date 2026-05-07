#pragma once

#include "PassManager.hpp"
#include "Module.hpp"
#include "BasicBlock.hpp"
#include "LoopInvHoist.hpp"
struct LoopInfoBase
{
  Value *start = nullptr;
  Value *end = nullptr;
  Value *step = nullptr;
};

using BBset_t = std::unordered_set<BasicBlock *>;
using LoopTree = std::unordered_map<BBset_t *, std::unordered_set<BBset_t *>>;
using LoopInfo = std::unordered_map<BBset_t *, std::tuple<Value *, Value *, Value *, Value *, std::string, std::string>>;
//(add,sub),(le,ge,lt,gt)
//while_bb after_end_bb start_bb  back_bb  update_val start   end   step step_type icmp_type
using new_LoopInfo = std::unordered_map<BBset_t *, std::tuple<BasicBlock* ,BasicBlock*,BasicBlock* ,BasicBlock* ,Value *,Value *, Value *, Value *, Value *, std::string, std::string>>;

struct ArrayAccessInfo
{
  std::string array_name;
  std::vector<Value *> indices;
  std::string access_type;              // "load" 或 "store"
  std::vector<Value *> used_loop_vars;  // 有序，父循环变量排前
  std::vector<Value *> current_loop_vars; // 当前循环的全部变量
  // Value* array_base;，待实现
};

using LoopArrayAccessInfo = std::unordered_map<BBset_t *, std::vector<ArrayAccessInfo>>;

// 循环变量累积映射类型（循环 -> 有序循环变量列表）
using CumulativeLoopVars = std::unordered_map<BBset_t *, std::vector<Value *>>;

class Looplook : public Pass
{
public:
  LoopTree loop_tree;
  LoopInfo loop_info;
  new_LoopInfo new_loop_info;
  LoopArrayAccessInfo loop_array_access;

  Looplook(Module *m) : Pass(m) {}

  void run() override;
  LoopTree &get_looptree() { return loop_tree; }
  LoopInfo &get_loopinfo() { return loop_info; }
  new_LoopInfo &new_get_loopinfo() { return new_loop_info; }
  LoopArrayAccessInfo &get_looparrayinfo() { return loop_array_access; }
  void clearloopinfo()
  {
    loop_tree.clear();
    loop_info.clear();
    loop_array_access.clear();
  }

  void get_loop_info(LoopTree &loop_tree, LoopInfo &loop_info);
  void new_get_loop_info(const std::unordered_set<BBset_t*> &all_loops, new_LoopInfo &loop_info);
  void collect_array_access(const LoopTree &loop_tree,
                            const LoopInfo &loop_info,
                            LoopArrayAccessInfo &loop_array_access);
  void analyze_array_access(
      Value *ptr,
      const std::string &access_type,
      const std::vector<Value *> &loop_vars,
      std::vector<ArrayAccessInfo> &access_list);
  bool is_loop_var(Value *idx, const std::vector<Value *> &loop_vars);
  void build_cumulative_loop_vars_ordered(
      const LoopTree &loop_tree,
      const LoopInfo &loop_info,
      BBset_t *current_loop,
      const std::vector<Value *> &inherited_vars,
      std::unordered_map<BBset_t *, std::vector<Value *>> &cumulative_loop_vars);
  bool contains_var(const std::vector<Value *> &vars, Value *v);

  void print_loop_tree(const LoopTree &loop_tree,
                       const LoopInfo &loop_info,
                       BBset_t *loop, int level);
  void print_all_loops(const LoopTree &loop_tree, const LoopInfo &loop_info);
  void print_array_access_info(const LoopArrayAccessInfo &access_info,const LoopTree &loop_tree);
  BasicBlock *find_loop_header(BBset_t *loop);
};
