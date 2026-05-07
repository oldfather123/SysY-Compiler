import backend.ir.entity.Program;
import backend.ir.handler.IRParser;
import backend.ir.optimize.IROptimize;
import backend.risc_v.handler.Translator;
import backend.risc_v.optimize.RiscVOptimize;
import frontend.lexer.handler.LexerAnalyzer;
import frontend.syntactic.entity.ast.CompileUnitNode;
import frontend.syntactic.handler.SyntacticParser;

import java.io.*;

public class Compiler {

    public static void main(String[] args) {
        boolean isCompetiton = true;
        boolean O1 = false;
        boolean isRISCV = true;
        String inFilePath = null;
        String OBJOutFilePath = null;
        String lexerOutFilePath = "lexer.txt";
        String syntacticOutFilePath = "syntactic.txt";
        String IROutFilePath = "llvm-ir.txt";
        String IROptimizeOutFilePath = "llvm-ir-optimize.txt";
        String CfgFilePath = "cfg.txt";
        String DominantTreeFilePath = "dominant-tree.txt";
        String ActiveVarAnalysisFilePath = "active-var-analysis.txt";
        String ActiveVarAnalysisInstFilePath = "active-var-analysis-inst.txt";
        String MoveFilePath = "move.txt";


        // 命令行参数解析
        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "-o":
                    if (i + 1 < args.length) OBJOutFilePath = args[++i];
                    break;
                case "-O1":
                    O1 = false;
                    break;
                case "--lexer":
                    if (i + 1 < args.length) lexerOutFilePath = args[++i];
                    break;
                case "--syntactic":
                    if (i + 1 < args.length) syntacticOutFilePath = args[++i];
                    break;
                case "--dt":
                    if(i + 1 < args.length) DominantTreeFilePath = args[++i];
                    break;
                case "--ava":
                    if(i + 1 < args.length) ActiveVarAnalysisFilePath = args[++i];
                    break;
                case "--avai":
                    if(i + 1 < args.length) ActiveVarAnalysisInstFilePath = args[++i];
                    break;
                case "--mv":
                    if(i + 1 < args.length) MoveFilePath = args[++i];
                    break;
                case "--cfg":
                    if(i + 1 < args.length) CfgFilePath = args[++i];
                    break;
                case "--ir":
                    if (i + 1 < args.length) IROutFilePath = args[++i];
                    break;
                case "--ir-opt":
                    if(i + 1 < args.length) IROptimizeOutFilePath = args[++i];
                    break;
                case "--is-not-com":
                    isCompetiton = false;
                    break;
                case "--is-not-riscv":
                    isRISCV = false;
                    break;
                default:
                    if(!args[i].startsWith("-S")) inFilePath = args[i];
                    break;
            }
        }

        if(!isCompetiton){
            lexerOutFilePath = "./txt/" + lexerOutFilePath;
            syntacticOutFilePath = "./txt/" + syntacticOutFilePath;
            CfgFilePath = "./txt/" + CfgFilePath;
            DominantTreeFilePath = "./txt/" + DominantTreeFilePath;
            ActiveVarAnalysisFilePath = "./txt/" + ActiveVarAnalysisFilePath;
            ActiveVarAnalysisInstFilePath = "./txt/" + ActiveVarAnalysisInstFilePath;
            MoveFilePath = "./txt/" + MoveFilePath;
        }

        if (inFilePath == null) {
            System.err.println("缺少输入文件名");
            return;
        }
        if (OBJOutFilePath == null)
            OBJOutFilePath = "risc-v.s";

        try {
            BufferedReader lexerReader = new BufferedReader(new FileReader(inFilePath));
            BufferedWriter lexerWriter = new BufferedWriter(new FileWriter(lexerOutFilePath));
            BufferedWriter syntacticWriter = new BufferedWriter(new FileWriter(syntacticOutFilePath));
            BufferedWriter IRWriter = new BufferedWriter(new FileWriter(IROutFilePath));
            BufferedWriter IROptWriter = new BufferedWriter(new FileWriter(IROptimizeOutFilePath));
            BufferedWriter cfgWriter = new BufferedWriter(new FileWriter(CfgFilePath));
            BufferedWriter DominantTreeWriter = new BufferedWriter(new FileWriter(DominantTreeFilePath));
            BufferedWriter ActiveVarAnalysisWriter = new BufferedWriter(new FileWriter(ActiveVarAnalysisFilePath));
            BufferedWriter ActiveVarAnalysisInstWriter = new BufferedWriter(new FileWriter(ActiveVarAnalysisInstFilePath));
            BufferedWriter MoveWriter = new BufferedWriter(new FileWriter(MoveFilePath));

            LexerAnalyzer lexerAnalyzer = new LexerAnalyzer(lexerReader, lexerWriter);
            lexerAnalyzer.WordAnalyze();
            lexerAnalyzer.WordListPrintText();
            lexerReader.close();
            lexerWriter.close();

            SyntacticParser syntacticParser = new SyntacticParser(lexerAnalyzer, syntacticWriter);
            syntacticParser.syntacticAnalyze();
            CompileUnitNode compileUnit = syntacticParser.astEntry;

            compileUnit.syntacticDebug(syntacticWriter);
            syntacticWriter.close();

            IRParser irParser = new IRParser();
            Program program = irParser.getProgram(compileUnit);
            program.printAssembly(IRWriter);
            IRWriter.close();

            if(O1){
                //中端优化
                IROptimize irOptimize = new IROptimize(program);
                irOptimize.setOptimize(true);
                irOptimize.runOptimize();
                //输出控制流图
                program.printCFG(cfgWriter);
                cfgWriter.close();
                //输出支配树
                program.printDominantTree(DominantTreeWriter);
                DominantTreeWriter.close();
                //输出活跃变量分析
                program.printActiveVarAnalysis(ActiveVarAnalysisWriter);
                program.printActiveVarAnalysisInst(ActiveVarAnalysisInstWriter);
                ActiveVarAnalysisWriter.close();
                ActiveVarAnalysisInstWriter.close();
                //打印优化的中间代码
                program.printAssembly(IROptWriter);
                IROptWriter.close();

                //进行后端优化
                RiscVOptimize riscVOptimize = new RiscVOptimize(program);
                riscVOptimize.setOptimize(true);
                riscVOptimize.runOptimize();
                //输出消除phi之后的中间代码文件
                program.printAssembly(MoveWriter);
                MoveWriter.close();
            }
            IROptWriter.close();
            cfgWriter.close();
            DominantTreeWriter.close();
            ActiveVarAnalysisWriter.close();
            ActiveVarAnalysisInstWriter.close();
            MoveWriter.close();

            BufferedWriter OBJWriter = new BufferedWriter(new FileWriter(OBJOutFilePath));
            if(isRISCV){
                Translator translator = new Translator();
                translator.translate(program);
                translator.printRiscV(OBJWriter);
            }
            OBJWriter.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}