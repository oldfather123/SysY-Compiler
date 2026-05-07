package cn.edu.bit.newnewcc;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class Main {
    public static void main(String[] args) throws IOException {
        String outputFilename = null;
        int optimizationLevel = 0;
        boolean emitAssembly = false;
        boolean emitLLVM = false;
        List<String> inputFilenames = new ArrayList<>();
        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "-o" -> outputFilename = args[++i];
                case "-O0" -> optimizationLevel = 0;
                case "-O1", "-O2" -> optimizationLevel = 1;
                case "-S" -> emitAssembly = true;
                case "--emit-llvm" -> emitLLVM = true;
                default -> inputFilenames.add(args[i]);
            }
        }

        Driver driver = new Driver(
            new CompilerOptions(
                inputFilenames.toArray(new String[0]),
                outputFilename,
                optimizationLevel,
                emitAssembly,
                emitLLVM
            )
        );
        driver.launch();
    }
}
