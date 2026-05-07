import IR.IRModule;
import antlr.SysYLexer;
import antlr.SysYParser;
import backend.MemoryRegisterAlloc;
import backend.RISCVBuilder;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import java.io.IOException;


import org.antlr.v4.runtime.tree.ParseTree;

public class Compiler {
    public static void main(String[] args) throws IOException{
        //    功能测试：compiler -S -o testcase.s testcase.sy
        //    性能测试：compiler -S -o testcase.s testcase.sy -O1
        if (args.length < 1) {
            System.err.println("input path is required");
            return;
        }
        String source = args[3];//源文件路径
        String Dest = args[2];//目标文件路径

        //获得输入
        CharStream input = CharStreams.fromFileName(source);
        //词法分析
        SysYLexer sysYLexer = new SysYLexer(input);
        CommonTokenStream tokens_input = new CommonTokenStream(sysYLexer);
        //语法分析+语义分析
        SysYParser sysYParser = new SysYParser(tokens_input);
        ParseTree parseTree = sysYParser.program();
        //生成中间代码
        IRVisitor irVisitor = new IRVisitor(source);
        irVisitor.visit(parseTree);
        //将生成的中间代码写入文件
        IRModule.IRPrintModuleToFile(irVisitor.getModule(), "./tests/test1.ll");
        //生成汇编代码
        MemoryRegisterAlloc registerAlloc = new MemoryRegisterAlloc();
        RISCVBuilder riscvBuilder = new RISCVBuilder(irVisitor.getModule(), registerAlloc);
        riscvBuilder.generateASM(Dest);
    }
}