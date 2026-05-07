package cn.edu.bit.newnewcc;

import cn.edu.bit.newnewcc.backend.asm.AsmCode;
import cn.edu.bit.newnewcc.frontend.Preprocessor;
import cn.edu.bit.newnewcc.frontend.SysYExternalFunctions;
import cn.edu.bit.newnewcc.frontend.Translator;
import cn.edu.bit.newnewcc.frontend.antlr.SysYLexer;
import cn.edu.bit.newnewcc.frontend.antlr.SysYParser;
import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.pass.IrPassManager;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.tree.ParseTree;

import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class Driver {
    private final CompilerOptions compilerOptions;

    public Driver(CompilerOptions compilerOptions) {
        this.compilerOptions = compilerOptions;
    }

    public void launch() throws IOException {
        for (String inputFileName : compilerOptions.getInputFileNames()) {
            String code = Files.readString(Path.of(inputFileName));
            code = Preprocessor.preprocess(code);
            CharStream input = CharStreams.fromString(code);
            SysYLexer lexer = new SysYLexer(input);
            CommonTokenStream tokenStream = new CommonTokenStream(lexer);
            SysYParser parser = new SysYParser(tokenStream);
            ParseTree tree = parser.compilationUnit();
            Translator translator = new Translator();
            Module module = translator.translate(tree, SysYExternalFunctions.EXTERNAL_FUNCTIONS);

            // 在IR层面优化代码
            IrPassManager.optimize(module, compilerOptions.getOptimizationLevel());

            // 输出LLVM IR格式的文件
            if (compilerOptions.isEmitLLVM()) {
                String outputFileName = compilerOptions.getOutputFileName();
                if (outputFileName == null) outputFileName = changeExtension(inputFileName, "ll");

                try (var fileOutputStream = new FileOutputStream(outputFileName)) {
                    var builder = new StringBuilder();
                    module.emitIr(builder);
                    fileOutputStream.write(builder.toString().getBytes(StandardCharsets.UTF_8));
                }

                continue;
            }

            //测试汇编输出
            AsmCode asmCode = new AsmCode(module);

            if (compilerOptions.isEmitAssembly()) {
                String outputFileName = compilerOptions.getOutputFileName();
                if (outputFileName == null) outputFileName = changeExtension(inputFileName, "s");

                try (var fileOutputStream = new FileOutputStream(outputFileName)) {
                    fileOutputStream.write(asmCode.emit().getBytes(StandardCharsets.UTF_8));
                }

                continue;
            }

            String outputFileName = compilerOptions.getOutputFileName();
            if (outputFileName == null) outputFileName = changeExtension(inputFileName, "o");
            // TODO
        }
    }

    private static String changeExtension(String fileName, String newExtension) {
        if (fileName.contains("."))
            return fileName.replaceAll("\\.[^.]+$", "." + newExtension);
        else
            return fileName + "." + newExtension;
    }
}
