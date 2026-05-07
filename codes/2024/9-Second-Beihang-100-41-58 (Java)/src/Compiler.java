import Backend.component.AsmModule;
import Backend.operand.AllPhysicalReg;
import Backend.process.AsmPass;
import Backend.process.RegAllocator.Monitor;
import Backend.process.CodeGen;
import Backend.process.RemoveNoUseReg;
import Utils.RiscVBuilder;
import config.ArgParser;
import config.Config;
import frontend.AST.AST;
import frontend.Parser;
import frontend.Lexer;
import midend.Function;
import midend.Module;
import midend.Visitor;
import midend.optimism.Optimizer;

import java.io.IOException;

public class Compiler {
    public static void main(String[] args) throws IOException {
        ArgParser argParser = new ArgParser(args);
        argParser.parseArgs();
        AST compAST = new Parser(new Lexer()).parseAST();
        Visitor visitor = new Visitor();
        visitor.visitAST(compAST);
        Module module = visitor.getModule();
        module.outputLLVM(Config.llvmoutFile);
        Optimizer optimizer = new Optimizer(module);
        optimizer.run();
        module.outputLLVM(Config.llvmoptFile);
        module.setExitBlock();
        if (Config.BackEnd) {
            AllPhysicalReg.init();
            CodeGen codeGen = new CodeGen(module);
            AsmModule asmModule = codeGen.run();
            RemoveNoUseReg removeNoUseRegTool = new RemoveNoUseReg(asmModule);
            removeNoUseRegTool.run();
            Monitor regAllocaMonitor = new Monitor(asmModule);
            regAllocaMonitor.run();
            AsmPass asmPass = new AsmPass(asmModule);
            asmPass.run();
            RiscVBuilder dumpTool = new RiscVBuilder(Config.outputFile);
            dumpTool.dumpRISC_V(asmModule);
        }
    }
}
