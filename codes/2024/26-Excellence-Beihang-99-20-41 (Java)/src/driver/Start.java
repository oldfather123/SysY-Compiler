package driver;

import backend.Backend;
import backend.regalloc.GraphColoringAlloc;
import backend.regalloc.FastLinearScannerAlloc;
import backend.process.RemovePhi;
import ir.IrFactory;
import pass.ir.*;

import java.io.IOException;

import static pass.Controller.runNormalPass;

/**
 * Usage: <code>java -jar compiler.jar [source] [-o output] [-O1] [--emit-ir filename]
 * [--emit-ast] [--emit-ir-each-time] [--emit-vr filename] [--emit-withoutPhi filename]</code>
 * Arguments are as follows:
 *
 * <li> <code>source</code>: the source file </li>
 * <li> <code>-o output</code>: the output file </li>
 * <li> <code>-O1</code>: optimization level 1 </li>
 * <li> <code>--emit-ir filename</code>: emit IR to the file, file <code>&lt;stdout&gt;</code> means to output to the console </li>
 * <li> <code>--emit-ast</code>: emit AST </li>
 * <li> <code>--emit-ir-each-time</code>: emit IR each time </li>
 * <li> <code>--emit-vr</code>: emit assembly program with phi and virtual reg </li>
 * <li> <code>--emit-withoutPhi</code>: emit assembly program with virtual reg </li>
 */
public class Start {
    public static void main(String[] args) throws IOException {
        // 遍历args，解析编译器参数
        for (int i = 1; i < args.length; i++) {
            if (args[i].equals("-o")) {
                Config.output = args[i + 1];
                i++;
            } else if (args[i].equals("debug") || args[i].equals("-d")) {
                Config.debug = true;
            } else if (args[i].equals("-O1")) {
                Config.optLevel = 1;
            } else if (args[i].equals("--emit-ast")) {
                Config.emitAst = true;
            } else if (args[i].equals("--emit-ir")) {
                Config.emitIr = args[i + 1];
                i++;
            } else if (args[i].equals("--emit-ir-each-time")) {
                Config.emitIrEachTime = true;
            } else if (args[i].equals("--emit-vr")) {
                Config.emitRISCV = args[i + 1];
                i++;
            } else if (args[i].equals("--emit-withoutPhi")) {
                Config.emitWithoutPhi = args[i + 1];
                i++;
            } else if (args[i].equals("--emit-block")) {
                Config.emitDotBlock = args[i + 1];
                i++;
            } else if (!args[i].startsWith("-")) {
                Config.source = args[i];
            }
        }

        IrFactory ir = frontend.Controller.start();
        if (Config.optLevel > 0) {
            if (Config.emitIr != null) {
                Config.emitIr += ".opt";
            }
            pass.Controller.runIrPass(ir);
        } else {
            pass.Controller.runNormalPass(ir, new MergeBasicBlock());
            pass.Controller.runNormalPass(ir, new ConstantPropagation());
            pass.Controller.runNormalPass(ir, new GVN());
            pass.Controller.runNormalPass(ir, new FalseADCE());
            // pass.Controller.runNormalPass(ir, new Mem2Reg());
            pass.Controller.runNormalPass(ir, new MergeBasicBlock());
        }

        if (Config.emitIr != null) {
            runNormalPass(ir, new OperandsRename());  // 整理操作符，供 IR 使用
            // runBackendPass(ir.getModule(), "final");
            if (Config.emitIr.equals("<stdout>")) {
                if (Config.emitIrEachTime) {
                    System.out.println("==== final-ir ====");
                }
                ir.emitIr(null);
            } else {
                ir.emitIr(Config.emitIr);
            }
        }

        Backend backend = new Backend(ir.getModule());
        RemovePhi removePhi = new RemovePhi();
        removePhi.run(backend.getModule());
        if (Config.emitWithoutPhi != null) {
            backend.emitBackend(Config.emitWithoutPhi);
        }
        if (Config.optLevel > 0) {
            // run backend optimization pass
            pass.Controller.runObjPass(backend.getModule());
        }
        if (Config.optLevel > 0) {
            new GraphColoringAlloc(backend.getModule()).run();
        } else {
            new FastLinearScannerAlloc(backend.getModule()).run();
        }
        backend.emitBackend(Config.output);
    }
}
