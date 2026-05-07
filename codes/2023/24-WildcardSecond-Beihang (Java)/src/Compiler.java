import backend.CodeGen;
import backend.RegAllocate.RegAllocator;
import backend.component.RiscvModule;
import backend.opt.BackLoopLiftOut;
import backend.opt.BackPeepHole;
import ir.pass.analyze.RemoveUselessUser;
import ir.pass.opt.*;
import ir.value.Module;
import org.antlr.v4.runtime.tree.ParseTree;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import frontend.*;
import tools.*;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Date;

public class Compiler {
    public static long time0 = 0;
    public static boolean noCrossArray = true;

    public static double gettime() {
        long timenow = System.currentTimeMillis();
        double returntime = (timenow - time0) / 1000.0;
        time0 = timenow;
        return returntime;
    }

    public static void main(String[] args) throws IOException, CloneNotSupportedException {
        Module module = null;
        MyVisitor visitor = null;
        String inputPath = null;
        String outputPath = null;
        for (int i = 0; i < args.length; i++) {
            if (args[i].startsWith("-o")) {
                outputPath = args[i + 1];
                inputPath = args[i + 2];
            }
        }
        try {
            String input = new String(Files.readAllBytes(Paths.get(inputPath == null ?
                    "./src/input.txt" : inputPath)));//输入文件
            gettime();
            CharStream inputStream = CharStreams.fromString(input);
            sysyLexer lexer = new sysyLexer(inputStream);
            CommonTokenStream tokenStream = new CommonTokenStream(lexer);
            sysyParser parser = new sysyParser(tokenStream);
            ParseTree tree = parser.compUnit();
            visitor = new MyVisitor();
            visitor.visit(tree);
            module = visitor.module;
        } catch (Exception e) {
            System.exit(2);
        }
        try {
            ModuleFlattener.flattenModule(module);
            System.out.printf("前端耗时：%f s%n", gettime());
            //BlockAnalyzer.updateBlockPredAndSuccs(module);
            BrRemoving brRemoving = new BrRemoving();
            brRemoving.run(module);
            try (FileWriter fileWriter = new FileWriter("./llvmIR1.ll")) {
                fileWriter.append(visitor.module.toString());
            }
//        GVN gvn = new GVN();
//        gvn.run(module);
//        ArrayEliminate eliminator2 = new ArrayEliminate();
//        eliminator2.run(module);
            DeadCodeEmit deadCodeEmit = new DeadCodeEmit();
            deadCodeEmit.runWeak(module);
            deadCodeEmit.runWeak(module);
            deadCodeEmit.runWeak(module);
            System.out.printf("第一遍死代码：%f s%n", gettime());
        }catch (Exception e){
            System.exit(3);
        }
        try{
            new DeadCodeEmit().run(module);
            RemoveUselessUser.run(module);
            GlobalVariableLocalize global2local = new GlobalVariableLocalize();
            global2local.run(module);
            ModuleFlattener.flattenModule(module);
            MemToReg memToReg = new MemToReg();
            memToReg.run(module);

            ModuleFlattener.flattenModule(module);
            GVN gvn2 = new GVN();
            gvn2.run(module);
            System.out.printf("GVN：%f s%n", gettime());
            ModuleFlattener.flattenModule(module);
            try (FileWriter fileWriter = new FileWriter("./llvmIRAfterGVN.ll")) {
                fileWriter.append(visitor.module.toString());
            }
            GCM gcm = new GCM();
            gcm.run(module);
            ModuleFlattener.flattenModule(module);
            try (FileWriter fileWriter = new FileWriter("./llvmIRPreRFE.ll")) {
                fileWriter.append(visitor.module.toString());
            }
            new RFE().run(module);
        }catch (Exception e){
            System.exit(4);
        }

        try (FileWriter fileWriter = new FileWriter("./llvmIRPreCRP.ll")) {
            fileWriter.append(visitor.module.toString());
        }
            CRP crp = new CRP();
            crp.run(module);
            ModuleFlattener.flattenModule(module);
            try (FileWriter fileWriter = new FileWriter("./llvmIRAfterCRP.ll")) {
                fileWriter.append(visitor.module.toString());
            }try (FileWriter fileWriter = new FileWriter("./llvmIRPreInlineFunction.ll")) {
                fileWriter.append(visitor.module.toString());
            }
            InlineFunction inlineFunction = new InlineFunction();
            inlineFunction.run(module, visitor);
            ModuleFlattener.flattenModule(module);
            try (FileWriter fileWriter = new FileWriter("./llvmIRAfterInlineFunction.ll")) {
                fileWriter.append(visitor.module.toString());
            }

            DeadCodeEmit deadCodeEmitAfterMem2reg = new DeadCodeEmit();
            deadCodeEmitAfterMem2reg.run(module);
            new BrRemoving().run(module);
            ModuleFlattener.flattenModule(module);
            System.out.printf("mem2reg+DCE+MF：%f s%n", gettime());
            try (FileWriter fileWriter = new FileWriter("./llvmIRPreGVN.ll")) {
                fileWriter.append(visitor.module.toString());
            }

        boolean needRepeat = true;
        while (needRepeat) {
            needRepeat = false;
            try{
                MemsetEliminate.run(module);
                GVN gvn2 = new GVN();
                gvn2.run(module);
                System.out.printf("GVN：%f s%n", gettime());
                ModuleFlattener.flattenModule(module);
                try (FileWriter fileWriter = new FileWriter("./llvmIRAfterGVN.ll")) {
                    fileWriter.append(visitor.module.toString());
                }
                GCM gcm = new GCM();
                gcm.run(module);
            }catch (Exception e){
                System.exit(7);
            }
            try{
                ModuleFlattener.flattenModule(module);
                System.out.printf("GCM+MF：%f s%n", gettime());
                try (FileWriter fileWriter = new FileWriter("./llvmIR.ll")) {
                    fileWriter.append(visitor.module.toString());
                }
                if(Debugger.noCrossArray){
                    ArrayEliminate eliminator = new ArrayEliminate();
                    needRepeat = eliminator.run(module);
                    needRepeat = LSGVN.run(module) || needRepeat;
                    if (!needRepeat) {
                        LSGCM lsgcm = new LSGCM();
                        needRepeat = lsgcm.run(module);
                    }
                }
                if(!needRepeat){
                    needRepeat = LoopCompute.run(module);
                }
                DeadCodeEmit deadCodeEmit1 = new DeadCodeEmit();
                deadCodeEmit1.runStrong(module);
                BrRemoving brRemoving2 = new BrRemoving();
                brRemoving2.run(module);
            }catch (Exception e){
                System.exit(8);
            }
        }
        try{
            DeadCodeEmit deadCodeEmitAfterGCM = new DeadCodeEmit();
            deadCodeEmitAfterGCM.runStrong(module);
            RemovePhi removePhi = new RemovePhi();
            removePhi.run(module);
            System.out.printf("removePHI：%f s%n", gettime());
            try (FileWriter fileWriter = new FileWriter("./llvmIRRemovePhi0.ll")) {
                fileWriter.append(visitor.module.toString());
            }
            BrRemoving brRemoving2 = new BrRemoving();
            brRemoving2.runStrong(module);
            try (FileWriter fileWriter = new FileWriter("./llvmIRRemovePhi.ll")) {
                fileWriter.append(visitor.module.toString());
            }
            Debugger debugger = new Debugger();
            debugger.run(module);
        }catch (Exception e){
            System.err.println(e);
            e.printStackTrace();
            System.exit(9);
        }
        RiscvModule rvModule = null;
        try{
            rvModule = new RiscvModule();
            CodeGen codeGen = new CodeGen(module, rvModule);
            codeGen.run();
            try (FileWriter fileWriter = new FileWriter(outputPath == null ? "./rv.txt" : outputPath)) {
                fileWriter.append(rvModule.toString());
            }
        }catch (Exception e){
            System.exit(10);
        }

//            if(input.contains("a[4][19999] = 1;")){
//                return;
//            }
            BackLoopLiftOut liftOut = new BackLoopLiftOut(rvModule, module);
            try (FileWriter fileWriter = new FileWriter(outputPath == null ? "./liftOut.txt" : outputPath)) {
                liftOut.process();
                fileWriter.append(rvModule.toString());
            }
        BackPeepHole backPeepHole = null;
        try{
            backPeepHole = new BackPeepHole(rvModule);
            try (FileWriter fileWriter = new FileWriter(outputPath == null ? "./afterPeelHoleDataFlow.txt" : outputPath)) {
                backPeepHole.DataFlowProcess();
                fileWriter.append(rvModule.toString());
            }
        }catch (Exception e){
            System.exit(11);
        }
        try {
            RegAllocator regAllocator = new RegAllocator(rvModule);
            regAllocator.Process();
            try (FileWriter fileWriter = new FileWriter(
                outputPath == null ? "./prePeepres.s" : outputPath)) {
                fileWriter.append(rvModule.toString());
            }
            backPeepHole.process();
        }catch (Exception e){
            System.exit(12);
        }

        try (FileWriter fileWriter = new FileWriter(outputPath == null ? "./res.s" : outputPath)) {
            fileWriter.append(rvModule.toString());
        }
    }
}
