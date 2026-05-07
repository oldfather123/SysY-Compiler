#pragma once

#include "LoopDetection.hpp"
#include "PassManager.hpp"
#include "FuncInfo.hpp"
#include <memory>
#include <unordered_map>

class LoopInvariantCodeMotion: public Pass<Function>{
  public:
    // LoopInvariantCodeMotion(Module *m) : Pass(m) {}
    ~LoopInvariantCodeMotion() = default;

    void run(Function *func, AnalysisPassManager &APM) override;
  private:
    std::unordered_map<std::shared_ptr<Loop>, bool> is_loop_done_; 
    void traverse_loop(std::shared_ptr<Loop> loop, const FuncInfo *func_info_, Function *func); 
    void run_on_loop(std::shared_ptr<Loop> loop, const FuncInfo *func_info_, Function *func);
};