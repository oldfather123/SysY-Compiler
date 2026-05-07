#pragma once

#include "FuncInfo.hpp"
#include "LoopDetection.hpp"
#include "PassManager.hpp"
#include <memory>
#include <unordered_map>
#include <set>
#include <vector>


//深层函数无法提取，正在改，目前功能很垃圾没啥用。
class LoopInvariantCodeMotion : public Pass {
  public:
    LoopInvariantCodeMotion(Module *m) : Pass(m) {}
    ~LoopInvariantCodeMotion() = default;

    void run() override;

  private:
    std::unordered_map<std::shared_ptr<Loop>, bool> is_loop_done_;
    std::unique_ptr<LoopDetection> loop_detection_;
    std::unique_ptr<FuncInfo> func_info_;
    bool is_value_available_in_preheader(Value *val, std::shared_ptr<Loop> loop);
    
    // 用于跟踪已识别的循环不变指令
    std::set<Value*> loop_invariant_instrs_;
    
    // 用于跟踪load指令访问的地址
    std::set<Value*> load_addresses_;
    
    // 遍历循环（从内到外）
    void traverse_loop(std::shared_ptr<Loop> loop);
    
    // 在特定循环上执行LICM
    //void run_on_loop(std::shared_ptr<Loop> loop);
    int run_on_loop(std::shared_ptr<Loop> loop);
    
    // 收集循环相关信息
    void collect_loop_info(std::shared_ptr<Loop> loop,
                          std::set<Value *> &loop_instructions,
                          std::set<Value *> &updated_values,
                          std::set<BasicBlock *> &loop_blocks,
                          bool &contains_impure_call);
    
    // 检查指令是否可以安全地外提
    bool is_safe_to_hoist(Instruction *instr,
                         const std::set<Value *> &loop_instructions,
                         const std::set<Value *> &updated_values,
                         const std::set<BasicBlock *> &loop_blocks);
    
    // 简单的别名分析
    bool may_alias(Value *ptr1, Value *ptr2);
    
    // 创建preheader基本块
    BasicBlock* create_preheader(std::shared_ptr<Loop> loop);
    
    // 将指令移动到preheader
    void move_to_preheader(Instruction *instr, BasicBlock *preheader);
    
    // 对需要外提的指令进行拓扑排序
    std::vector<Instruction*> topological_sort(const std::vector<Instruction*> &instrs);
};