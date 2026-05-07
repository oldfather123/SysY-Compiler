#include "opt/engine.hpp"

#include <unordered_set>
#include <vector>

#include "log.hpp"
#include "opt/pass.hpp"

namespace opt {
std::unordered_set<OptGroup *> groups;
std::unordered_set<pass::Pass *> passes;

OptEngine opt_engine = OptEngine();

}  // namespace opt

namespace opt {
OptGroup::OptGroup(std::string name) : name(std::move(name)), engine(&opt_engine) { groups.insert(this); }

OptGroup *OptGroup::attach(OptStatus status) {
    engine->assiatance_status[status].first = this;
    this->active_status = status;
    return this;
}

OptGroup *OptGroup::execute(std::vector<std::pair<OptNode, bool>> nodes) {
    this->nodes = std::move(nodes);
    return this;
}

OptGroup *OptGroup::require(std::vector<OptStatus> requirements) {
    this->requirements = std::move(requirements);
    return this;
}

OptGroup *OptGroup::expose(std::vector<OptGroup *> exposures) {
    this->exposures = std::move(exposures);
    return this;
}

OptGroup *OptGroup::follow(std::vector<OptGroup *> followings) {
    this->followings = std::move(followings);
    return this;
}

OptGroup *OptGroup::disable(std::vector<OptStatus> destructions) {
    this->destructions = std::move(destructions);
    return this;
}

void OptEngine::pre_lauch(std::vector<OptGroup *> groups) { this->early_opts = std::move(groups); }

void OptEngine::defer(std::vector<OptGroup *> groups) { this->late_opts = std::move(groups); }

void OptEngine::cycle(std::vector<OptGroup *> groups) { this->mid_opts = std::move(groups); }

void OptEngine::set_cycle_times(int times) { this->cycle_times = times; }

void OptEngine::compile(ir::Module *module) {
    // 1. early opts
    for (auto *group : early_opts) group->run(module);

    // 2. main cycle
    for (int i = 0; i < cycle_times; i++) {
        for (const auto &group : mid_opts) {
            if (in_pipeline.count(group)) continue;
            pipeline.push_back(group);
            in_pipeline.insert(group);
        }
        while (!pipeline.empty()) {
            auto *group = pipeline.front();
            pipeline.pop_front();
            in_pipeline.erase(group);
            group->run(module);
        }
    }

    // 3. late opts
    for (auto *group : late_opts) group->run(module);

    // 4. clean up
    for (auto *group : groups) delete group;
    for (auto *pass : passes) delete pass;
}

bool OptGroup::run(ir::Module *module) {
    // printf("\nRunning %s\n", name.c_str());
    bool total_modified = false;
    for (const auto &[node, complete] : nodes) {
        bool inner_modified = false;
        do {
            inner_modified = false;
            for (const auto &requirement : requirements) {
                if (!engine->assiatance_status[requirement].second) {
                    engine->assiatance_status[requirement].first->run(module);
                }
            }
            inner_modified |=
                std::visit([module](auto &&pass_or_group) -> bool { return pass_or_group->run(module); }, node);
            total_modified |= inner_modified;
            if (active_status.has_value()) engine->assiatance_status[active_status.value()].second = true;
        } while (complete && inner_modified);
    }
    logger::INFO("Runed ", name, "...");
    // debug info
    // printf("\nRuned %s\n", name.c_str());
    // printf("Module:\n%s", module->to_string().c_str());
    if (total_modified) {
        for (const auto &destruction : destructions) engine->assiatance_status[destruction].second = false;
        for (const auto &following : followings) following->run(module);
        for (const auto &exposure : exposures) {
            if (!engine->in_pipeline.count(exposure)) {
                engine->pipeline.push_back(exposure);
                engine->in_pipeline.insert(exposure);
            }
        }
    }
    return total_modified;
}

}  // namespace opt

namespace opt {
#define USE_PASS(PassName)                   \
    auto *(PassName) = new pass::PassName(); \
    passes.insert(PassName);

#define DEFINE_GROUP(GroupName) auto *GroupName = (new OptGroup(#GroupName))  // NOLINT

void OptEngine::init() {
    // passes to be used
    USE_PASS(RemoveUnreachableInstructions);
    USE_PASS(ControlFlowGraphAnalyzation);
    USE_PASS(ConstFold);
    USE_PASS(RemoveUnreachableBasicBlocks);
    USE_PASS(DominanceAnalyzation);
    USE_PASS(Mem2Reg);
    USE_PASS(DeadCodeElimination);
    USE_PASS(FunctionInlining);
    USE_PASS(ConstArrayFold);
    USE_PASS(SparseConditionalConstantPropagation);
    USE_PASS(TailRecursionElimination);
    USE_PASS(CallGraphAnalyzation);
    USE_PASS(PureFunctionAnalysis);
    USE_PASS(GlobalValueNumbering);
    USE_PASS(GlobalCodeMotion);
    USE_PASS(GlobalScalarLocalization);
    USE_PASS(SideEffectAnalyzation);
    USE_PASS(LoopAnalyzation);
    USE_PASS(LoopSimplification);
    USE_PASS(LoopClosedSSA);
    USE_PASS(AliasAnalyzation);
    USE_PASS(LoopInvariantCodeMotion);
    USE_PASS(ScalarEvolutionAnalyzation);
    USE_PASS(InductionVariableAnalyzation);
    USE_PASS(InductionVariableReplacement);
    USE_PASS(LoopUnrolling);
    USE_PASS(LoopStrengthReduction);
    USE_PASS(TripCountAnalyzation);
    USE_PASS(PtrAllocaReduction);
    USE_PASS(BlockMerge);
    USE_PASS(SimplifyCFG);
    USE_PASS(DeadLoopEliminate);
    USE_PASS(GepLift);
    USE_PASS(LengauerTarjanDominanceAnalyzation);
    USE_PASS(PhiElimination);
    USE_PASS(FunctionAnalyzation);
    USE_PASS(PointerBaseAnalyzation);
    USE_PASS(CommonSubexpressionElimination);
    USE_PASS(LocalValueNumbering);
    USE_PASS(AggressiveDeadCodeElimination);
    USE_PASS(RedundantLoopElimination);
    USE_PASS(IntegerRangeAnalyzation);
    USE_PASS(LoadElimination);
    USE_PASS(ConstraintReduction);
    USE_PASS(ConstexprFunctionEvaluation);
    USE_PASS(RemoveUnusedFunctions);
    USE_PASS(LoopBodyExtract);
    USE_PASS(PartialEvaluate);
    USE_PASS(Reassociate);
    USE_PASS(ArithReduce);
    USE_PASS(GepSplitting);
    USE_PASS(GepFuse);
    USE_PASS(LoopGEPCombine);
    USE_PASS(ConstIdx2Value);
    USE_PASS(LoopInterchange);
    USE_PASS(ScalarReplacementOfAggregates);
    USE_PASS(LocalArrayLift);
    USE_PASS(Unconditional_Br_Inlining);
    USE_PASS(RangeFolding);
    USE_PASS(ModuloLoopOptimization);
    USE_PASS(FunctionMemoization);
    USE_PASS(AggressiveLoopElimination);
    // define groups rules
    // clang-format off

    DEFINE_GROUP(BuildCFG)
        ->execute({{RemoveUnreachableInstructions, false}, {ControlFlowGraphAnalyzation, false}})
        ->attach(CFG);

    DEFINE_GROUP(BuildDominance)
        ->execute({{RemoveUnreachableBasicBlocks, false}, {LengauerTarjanDominanceAnalyzation, false}})
        ->require({CFG})
        ->attach(DOM);

    DEFINE_GROUP(BuildSSA)
        ->execute({ {Mem2Reg, false}, {PtrAllocaReduction, false}})
        ->require({CFG, DOM})
        ->attach(SSA);

    DEFINE_GROUP(BuildCallGraph)
        ->execute({{CallGraphAnalyzation, false}})
        ->require({CFG})
        ->attach(CALL_GRAPH);

    DEFINE_GROUP(AnalyzePureFunc)
        ->execute({{PureFunctionAnalysis, false}})
        ->require({CALL_GRAPH})
        ->attach(PURE_FUNC);

    DEFINE_GROUP(AnalyzeFunc)
        ->execute({{FunctionAnalyzation, false}})
        ->attach(FUNC);

    DEFINE_GROUP(AnalyzeSideEffect)
        ->execute({{SideEffectAnalyzation, false}})
        ->require({CALL_GRAPH})
        ->attach(SIDE_EFFECT);

    DEFINE_GROUP(AnalyzePointerBase)
        ->execute({{PointerBaseAnalyzation, false}})
        ->attach(POINTER);

    DEFINE_GROUP(AnalyzeLoop)
        ->execute({{LoopAnalyzation, false}})
        ->require({CFG, DOM})
        ->attach(LOOP);

    DEFINE_GROUP(BuildSimpleLoop)
        ->execute({{LoopSimplification, false},
                   {BuildCFG, false},
                   {BuildDominance, false},
                   {LoopAnalyzation, false}})
        ->require({LOOP, CALL_GRAPH})
        ->attach(SIMPLE_LOOP);

    DEFINE_GROUP(RemovePhi)
        ->execute({{PhiElimination, false}})
        ->disable({LCSSA});

    DEFINE_GROUP(BuildLCSSA)
        ->execute({{LoopClosedSSA, false}})
        ->require({LOOP, SIMPLE_LOOP, DOM})
        ->expose({RemovePhi})
        ->attach(LCSSA);

    DEFINE_GROUP(AnalyzeAlias)
        ->execute({{AliasAnalyzation, false}})
        ->attach(ALIAS);

    DEFINE_GROUP(AnalyzeIndVar)
        ->execute({{InductionVariableAnalyzation, false}})
        ->require({LOOP, SIMPLE_LOOP})
        ->attach(IND_VAR);

    DEFINE_GROUP(AnalyzeSCEV)
        ->execute({{ScalarEvolutionAnalyzation, false}})
        ->require({LOOP, SIMPLE_LOOP})
        ->attach(SCEV);

    DEFINE_GROUP(CalculateTripCount)
        ->execute({{TripCountAnalyzation, false}})
        ->require({LOOP, IND_VAR, SCEV, SIMPLE_LOOP, LCSSA})
        ->attach(TRIP_COUNT);

    DEFINE_GROUP(LocalizeGlobalScalar)
        ->execute({{GlobalScalarLocalization, false}});

    DEFINE_GROUP(LiftLocalArray)
        ->execute({{FunctionAnalyzation, false},{LocalArrayLift, false}})
        ->require({CFG, DOM});

    DEFINE_GROUP(MergeBlock)
        ->execute({{BuildCFG, false}, {BlockMerge, false}})
        ->disable({DOM});


    DEFINE_GROUP(DCE)
        ->execute({{DeadCodeElimination, true}})
        ->require({SIDE_EFFECT});

    DEFINE_GROUP(ADCE)
        ->execute({{AnalyzeFunc, false}, {AggressiveDeadCodeElimination, true}})
        ->disable({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, CFG, DOM, FUNC});

    DEFINE_GROUP(CSE)
        ->require({CFG, CALL_GRAPH})
        // ->disable({SIDE_EFFECT})
        ->execute({{PureFunctionAnalysis, false}, {SideEffectAnalyzation, false}, {CommonSubexpressionElimination, false}});

    DEFINE_GROUP(SCCP)
        ->execute({{SparseConditionalConstantPropagation, false}})
        ->require({CFG})
        ->disable({LOOP, SIMPLE_LOOP, DOM, LCSSA, SCEV, IND_VAR, FUNC});

    DEFINE_GROUP(FoldConstant)
        ->execute({{ConstFold, false}, {ConstArrayFold, false}, {ArithReduce, false}});

    DEFINE_GROUP(InlineFunc)
        ->execute({{FunctionInlining, false}})
        ->require({CALL_GRAPH})
        ->disable({LOOP, CFG, DOM, LOOP, SIMPLE_LOOP, FUNC})
        ->expose({DCE});

    DEFINE_GROUP(EliminateTailRecursion)
        ->execute({{TailRecursionElimination, false}})
        ->disable({LOOP, CFG, DOM, LOOP, SIMPLE_LOOP, FUNC})
        ->expose({InlineFunc});

    DEFINE_GROUP(LVN)
        ->execute({{PointerBaseAnalyzation, false},{LocalValueNumbering, false}})
        ->require({DOM})
        ->expose({SCCP});

    DEFINE_GROUP(GCMGVN)
        ->execute({{PureFunctionAnalysis, false},{AnalyzeFunc, false},{GlobalCodeMotion, false}, {GlobalValueNumbering, false}, {LVN, false}})
        ->require({DOM, LOOP})
        ->expose({SCCP});

    DEFINE_GROUP(MoveLoopInvariant)
        ->execute({{LoopInvariantCodeMotion, false}})
        ->require({LOOP,SIMPLE_LOOP, LCSSA, ALIAS, SIDE_EFFECT});

    DEFINE_GROUP(ReplaceIndVar)
        ->execute({{InductionVariableReplacement, false}})
        ->require({LOOP, SIMPLE_LOOP, LCSSA, IND_VAR, TRIP_COUNT, SCEV});

    DEFINE_GROUP(UnrollLoop)
        ->execute({{LoopUnrolling, false}})
        ->require({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR})
        ->disable({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, CFG, DOM, FUNC})
        ->follow({MergeBlock, DCE})
        ->expose({SCCP, FoldConstant});

    DEFINE_GROUP(ReduceLoopStrength)
        ->execute({{LoopStrengthReduction, false}})
        ->require({LOOP, SIMPLE_LOOP, LCSSA, IND_VAR});

    DEFINE_GROUP(AnalyzeIntRange)
        ->execute({{BuildCFG, false}, {BuildDominance, false},{AnalyzeLoop, false}, {BuildCallGraph, false}, {BuildSimpleLoop, false}, {AnalyzeSCEV, false}, {AnalyzePointerBase, false}, {IntegerRangeAnalyzation, false}});

    DEFINE_GROUP(LoadEliminationGrp)
        ->execute({{LoadElimination, false}})
        ->require({CFG, DOM})
        ->expose({SCCP});

    DEFINE_GROUP(ReduceConstraints)
        ->execute({{AnalyzeIntRange, false}, {ConstraintReduction, false}})
        ->expose({SCCP});

    DEFINE_GROUP(ExtractLoopBody)
        ->execute({{LoopBodyExtract, false}})
        ->require({CFG, DOM, LOOP});

    DEFINE_GROUP(PartialEval)
        ->execute({{PartialEvaluate, false}})
        ->require({CFG, CALL_GRAPH, LOOP, SIMPLE_LOOP, LCSSA, ALIAS, SIDE_EFFECT, TRIP_COUNT})
        ->disable({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, CFG, DOM, FUNC});

    DEFINE_GROUP(ConstexprFunctionEvaluationGrp)
        ->execute({{ConstexprFunctionEvaluation, false}, {CallGraphAnalyzation, false}, {RemoveUnusedFunctions, false}})
        ->require({CALL_GRAPH, SIDE_EFFECT})
        ->disable({LOOP, CFG, DOM, LOOP, SIMPLE_LOOP, FUNC})
        ->expose({DCE, SCCP});

    DEFINE_GROUP(SimplifyCFGGrp)
        ->execute({{SimplifyCFG, true}})
        ->require({CFG})
        ->disable({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, CFG, DOM, FUNC, SIDE_EFFECT, CALL_GRAPH})
        ->expose({DCE, ADCE});

    DEFINE_GROUP(SimplifyCFGPro)
        ->execute({{SimplifyCFGGrp, true},  {ReduceLoopStrength, false}, {ReplaceIndVar, false}, {LoopUnrolling, false}})
        ->require({CFG})
        ->disable({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, CFG, DOM, FUNC, SIDE_EFFECT, CALL_GRAPH})
        ->expose({DCE, ADCE});

    DEFINE_GROUP(GepLiftGrp)
        ->execute({{GepLift, false}})
        ->require({CFG});

    DEFINE_GROUP(ReassociateGrp)
        ->execute({{Reassociate, false}})
        ->require({CFG});

    DEFINE_GROUP(GepSplittingGrp)
        ->execute({{GepSplitting, false}})
        ->disable({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, CFG, DOM, FUNC, SIDE_EFFECT, CALL_GRAPH});

    DEFINE_GROUP(GepFuseGrp)
        ->execute({{GepFuse, false}})
        ->require({CFG});

    DEFINE_GROUP(LoopGEPCombineGrp)
        ->execute({{LoopGEPCombine, false}})
        ->require({CFG});

    DEFINE_GROUP(LoopEliminationPrim)
        ->execute({{RedundantLoopElimination, false}});

    DEFINE_GROUP(LoopElimination)
        ->execute({{AnalyzeLoop, false},{BuildCallGraph, false}, {BuildSimpleLoop, false},{BuildLCSSA, false}, {FunctionAnalyzation, false},{RedundantLoopElimination, false}, {BuildCFG, false}, {BuildDominance, false}})
        ->require({CFG, DOM})
        ->disable({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, FUNC});

    DEFINE_GROUP(SROA)
        ->execute({{ScalarReplacementOfAggregates, false}})
        ->follow({LocalizeGlobalScalar, BuildSSA});

    DEFINE_GROUP(ConstIdx2ValueGrp)
        ->execute({{ConstIdx2Value, false}})
        ->require({CFG});

    DEFINE_GROUP(LoopInterchangeGrp)
        ->execute({{LoopInterchange, false}})
        ->require({LOOP, SIMPLE_LOOP, IND_VAR})
        ->follow({BuildCFG, BuildDominance, AnalyzeLoop})
        ->disable({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, CFG, DOM, FUNC});
    // ->require({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR})

    DEFINE_GROUP(BrInline)
        ->execute({{AnalyzeLoop, false},{BuildCallGraph, false}, {BuildSimpleLoop, false},{Unconditional_Br_Inlining, false},{BuildDominance, false}})
        ->require({CFG, DOM})
        ->disable({});

    DEFINE_GROUP(FoldRange)
        ->execute({{IntegerRangeAnalyzation, false}, {RangeFolding, false}})
        ->follow({BrInline, ADCE})
        ->disable({DOM, LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, CALL_GRAPH});

    DEFINE_GROUP(OptimizeModuloLoops)
        ->execute({{ModuloLoopOptimization, false}})
        ->require({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR})
        ->disable({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, CFG, DOM, FUNC})
        ->expose({DCE, ADCE});

    DEFINE_GROUP(FunctionMemoizationGrp)
        ->execute({{FunctionMemoization, false}})
        ->require({CALL_GRAPH, SIDE_EFFECT})
        ->disable({LOOP, SIMPLE_LOOP, LCSSA, TRIP_COUNT, IND_VAR, SCEV, ALIAS, CFG, DOM, FUNC});

    DEFINE_GROUP(FinalLoopAggressiveElimination)
        ->execute({{AggressiveLoopElimination, false}});

    pre_lauch({
        LocalizeGlobalScalar,
        BuildSSA,
        DCE,
        ADCE,
        GCMGVN,
        EliminateTailRecursion,
        ConstexprFunctionEvaluationGrp,
        InlineFunc,
        LocalizeGlobalScalar,
        BuildSSA,
        ReassociateGrp,
        ReduceConstraints,
        // FunctionMemoizationGrp,
        LiftLocalArray,
        MoveLoopInvariant,
        BuildDominance,
        ReduceLoopStrength,
        ReplaceIndVar,
        UnrollLoop,
        SROA,
    });
    defer({
        RemovePhi,
        ADCE,
        GepLiftGrp,
        SimplifyCFGGrp,
        ReduceConstraints,
        BrInline,
        ADCE,
        SCCP,

        BuildCFG,
        BuildDominance,
        AnalyzeLoop,
        BuildSimpleLoop,
        BuildLCSSA,
        BuildCallGraph,
        AnalyzeIndVar,
        AnalyzeSCEV,
        CalculateTripCount,
        ReplaceIndVar,
        FinalLoopAggressiveElimination,
        RemovePhi,
        SimplifyCFGGrp,
        BrInline,
        MergeBlock,
        ADCE,

        FunctionMemoizationGrp,
        DCE,
        ADCE,
        RemovePhi,
        GCMGVN,
        MergeBlock,

    });

    cycle({
        LiftLocalArray,
        FoldConstant,
        LoopGEPCombineGrp,
        DCE,
        ADCE,
        RemovePhi,
        FoldConstant,
        FoldRange,
        ConstexprFunctionEvaluationGrp,
        SCCP,
        SimplifyCFGPro,
        BrInline,
        MergeBlock,
        GCMGVN,
        LoopElimination,
        LoadEliminationGrp,
        GepSplittingGrp,
        ReduceLoopStrength,
        ReplaceIndVar,
        UnrollLoop,
    });
    // clang-format on
}
}  // namespace opt
