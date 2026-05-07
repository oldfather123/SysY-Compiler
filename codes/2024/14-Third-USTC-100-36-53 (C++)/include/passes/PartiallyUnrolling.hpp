#include "Instruction.hpp"
#include "LoopDetection.hpp"
#include <optional>
#include "Value.hpp"
#include "PassManager.hpp"

class PartiallyUnrolling : public Pass<Function> {
  public:
    // PartiallyUnrolling(Module *m) : Pass(m) {}
    ~PartiallyUnrolling() = default;
    void run(Function *f, AnalysisPassManager &APM) override;

  private:

    static constexpr int UNROLL_TIMES = 16;

    // the loop that has 1 header, 1 body, 1 latch and 1 exit
    struct SimpleLoopInfo {
        std::vector<BasicBlock *> bbs, bodies;
        BasicBlock *header{nullptr}, *exit{nullptr}, *preheader{nullptr};
        Value *ind_var{nullptr}, *initial{nullptr}, *bound{nullptr}, *step{nullptr};
        Instruction *cond{nullptr};
    };

    static std::optional <SimpleLoopInfo> parse_simple_loop(const std::shared_ptr<Loop>);

    static bool should_unroll(const SimpleLoopInfo &simple_loop);

    static void unroll_simple_loop(const SimpleLoopInfo &simple_loop, Module *m_);

    static void handle_loop(std::shared_ptr<Loop> loop, Module *m_);

    static Value* find_map(Instruction* inst, int i, const SimpleLoopInfo &simple_loop, std::map<Instruction* , std::vector<Instruction* >> oldnew_inst);
};
