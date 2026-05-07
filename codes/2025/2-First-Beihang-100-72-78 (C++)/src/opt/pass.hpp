#ifndef GEECEECEE_OPT_PASS_HPP
#define GEECEECEE_OPT_PASS_HPP
#include "ir/mod.hpp"
#include "log.hpp"

namespace opt::pass {
class Pass;

class Pass {
public:
    virtual ~Pass() = default;
    virtual bool run(ir::Module *module) = 0;
};

#define COMMON_PASS(PassName) /* NOLINT */     \
    class PassName : public Pass {             \
    public:                                    \
        bool run(ir::Module *module) override; \
    }

COMMON_PASS(RemoveUnreachableInstructions);
COMMON_PASS(ControlFlowGraphAnalyzation);
COMMON_PASS(ConstFold);
COMMON_PASS(RemoveUnreachableBasicBlocks);
COMMON_PASS(DominanceAnalyzation);
COMMON_PASS(LengauerTarjanDominanceAnalyzation);
COMMON_PASS(Mem2Reg);
COMMON_PASS(RemovePhi);
COMMON_PASS(DeadCodeElimination);
COMMON_PASS(AggressiveDeadCodeElimination);
COMMON_PASS(RedundantLoopElimination);
COMMON_PASS(Unconditional_Br_Inlining);
COMMON_PASS(FunctionInlining);
COMMON_PASS(GepSplitting);  // This optimization is deprecated, as it interferes with the ConstArrayFold optimization
COMMON_PASS(ConstArrayFold);
COMMON_PASS(SparseConditionalConstantPropagation);
COMMON_PASS(TailRecursionElimination);
COMMON_PASS(CallGraphAnalyzation);
COMMON_PASS(PureFunctionAnalysis);
COMMON_PASS(LocalArrayLift);
COMMON_PASS(GlobalValueNumbering);
COMMON_PASS(LocalValueNumbering);
COMMON_PASS(GlobalCodeMotion);
COMMON_PASS(GlobalScalarLocalization);
COMMON_PASS(SideEffectAnalyzation);
COMMON_PASS(LoopAnalyzation);
COMMON_PASS(LoopRotation);
COMMON_PASS(LoadElimination);
COMMON_PASS(LoopSimplification);
COMMON_PASS(LoopClosedSSA);
COMMON_PASS(AliasAnalyzation);
COMMON_PASS(LoopInvariantCodeMotion);
COMMON_PASS(ScalarEvolutionAnalyzation);
COMMON_PASS(InductionVariableCanonicalization);
COMMON_PASS(InductionVariableAnalyzation);
COMMON_PASS(InductionVariableReplacement);
COMMON_PASS(LoopUnrolling);
COMMON_PASS(LoopStrengthReduction);
COMMON_PASS(TripCountAnalyzation);
COMMON_PASS(PtrAllocaReduction);
COMMON_PASS(RedundantRetEliminate);
COMMON_PASS(FunctionAnalyzation);
COMMON_PASS(BlockMerge);
COMMON_PASS(SimplifyCFG);
COMMON_PASS(DeadLoopEliminate);
COMMON_PASS(ScalarReplacementOfAggregates);
COMMON_PASS(GepLift);
COMMON_PASS(PointerBaseAnalyzation);
COMMON_PASS(PhiElimination);
COMMON_PASS(CommonSubexpressionElimination);
COMMON_PASS(IntegerRangeAnalyzation);
COMMON_PASS(ConstraintReduction);
COMMON_PASS(ConstexprFunctionEvaluation);
COMMON_PASS(RemoveUnusedFunctions);
COMMON_PASS(Reassociate);
COMMON_PASS(ArithReduce);
COMMON_PASS(LoopBodyExtract);
COMMON_PASS(PartialEvaluate);
COMMON_PASS(GepFuse);
COMMON_PASS(LoopGEPCombine);
COMMON_PASS(ConstIdx2Value);
COMMON_PASS(LoopInterchange);
COMMON_PASS(RangeFolding);
COMMON_PASS(ModuloLoopOptimization);
COMMON_PASS(FunctionMemoization);
COMMON_PASS(AggressiveLoopElimination);
}  // namespace opt::pass

#endif  // GEECEECEE_OPT_PASS_HPP
