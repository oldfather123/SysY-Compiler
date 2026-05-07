package midend.optimism;

import config.Config;
import midend.Module;
import midend.analysis.CFGBuilder;

import java.io.IOException;

public class Optimizer {
    private Module module;

    public Optimizer(Module module) {
        this.module = module;
    }

    public void run() throws IOException {
        new CFGBuilder(module).run();
        new Mem2Reg(module).run();
        new DeadCodeRemove(module).run();
        new ALUOptimize(module, false).run();
        new GVN(module).run();
        if (Config.isO1) {
            new LoopExtract(module).run();
            new CFGBuilder(module).run();
            new LoopSimplify(module, true).run();
            new CFGBuilder(module).run();
            new DeadCodeRemove(module).run();
            new RemovePhi(module, true).run();
//
            new MergeBB(module).run();
            new CFGBuilder(module).run();

//            if (new RecursionToLoop(module).run()) {
//                new ALUOptimize(module, true).run();
//                new CFGBuilder(module).run();
//                new RemovePhi(module, false).run();
//                return;
//            }
            new RecursionToLoop(module).run();
            new CFGBuilder(module).run();
        }
        new GCM(module).run();
        new FuncInline(module).run();
        new CFGBuilder(module).run();
        new MergeBB(module).run();
        new CFGBuilder(module).run();
        new GlobalValueLocal(module).run();
        new Mem2Reg(module).run();

        new GVN(module).run();
        new LoadGVN(module).run();
        new PeepHole(module).run();
        new GVN(module).run();
        new MergeBB(module).run();
        new CFGBuilder(module).run();
        new GCM(module).run();
        new CFGBuilder(module).run();
        if (Config.isO1) {
//            new LoopExtract(module).run();
//            new CFGBuilder(module).run();

//            new LoopSimplify(module).run();
//            new CFGBuilder(module).run();
            new GVN(module).run();
            new LCSSA(module).run();
//            new ConstLoopUnroll(module).run();
//            new CFGBuilder(module).run();
            new LoopUnroll(module).run();
            new CFGBuilder(module).run();
            new GVN(module).run();
            new LoadGVN(module).run();
            new PeepHole(module).run();

            new RemoveUseLessStoreLoad(module).run();
            new GepFuse(module).run();
            new GVN(module).run();
            new ALUOptimize(module, false).run();
            new DeadCodeRemove(module).run();
            new RemoveUseLessStore(module).run();
            new RecursionMemory(module).run();
            new O4optimism(module).run();
//            new RecursiveMem(module).run();
            new CFGBuilder(module).run();
//            new RemovePhi(module, true).run();
            new MergeBB(module).run();
            new CFGBuilder(module).run();

            new RemoveArgStoreLoad(module).run();
            new GVN(module).run();
            new ALUOptimize(module, false).run();
            new DeadCodeRemove(module).run();
            new RemoveUseLessStore(module).run();
            new RemovePhi(module, true).run();
            new MergeBB(module).run();
            new CFGBuilder(module).run();

            new LoopSimplify(module, false).run();
            new ALUOptimize(module, true).run();
            new GVN(module).run();
            new DeadCodeRemove(module).run();

            new IfOptimize(module).run();
            new GVN(module).run();
            new DeadCodeRemove(module).run();

            new ArrayLift(module).run();
            new DeadCodeRemove(module).run();
            new GVN(module).run();
            new GCM(module).run();
            new DeadCodeRemove(module).run();

            new GepSeparate(module).run();
            new DeadCodeRemove(module).run();
            new GVN(module).run();
            new GCM(module).run();
//        new PeepHole(module).run();
            new MergeBB(module).run();
            new CFGBuilder(module).run();
            new GVN(module).run();
            new GCM(module).run();

        }
//        module.maintainPhiBlocks();
//        if (!Config.isO1) {
        new ALUOptimize(module, true).run();
        new GVN(module).run();
        new DeadCodeRemove(module).run();
        new GCM(module).run();
//////        }
        new RemovePhi(module, true).run();
        new MergeBB(module).run();
        new CFGBuilder(module).run();

//        if (Config.isO1) {
//            new Parallel(module).run();
//            new CFGBuilder(module).run();
//        }

        new RemovePhi(module, false).run();
//        new MergeBB(module).run();
//        new CFGBuilder(module).run();
    }
}
