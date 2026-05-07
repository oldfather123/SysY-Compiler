package Driver;

import Backend.Backend;
import Backend.process.RegAllo;

import Frontend.AST;
import Frontend.Lexer;
import Frontend.Parser;
import Frontend.TokenList;
import IR.IRModule;
import IR.Visitor;
import Pass.RemovePhi;
import Pass.PassManager;
import Utils.IRDump;
import Utils.RISC_Dump;

import java.io.IOException;

public class Driver {
    public void run(String[] args) throws IOException {
        //  解析参数
        parseArgs(args);
        //  开始编译流程
        TokenList tokenList = Lexer.getInstance().lex();
        AST compAST = new Parser(tokenList).parseAST();
        IRModule irModule = new Visitor().visitAST(compAST);

        if(Config.outputLLVM) {
            IRDump.DumpModule(irModule, "llvm_no_opt.ll");
        }
        PassManager passManager = PassManager.getInstance();
        passManager.runIRPasses(irModule);

        if(Config.outputLLVM) {
            IRDump.DumpModule(irModule, Config.iroutFile);
        }

        if(Config.noBackend){
            return;
        }
        new RemovePhi().run(irModule);

        if(Config.outputLLVM) {
            IRDump.DumpModule(irModule, "removed_phi.ll");
        }
        Backend backend = new Backend(irModule);

        if(Config.outputNoAlloc) {
            RISC_Dump.DumpBackend(backend, "riscv_withno_alloc.s");
        }

        RegAllo ar=new RegAllo(backend.getModule());
        ar.run();
        if(Config.outputNoAlloc) {
            RISC_Dump.DumpBackend(backend, "riscv_alloc_without_opt.s");
        }
        passManager.runObjPasses(backend.getModule());
        RISC_Dump.DumpBackend(backend,Config.outputFile);
    }

    private void parseArgs(String[] args){
        for(String arg : args){
            if(arg.equals("-debug")) {
                Config.outputReturn = true;
                Config.outputNoAlloc = true;
                Config.outputLLVM = true;
            }
            else if(arg.equals("-debug-llvm")){
                Config.outputReturn = true;
                Config.outputLLVM = true;
                Config.noBackend = true;
                Config.noRename = true;
            }
            else if(arg.equals("-test-llvm")){
                Config.outputReturn = true;
                Config.outputLLVM = true;
                Config.noBackend = true;
            }
            else if(arg.equals("-O1")) {
                Config.isO1 = true;
            }
            else if(arg.contains(".s") && !arg.contains(".sy")){
                Config.outputFile = arg;
            }
            else if(arg.contains(".sy")){
                Config.inputFile = arg;
            }
        }
    }
}
