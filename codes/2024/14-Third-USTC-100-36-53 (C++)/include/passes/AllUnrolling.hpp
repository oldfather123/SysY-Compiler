#pragma once
#include "LoopDetection.hpp"
#include <optional>
#include "Value.hpp"
#include "PassManager.hpp"

class AllUnrolling final : public Pass<Function> {
  public:
    //AllUnrolling(Module *m) : Pass(m) {}
    ~AllUnrolling() = default;
    void run(Function *f, AnalysisPassManager &APM) override;

  private:

    static constexpr int UNROLL_MAX = 10000;
    static constexpr int DYNAMIC_UNROLL_TIMES = 4;

    // the loop that has 1 header, 1 body, 1 latch and 1 exit
    struct SimpleLoopInfo {
        std::vector<BasicBlock *> bbs, bodies;
        BasicBlock *header{nullptr}, *exit{nullptr}, *preheader{nullptr};
        Value *ind_var{nullptr}, *bound_var{nullptr};
        ConstantInt *initial{nullptr}, *bound{nullptr}, *step{nullptr};
        ICmpInst::OpID icmp_op;
        bool static_unroll;
    };

    std::vector<BasicBlock *> has_been_unrolled;

    static std::optional <SimpleLoopInfo>
    parse_simple_loop(const std::shared_ptr<Loop>);

    static bool should_unroll(const SimpleLoopInfo &simple_loop);

    static void static_unroll_simple_loop(const SimpleLoopInfo &simple_loop, Module *m_);

    void dynamic_unroll_simple_loop(const SimpleLoopInfo &simple_loop, Module *m_);

    static Value* find_map(Instruction* inst, int i, const SimpleLoopInfo &simple_loop, std::map<Instruction* , std::vector<Instruction* >> oldnew_inst);
};

