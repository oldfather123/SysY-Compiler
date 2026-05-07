package backend;

import Utils.CustomList;
import backend.itemStructure.AsmBlock;
import backend.itemStructure.AsmFunction;
import backend.itemStructure.AsmGlobalVar;
import backend.itemStructure.AsmModule;
import backend.regs.RegGeter;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;

public class BackendPrinter {
    private AsmModule module;
    private boolean withAlloc = false;
    private BufferedWriter writer = null;

    public BackendPrinter(AsmModule module, boolean withAlloc) {
        this.module = module;
        this.withAlloc = withAlloc;
    }

    public BackendPrinter(AsmModule module, boolean withAlloc, BufferedWriter writer) {
        this.module = module;
        this.withAlloc = withAlloc;
        this.writer = writer;
    }

    public void printBackend() throws IOException {
        BufferedWriter writer;
        if (this.writer != null) {
            writer = this.writer;
        } else if (withAlloc) {
            writer = new BufferedWriter(new FileWriter("output.s"));
        } else {
            writer = new BufferedWriter(new FileWriter("output_no_alloc.s"));
        }

        //打印全局变量
        ArrayList<AsmGlobalVar> globalVars = module.getGlobalVars();
        for (AsmGlobalVar globalVar : globalVars) {
            writer.write(globalVar.toString());
            writer.newLine();
        }
        //打印函数
        ArrayList<AsmFunction> functions = module.getFunctions();
        for (AsmFunction function : functions) {
            if (function.getName().equals("main")) {
                writer.write(".section\t.text.startup");
                writer.newLine();
                writer.write(".align\t1");
                writer.newLine();
                writer.write(".globl\tmain");
                writer.newLine();
            } else {
                writer.write(".section\t.text");
                writer.newLine();
                writer.write(".align\t1");
                writer.newLine();
            }
            writer.write(function.getName() + ":");
            writer.newLine();
//            if(function.getName().equals("main")){
//                //所有寄存器全部初始化为0
//                for (int i = 5; i < 32; i++) {
//                    writer.write("li\t" + RegGeter.AllRegsInt.get(i).toString() + ",\t0");
//                    writer.newLine();
//                }
//                writer.newLine();
//            }
            CustomList blocks = function.getBlocks();
            for (CustomList.Node node : blocks) {
                AsmBlock block = (AsmBlock) node;
                writer.write(block.getName() + ":");
                writer.newLine();
                for (CustomList.Node instrNode : block.getInstrs()) {
                    writer.write("\t" + instrNode.toString());
                    writer.newLine();
                }
            }
        }
        writer.close();
    }
}
