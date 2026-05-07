package utils;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PushbackReader;

import static java.lang.System.exit;

public class Runner {
    public void run(String[] args) throws IOException {
        for (int i = 0; i < args.length; i++) {
            String arg = args[i];
            if (arg.equals("-O1")) {
                utils.Config.opt = true;
            } else if (arg.contains("-output_llvm")) {
                utils.Config.output_llvm = true;
            } else if (arg.equals("-o")) {
                utils.Config.output = args[i + 1];
                i++;
            } else if (arg.equals("-S")) {
                continue;
            } else {
                utils.Config.input = arg;
            }
        }

        File inFile = new File(utils.Config.input);
        PushbackReader inputStream = new PushbackReader(new FileReader(inFile));

        utils.tools.Timer.INSTANCE.start();

        // front
        front.lexer.Lexer lexer = new front.lexer.Lexer(inputStream);
        front.parser.Parser parser = new front.parser.Parser(lexer.getTokenStream());
        front.AST.Node compUnit = parser.parseCompUnit();

        // mid
        compUnit.toIR();
        mid.IntermediatePresentation.Module module = mid.IntermediatePresentation.IRManager.getModule();
        output("ir_un_opt.ll", module);
        System.out.println("opt");
        // mid optimize
        if (utils.Config.opt) {
//            try {
                mid.Optimizer.Optimizer.instance().optimize();
//            } catch (Exception e) {
//                e.printStackTrace();
//                try {
//                    output("if err.ll", mid.IntermediatePresentation.IRManager.getModule());
//                } catch (Exception oe) {
//                    System.out.println("output failed");
//                }
//                exit(-1);
//            }
        } else if (module.getDecledFunctions().stream().allMatch(f -> f.getBlocks().size() <= 3000)) {
            // 否则会tle...
            mid.Optimizer.Optimizer.instance().backendBasicOpt();
        } else {
            mid.Optimizer.Optimizer.instance().mem2regOnly();
        }
        System.out.println("back");
        backend.Backend backend = new backend.Backend(module);
        backend.tools.BackModule backModule = backend.run();
        System.out.println("opt");
        if (utils.Config.opt) {
            backModule.optimize();
        }
        outputBack(utils.Config.output, backModule);
    }

    public static void output(String path, Object o) {
        if (o.equals(mid.IntermediatePresentation.IRManager.getModule()) && !utils.Config.output_llvm) {
            return;
        }
        try {
            BufferedWriter bw = new BufferedWriter(new FileWriter(path));
            bw.write(o.toString());
            bw.flush();
            bw.close();
        } catch (IOException e) {
            System.out.println("output failed");
        }
    }

    public static void outputBack(String path, Object o) throws IOException {
        BufferedWriter bw = new BufferedWriter(new FileWriter(path));
        bw.write(o.toString());
        bw.flush();
        bw.close();
    }
}
