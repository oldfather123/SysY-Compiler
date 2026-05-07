package pass;

import backend.Backend;
import backend.component.ObjModule;
import backend.regalloc.GraphColoringAlloc;
import driver.Config;
import ir.IrFactory;
import ir.IrModule;
import ir.instr.InstructionCombiner;
import pass.ir.*;

import java.util.List;

public class Controller {

    private static final List<Pass.IrPass> irPasses = List.of(
            new MergeBasicBlock(),
            new DomTree(),
            new Mem2Reg(),                  // Depend on DomTree
            new FunctionInline(),
            new MergeBasicBlock(),
            new ConstantPropagation(),
            new GVN(),
            new InstructionCombiner(),
            new ConstantFold(),
            new ADCE(),
            new DomTree(),
            new LoopAnalysis(),             // Depend on DomTree
            new LoopHoist(),                // Depend on LoopAnalysis
            new LoopInductionSimplify(),    // Depend on LoopHoist, Require Mem2Reg
            new ADCE(),
            new FalseADCE(),
            new MergeBasicBlock()
    );

    private static final List<Pass.ObjPass> objPasses = List.of(

    );

    public static void runIrPass(IrFactory ir) {
        emitIr(ir, () -> "before-ir-pass");
        for (Pass.IrPass pass : irPasses) {
            pass.run(ir.getModule());
            emitIr(ir, pass);
//             runBackendPass(ir.getModule(), pass.getName());
        }
    }

    public static void runObjPass(ObjModule obj) {
        for (Pass.ObjPass pass : objPasses) {
            pass.run(obj);
        }
    }

    /**
     * 无论开不开优化，都要执行的 Pass
     *
     * @param ir   Ir 的模块内容
     * @param pass IrPass 实例
     */
    public static void runNormalPass(IrFactory ir, Pass.IrPass pass) {
        pass.run(ir.getModule());
    }

    public static void runBackendPass(IrModule ir, String middleName) {
        if (Config.emitRISCV == null) {
            return;
        }
        Backend backend = new backend.Backend(ir);
        backend.removePhi();
        backend.setExit();
        GraphColoringAlloc graphColoringAlloc = new GraphColoringAlloc(backend.getModule());
        graphColoringAlloc.run();
        pass.Controller.runObjPass(backend.getModule());
        if (Config.emitRISCV.equals("<stdout>")) {
            System.out.println("=== RISC V: " + middleName + " ===");
            backend.emitBackend("<stdout>");
        } else {
            backend.emitBackend(String.format("%s.%02d.%s.S", Config.emitRISCV, emitIndex, middleName));
        }
    }

    private static int emitIndex = 0;

    private static void emitIr(IrFactory ir, Pass pass) {
        if (Config.emitIrEachTime && Config.emitIr != null) {
            if (Config.emitIr.equals("<stdout>")) {
                System.out.println("==== " + pass.getName() + " ====");
                ir.emitIr(null);
            } else {
                ir.emitIr(String.format("%s.%02d.%s.ll", Config.emitIr, emitIndex++, pass.getName()));
            }
        }
        if (Config.emitDotBlock != null) {
            ir.getModule().emitDotBlock(String.format("%s.%02d.%s.dot", Config.emitDotBlock, emitIndex, pass.getName()));
        }
    }
}
