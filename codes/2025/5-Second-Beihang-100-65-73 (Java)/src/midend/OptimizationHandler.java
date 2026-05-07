package midend;

import frontend.ir.structure.Function;
import frontend.ir.structure.IRModel;
import frontend.ir.symbol.SymTab;
import midend.Analysis.*;
import midend.Analysis.SCEV.SCEVAnalyse;
import midend.DCE.*;
import midend.range.RangeAnalysisV3;

import java.util.List;

public class OptimizationHandler {
    private static final OptimizationHandler OPTIMIZATION_HANDLER = new OptimizationHandler();

    private OptimizationHandler() {

    }

    public static OptimizationHandler getInstance() {
        return OPTIMIZATION_HANDLER;
    }

    /**
     * @param opt 之后如果有比较激进的优化写到 `if (opt) {}` 里面
     */
    public void execute(IRModel irModel, boolean opt, boolean runARM, boolean runRV) {
        if (irModel == null) {
            throw new NullPointerException();
        }
        List<Function> functions = irModel.getFunctions();
        SymTab globalSymTab = irModel.getGlobalSymTab();

        CFG.execute(functions);
        Mem2Reg.execute(functions);
        ArrayFParamMem2Reg.execute(functions);
        BranchInsSimplify.execute(functions);

        RemoveDeadFunction.execute(functions);
        RemoveDeadCode.execute(functions);

        FunctionInline.execute(functions);

//        LocalArrayExtraction.execute(functions, globalSymTab);  // 其实没使用过或者只定义一次的数组也可以找出来干掉，但是还没做
        GlobalValueSimplify.execute(globalSymTab);

        // C0级别，简单常量传播
        CFG.execute(functions);
        Mem2Reg.execute(functions);
        ConstFold.execute(functions);
        CFG.execute(functions);

        // C1 - 0级别
        RemoveDeadArgument.execute(functions);
        PureFunctionAnalysis.execute(functions);
        GlobalScalar2Param.execute(functions);
        PureFunctionAnalysis.execute(functions);
        LoopAnalysis.execute(functions);
        FuncMemorizeV3.execute(functions, globalSymTab);

        CFG.execute(functions);
        BlockMerge.execute(functions, BlockMerge.MERGE_STRATEGY.NORMAL);
        CFG.execute(functions);

        // C1 - 1A级别（分析）
        LoopAnalysis.execute(functions);
        LoopSimplify.execute(functions);
        CFG.execute(functions);
        LCSSA.execute(functions);

        LoopMemoryLift.execute(functions);
        SCEVAnalyse.execute(functions, false);
        TripCountAnalyzer.execute(functions);
        RemoveDeadCode.execute(functions);
        LoopContentAnalyse.execute(functions);

        LoopAnalysis.execute(functions);
        BlockReposition.execute(functions);
        // C1 - 1B级别（优化）
        LoopUnrollConst.execute(functions);
        ConstFold.execute(functions);

        // C2 - 0级别
        CFG.execute(functions);
        LoopAnalysis.execute(functions);
        RemoveDeadCode.execute(functions);
        GCM.execute(functions, GCM.STRATEGY.DOMEARLY);
        ConstFold.execute(functions);
        CFG.execute(functions);
        Reassociate.execute(functions);
        GVN.execute(functions);
        GCM.execute(functions, GCM.STRATEGY.ASLATE);

        BlockMerge.execute(functions, BlockMerge.MERGE_STRATEGY.NORMAL);
        CFG.execute(functions);
        RemoveDeadCode.execute(functions);

//        OutputHandler.getInstance().printIR(Arg.ARG.getIrWriter(), irModel);

        /* 内存相关pass如下组合效果比较好 */
        LoopAnalysis.execute(functions);
        LoopSimplify.execute(functions);
        CFG.execute(functions);
        AliasAnalysis.execute(functions);
        LoadCSE.execute(functions);
        if (opt) {
            StoreToLoadForwardingV2.execute(functions, globalSymTab, true);
        }
        StoreCSE.execute(functions);

        LoopMemoryLift.execute(functions);
        RemoveDeadCode.execute(functions);
        ConstFold.execute(functions);

        CFG.execute(functions);
        LoopAnalysis.execute(functions);
        GVN.execute(functions);
        GCM.execute(functions, GCM.STRATEGY.DOMEARLY);
        // GCM应当依赖于LoopAnalysis, 此处GVN/GCM完成后应当再次分析，尝试解除 header-bound rule 的限制

        LoopAnalysis.execute(functions);
        LoopSimplify.execute(functions);
        CFG.execute(functions);
        LCSSA.execute(functions);
        SCEVAnalyse.execute(functions, false);
        TripCountAnalyzer.execute(functions);
        SCEVSimplify.execute(functions);
        LoopContentAnalyse.execute(functions);
        DeadLoopErase.execute(functions);
        RemoveDeadCode.execute(functions);

        //* 这是一整套循环分析 *//*
        CFG.execute(functions);
        LoopAnalysis.execute(functions);
        LoopSimplify.execute(functions);
        CFG.execute(functions);
        LCSSA.execute(functions);
        SCEVAnalyse.execute(functions, false);
        TripCountAnalyzer.execute(functions);

        if (opt) {
            RangeAnalysisV3.execute(functions);
            RemoveDeadCode.execute(functions);
            RemoveDeadBranch.execute(functions);
        }
        CFG.execute(functions);

        LoopAnalysis.execute(functions);
        LoopSimplify.execute(functions);
        CFG.execute(functions);
        LCSSA.execute(functions);
        SCEVAnalyse.execute(functions, false);
        //RangeAnalysisV3.execute(functions);
        TripCountAnalyzer.execute(functions);
        LoopContentAnalyse.execute(functions);

        if (!runARM) {
            // Loop2Memset.execute(functions);           // 这个 PASS 的效果略微不如展开之后向量化存储，对于 ARM 没有开的必要
            PureFunctionAnalysis.execute(functions);
            CFG.execute(functions);
            GCM.execute(functions, GCM.STRATEGY.DOMEARLY);
        }

        LoopUnrollN.execute(functions);
        CFG.execute(functions);
        BlockMerge.executeForRange(functions, BlockMerge.MERGE_STRATEGY.NORMAL);
        CFG.execute(functions);

        LoopAnalysis.execute(functions);
        LoopSimplify.execute(functions);
        CFG.execute(functions);
        LCSSA.execute(functions);
        SCEVAnalyse.execute(functions, false);
        TripCountAnalyzer.execute(functions);

        if (opt) {
            RangeAnalysisV3.execute(functions);
        }

        LoopAnalysis.execute(functions);
        RemoveDeadCode.execute(functions);
        GCM.execute(functions, GCM.STRATEGY.DOMEARLY);
        ConstFold.execute(functions);
        CFG.execute(functions);
        GVN.execute(functions);

        BlockMerge.execute(functions, BlockMerge.MERGE_STRATEGY.HOIST);
        CFG.execute(functions);
        RemoveDeadCode.execute(functions);
        GCM.execute(functions, GCM.STRATEGY.ASLATE);

        LoopAnalysis.execute(functions);
        LoopSimplify.execute(functions);
        CFG.execute(functions);
        if (opt) {
            AliasAnalysis.execute(functions);
            LoadCSE.execute(functions);
            StoreToLoadForwardingV2.execute(functions, globalSymTab, true);
            StoreCSE.execute(functions);
        }


        LoopMemoryLift.execute(functions);
        RemoveDeadCode.execute(functions);
        ConstFold.execute(functions);

        CFG.execute(functions);

        if (runARM) {
            LoopAnalysis.execute(functions);
            LoopSimplify.execute(functions);
            CFG.execute(functions);
            LCSSA.execute(functions);
            SCEVAnalyse.execute(functions, false);
            TripCountAnalyzer.execute(functions);
            VectorizationDetect.execute(functions);
            ConstFold.execute(functions);
        }

        CFG.execute(functions);
        LoopAnalysis.execute(functions);
        LoopSimplify.executePreHeaderOnly(functions);
//        BlockReposition.execute(functions);
    }
}
