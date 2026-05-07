package Utils;

import Backend.component.AsmBlock;
import Backend.component.AsmFunction;
import Backend.component.AsmGlobalVariable;
import Backend.component.AsmModule;
import Backend.component.AsmInstr;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedList;

public class RiscVBuilder {
    private static final String SECTION_TEXT_STARTUP = ".section\t.text.startup";
    private static final String SECTION_TEXT = ".section\t.text";
    private static final String ALIGN = ".align\t1";
    private static final String GLOBAL = ".globl\t";
    private static final String FUNCTION_LABEL = "\n%s:\n";

    private BufferedWriter bufferedWriter;

    public RiscVBuilder(String filename) {
        try {
            bufferedWriter = new BufferedWriter(new FileWriter(filename));
        } catch (IOException e) {
            System.err.println("Error opening file: " + e.getMessage());
        }
    }

    public void dumpRISC_V(AsmModule asmModule) {
        try {
            dealWithModule(asmModule);
        } catch (IOException e) {
            System.err.println("Error processing module: " + e.getMessage());
        } finally {
            try {
                bufferedWriter.close();
            } catch (IOException e) {
                System.err.println("Error closing file: " + e.getMessage());
            }
        }
    }

    private void dealWithModule(AsmModule asmModule) throws IOException {
        writeGlobalVariables(asmModule.getGlobalVariables());
        writeFunctions(asmModule.getFunctions());
    }

    private void writeGlobalVariables(ArrayList<AsmGlobalVariable> globalVariables) throws IOException {
        for (AsmGlobalVariable var : globalVariables) {
            bufferedWriter.write(var.toString() + "\n");
        }
    }

    private void writeFunctions(ArrayList<AsmFunction> functions) throws IOException {
        for (AsmFunction func : functions) {
            dealWithFunction(func);
        }
    }

    private void dealWithFunction(AsmFunction objFunction) throws IOException {
        if (objFunction.getName().equals("main")) {
            bufferedWriter.write(SECTION_TEXT_STARTUP);
            bufferedWriter.newLine();
            bufferedWriter.write(ALIGN);
            bufferedWriter.newLine();
            bufferedWriter.write(GLOBAL + objFunction.getName());
        } else {
            bufferedWriter.write(SECTION_TEXT);
            bufferedWriter.newLine();
            bufferedWriter.write(ALIGN);
            bufferedWriter.newLine();
        }
        bufferedWriter.write(String.format(FUNCTION_LABEL, objFunction.getName()));
        writeBlocks(objFunction.getObjBlocks());
    }

    private void writeBlocks(LinkedList<AsmBlock> blocks) throws IOException {
        for (AsmBlock block : blocks) {
            dealWithBlock(block);
        }
    }

    private void dealWithBlock(AsmBlock asmBlock) throws IOException {
        bufferedWriter.write(asmBlock.getName() + ":\n");
        writeInstructions(asmBlock.getInstrList());
    }

    private void writeInstructions(LinkedList<AsmInstr> instructions) throws IOException {
        for (AsmInstr instr : instructions) {
            bufferedWriter.write("\t" + instr.toString() + "\n");
        }
    }
}