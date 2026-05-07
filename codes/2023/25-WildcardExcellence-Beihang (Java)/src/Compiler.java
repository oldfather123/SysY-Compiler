import arg.Arg;
import frontend.SysYLexer;
import frontend.SysYParser;
import frontend.Visitor;
import midend.MyModule;
import midend.value.Function;
import midpass.*;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.tree.ParseTree;
import riscv.AsmGen;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;

public class Compiler {
    public static void main(String[] args) throws IOException {
        Arg arg = new Arg(args);

        CharStream input = CharStreams.fromStream(getFileStream(arg.getInputFilePath()));
        SysYLexer lexer = new SysYLexer(input);
        CommonTokenStream tokenStream = new CommonTokenStream(lexer);
        SysYParser parser = new SysYParser(tokenStream);
        ParseTree tree = parser.compUnit();
        Visitor visitor = new Visitor();
        visitor.visit(tree);
        ArrayList<Function> functions = MyModule.myModule.getDefineFunc();
        new ConstantFold(functions);

        if (arg.isSpeed()) {
            // 性能模式
            new MidPassManager(functions);
        }

        MyModule.myModule.toMidOut(functions, "llvm.ll");

        String outputFile = arg.getOutputFilePath();
        // 只输出 .ll 文件
        if (arg.isOnlyLLVM()) {
            MyModule.myModule.toMidOut(functions, outputFile);
        }
        // 输出汇编
        else {
            AsmGen gen = new AsmGen(MyModule.myModule);
            gen.generateRiscV(outputFile);
        }
    }

    private static InputStream getFileStream(String path) throws IOException {
        Path filePath = Path.of(path);
        byte[] fileBytes = Files.readAllBytes(filePath);
        return new ByteArrayInputStream(fileBytes);
    }
}
