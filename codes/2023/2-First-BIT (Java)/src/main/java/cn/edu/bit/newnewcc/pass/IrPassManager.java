package cn.edu.bit.newnewcc.pass;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.pass.ir.*;

public class IrPassManager {

    public static void optimize(Module module, int optimizeLevel) {
        runFrontendCodeFixPass(module);
        switch (optimizeLevel) {
            case 0 -> {
            }
            case 1 -> {
                MemoryToRegisterPass.runOnModule(module);
                runSimplifyPasses(module);

                runInstructionMatchPass(module);
                runSimplifyPasses(module);

                runFunctionPasses(module);
                runSimplifyPasses(module);

                runLoopPasses(module);
                runSimplifyPasses(module);

                runArrayPasses(module);
                runSimplifyPasses(module);

                runFunctionPasses(module);
                runSimplifyPasses(module);

                runTrickyPasses(module);
                runSimplifyPasses(module);

                // 调整指令顺序，以得到更好的寄存器分配结果
                InstructionSchedulePass.runOnModule(module);

            }
        }
        IrSemanticCheckPass.verify(module);
        IrNamingPass.runOnModule(module);
    }

    /**
     * 修复前端产生的代码中的一些缺陷，包括： <br>
     * <ol>
     *   <li>前端使用store整个数组的方式初始化数组，对其进行展开</li>
     *   <li>前端为跳转语句生成了过多不必要的指令，将其简化</li>
     *   <li>前端可能产生无效代码，导致编译性能下降，将其删除</li>
     * </ol>
     *
     * @param module 被优化的模块
     */
    private static void runFrontendCodeFixPass(Module module) {
        LocalArrayInitializePass.runOnModule(module);
        BranchInstructionSimplifyPass.runOnModule(module);
        DeadCodeEliminationPass.runOnModule(module);
    }

    private static void runInstructionMatchPass(Module module) {
        BranchToMinMaxPass.runOnModule(module);
    }

    private static void runFunctionPasses(Module module) {
        TailRecursionEliminationPass.runOnModule(module);
        FunctionInline.runOnModule(module);
        GvToLvPass.runOnModule(module);
    }

    private static void runTrickyPasses(Module module) {
        IsmToImmPass.runOnModule(module);
    }

    private static void runLoopPasses(Module module) {
        while (true) {
            boolean changed = false;
            while (ConstLoopUnrollPass.runOnModule(module)) {
                changed = true;
                runSimplifyPasses(module);
            }
            while (LoopUnrollPass.runOnModule(module)) {
                changed = true;
                runSimplifyPasses(module);
            }
            if (!changed) break;
        }
    }

    private static void runArrayPasses(Module module) {
        SroaPass.runOnModule(module);
        LocalArrayPromotionPass.runOnModule(module);
        ConstArrayInlinePass.runOnModule(module);
        ArrayOffsetCompressPass.runOnModule(module);
    }

    private static void runUnevaluableSimplifyPasses(Module module) {
        AddToMulPass.runOnModule(module);
        GlobalCodeMotionPass.runOnModule(module);
        MemoryAccessOptimizePass.runOnModule(module);
    }

    private static boolean runQuickEvaluablePasses(Module module) {
        boolean changed;
        changed = InstructionCombinePass.runOnModule(module);
        changed |= ConstantFoldingPass.runOnModule(module);
        changed |= JumpInstMergePass.runOnModule(module);
        changed |= BranchSimplifyPass.runOnModule(module);
        changed |= BasicBlockMergePass.runOnModule(module);
        changed |= DeadCodeEliminationPass.runOnModule(module);
        return changed;
    }

    private static boolean runEvaluablePasses(Module module) {
        boolean changed = false;
        while (runQuickEvaluablePasses(module)) {
            changed = true;
        }
        changed |= RangeRelatedConstantFoldingPass.runOnModule(module);
        return changed;
    }

    private static void runSimplifyPasses(Module module) {
        for (int t = 0; t < 2; t++) {
            while (runEvaluablePasses(module)) ;
            runUnevaluableSimplifyPasses(module);
        }
        while (runEvaluablePasses(module)) ;
    }

}
