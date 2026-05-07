package IO;

import backend.asm.instr.ASMInstruction;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;
import backend.asm.structure.ASMGlobalObject;
import backend.asm.structure.ASMModel;
import backend.opt.riscv.ParallelLib;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.IRModel;
import frontend.ir.symbol.StringLiteral;
import frontend.ir.symbol.SymTab;
import frontend.ir.symbol.Symbol;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.List;

public class OutputHandler {
    private static final OutputHandler OUTPUT_HANDLER = new OutputHandler();

    private OutputHandler() {
    }

    public static OutputHandler getInstance() {
        return OUTPUT_HANDLER;
    }

    private BufferedWriter openBufferedWriter(FileWriter fileWriter) {
        BufferedWriter bufferedWriter;
        try {
            bufferedWriter = new BufferedWriter(fileWriter);
            bufferedWriter.write("");
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return bufferedWriter;
    }

    private void appendBufferedWriter(BufferedWriter bufferedWriter, String s) {
        try {
            bufferedWriter.append(s);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private void closeBufferedWriter(BufferedWriter bufferedWriter) {
        try {
            bufferedWriter.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    // 中间代码生成
    public void printIR(FileWriter llvmIRWriter, IRModel irModel) {
        List<Function> functions = irModel.getFunctions();
        SymTab globalSymTab = irModel.getGlobalSymTab();

        BufferedWriter irWriter = openBufferedWriter(llvmIRWriter);

        printLibFuncDecl(irWriter);
        for (Symbol object : globalSymTab.getObjectList()) {
            if (object.isAbandoned()) {
                continue;
            }
            printIRGlobalObject(irWriter, object);
        }
        for (Function function : functions) {
            printIRFunction(irWriter, function);
        }

        closeBufferedWriter(irWriter);
    }

    private void printIRFunction(BufferedWriter irWriter, Function function) {
        appendBufferedWriter(irWriter, "\n");
        appendBufferedWriter(irWriter, "define dso_local " + function.getType().printIRType());
        appendBufferedWriter(irWriter, " @" + function.getName() + "(");
        printFParams(irWriter, function.getFParams());
        appendBufferedWriter(irWriter, ") {\n");
        printBasicBlocks(irWriter, function.getBasicBlockList());
        appendBufferedWriter(irWriter, "}\n");
    }

    private void printBasicBlocks(BufferedWriter irWriter, List<BasicBlock> basicBlockList) {
        for (BasicBlock basicBlock : basicBlockList) {
            appendBufferedWriter(irWriter, basicBlock.value2string() + ":\n");
            for (Instruction ins : basicBlock.getInstrList()) {
                appendBufferedWriter(irWriter, "\t" + ins.toString() + "\n");
            }
        }
    }

    private void printFParams(BufferedWriter irWriter, List<Function.FParam> fParams) {
        for (int i = 0; i < fParams.size(); i++) {
            Function.FParam fParam = fParams.get(i);
            appendBufferedWriter(irWriter, fParam.toString());
            if (i < fParams.size() - 1) {
                appendBufferedWriter(irWriter, ", ");
            }
        }
    }

    private void printIRGlobalObject(BufferedWriter irWriter, Symbol object) {
        boolean isStr = object.getInitVal() instanceof StringLiteral;
        appendBufferedWriter(irWriter, "@" + object.getIdent() + " = ");
        String s;
        if (isStr) {
            s = object.getInitVal().toString();
        } else {
            s = "dso_local global " + object.getInitVal().toString();
        }
        appendBufferedWriter(irWriter, s + "\n");
    }

    private void printLibFuncDecl(BufferedWriter irWriter) {
        for (Function.LibFunc libFunc : Function.LibFunc.ALL_LIB_FUNC) {
            appendBufferedWriter(irWriter, libFunc.getDecl() + "\n");
        }
        appendBufferedWriter(irWriter, "\n");
    }

    // 目标代码
    public void printASM(FileWriter asmWriter, ASMModel asmModel, boolean isRV) {
        BufferedWriter asmBufferedWriter = openBufferedWriter(asmWriter);

//        appendBufferedWriter(asmBufferedWriter, ".data\n");
        if (isRV) {
            appendBufferedWriter(asmBufferedWriter, ".option nopic\n");
            appendBufferedWriter(asmBufferedWriter, ".attribute arch, \"rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zba1p0_zbb1p0\"\n");
            appendBufferedWriter(asmBufferedWriter, ".attribute unaligned_access, 0\n");
            if (ParallelLib.need(asmModel)) {
                appendBufferedWriter(asmBufferedWriter, ParallelLib.model);
            }
        }
        for (ASMGlobalObject globalObject : asmModel.getGlobalObjectlist()) {
            appendBufferedWriter(asmBufferedWriter, globalObject.toString() + "\n");
        }

        for (ASMFunction function : asmModel.getFunctionList()) {
            if (function.isMain()) {
                appendBufferedWriter(asmBufferedWriter, ".section\t.text.startup\n");
                appendBufferedWriter(asmBufferedWriter, ".align\t1\n");
                appendBufferedWriter(asmBufferedWriter, ".globl\tmain\n");
            } else {
                appendBufferedWriter(asmBufferedWriter, "\n.section\t.text\n");
                appendBufferedWriter(asmBufferedWriter, ".align\t1\n");
            }
            appendBufferedWriter(asmBufferedWriter, function + ":\n");
            ASMBasicBlock basicBlock = (ASMBasicBlock) function.getBasicBlockList().getHead();
            while (basicBlock != null) {
                appendBufferedWriter(asmBufferedWriter, "\t" + basicBlock + ":\n");
                ASMInstruction ins = (ASMInstruction) basicBlock.getInstructionList().getHead();
                while (ins != null) {
                    appendBufferedWriter(asmBufferedWriter, "\t\t" + ins + "\n");
                    ins = (ASMInstruction) ins.getNext();
                }
                basicBlock = (ASMBasicBlock) basicBlock.getNext();
            }
        }

        closeBufferedWriter(asmBufferedWriter);
    }

    public void printRVPlaceholder(FileWriter asmWriter) {
        BufferedWriter asmBufferedWriter = openBufferedWriter(asmWriter);
        appendBufferedWriter(asmBufferedWriter, ".option nopic\n" +
                ".attribute arch, \"rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zba1p0_zbb1p0\"\n" +
                ".attribute unaligned_access, 0\n" +
                "\n" +
                ".section\t.text.startup\n" +
                ".align\t1\n" +
                ".globl\tmain\n" +
                "main:\n" +
                "\tmain_head_blk_2:\n" +
                "\t\taddi sp, sp, -16\n" +
                "\tmain_blk_1:\n" +
                "\t\tmv a0, zero\n" +
                "\t\taddi sp, sp, 16\n" +
                "\t\tjr ra\n");

        closeBufferedWriter(asmBufferedWriter);
    }
}
