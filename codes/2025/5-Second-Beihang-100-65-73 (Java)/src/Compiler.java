import IO.OutputHandler;
import IO.Arg;
import backend.asm.IRParser4ARM;
import backend.asm.IRParser4RV;
import backend.asm.PostProcess;
import backend.asm.insfact.InsFact4ARM;
import backend.asm.insfact.InsFact4RV;
import backend.asm.insfact.InstrFactory;
import backend.asm.regalloc.RegAllocator;
import backend.asm.register.store.ArmRegStore;
import backend.asm.register.store.RegStore;
import backend.asm.register.store.RvRegStore;
import backend.asm.structure.ASMModel;
import backend.asm.IRParser;
import backend.opt.Peephole;
import backend.opt.arm.InsSelect4ARM;
import backend.opt.riscv.LoadFloatFromPool;
import frontend.ir.structure.Function;
import frontend.ir.structure.IRModel;
import frontend.ir.Visitor;
import frontend.lexer.Lexer;
import frontend.lexer.TokenList;
import frontend.syntax.SyntaxParser;
import frontend.syntax.ast.AST;
import midend.AdjustUnrollToGep;
import midend.DetectTailRecursive;
import midend.OptimizationHandler;
import midend.RemovePhi;

import java.io.*;
import java.util.List;

public class Compiler {
    public static void main(String[] args) throws IOException {
        Arg.parse(args);
        BufferedInputStream sourceStream = new BufferedInputStream(Arg.ARG.getSrcStream());
        final boolean opt = Arg.ARG.getOptLevel() > 0;
        final boolean skipBackEnd = Arg.ARG.toSkipBackEnd();
        final boolean runARM = !skipBackEnd && Arg.ARG.getFramework() == Arg.Framework.ARM;
        final boolean runRV = !skipBackEnd && Arg.ARG.getFramework() == Arg.Framework.RV;

        TokenList tokenList = Lexer.getInstance().analyze(sourceStream);
        AST abstractSyntaxTree = SyntaxParser.getInstance().parse(tokenList);
        IRModel irModel = Visitor.getInstance().visitAST(abstractSyntaxTree);


        // 要做优化啦
        OptimizationHandler.getInstance().execute(irModel, opt, runARM, runRV);

        if (Arg.ARG.toPrintIR()) {
            OutputHandler.getInstance().printIR(Arg.ARG.getIrWriter(), irModel);
        }

        if (skipBackEnd) {
            return;
        }

        List<Function> functions = irModel.getFunctions();

        RemovePhi.execute(functions);
        DetectTailRecursive.excute(functions);
        AdjustUnrollToGep.execute(functions);

        IRParser irParser;
        RegStore regStore;
        InstrFactory instrFactory;
        if (runRV) {
            irParser = IRParser4RV.getInstance();
            regStore = RvRegStore.getInstance();
            instrFactory = InsFact4RV.getInstance();
        } else {
            irParser = IRParser4ARM.getInstance();
            regStore = ArmRegStore.getInstance();
            instrFactory = InsFact4ARM.getInstance();
        }

        ASMModel asmModel = irParser.parseIR(irModel, regStore);

        Peephole.execute4vir(asmModel, regStore, instrFactory);

        if (runARM) {
            InsSelect4ARM.execute4vir(asmModel, regStore, opt);
        }

        LoadFloatFromPool.execute(asmModel);

        Peephole.execute4virAfterInsSel(asmModel, regStore, instrFactory);

        RegAllocator.getInstance().allocRegister(asmModel, regStore, instrFactory);

        Peephole.execute4phy(asmModel, regStore, instrFactory, runRV);

        PostProcess.getInstance().execute(asmModel, regStore, instrFactory);

        if (runARM) {
            InsSelect4ARM.execute4phy(asmModel);
        }

//        if (runRV) {
//            InsSelect4RV.execute4phy(asmModel, regStore);
//        }

        OutputHandler.getInstance().printASM(Arg.ARG.getAsmWriter(), asmModel, runRV);
    }
}