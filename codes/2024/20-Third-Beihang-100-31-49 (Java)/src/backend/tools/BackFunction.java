package backend.tools;

import backend.instructions.BackInstruction;
import backend.operand.BackIReg;
import backend.operand.BackReg;
import mid.IntermediatePresentation.Function.Function;

import java.util.ArrayList;

public class BackFunction {
    private final Function midFunction;
    private String name;
    private ArrayList<BackBlock> backBlocks;
    private ArrayList<BackBlock> retBlocks;
    private int S = 0; //局部变量和寄存器
    private int A = 0; //(所有call最大参数个数-8) * 8
    private int R = 0; //是否有call
    private int curStack = 0;

    public BackFunction(Function f) {
        this.name = f.getName();
        backBlocks = new ArrayList<>();
        retBlocks = new ArrayList<>();
        midFunction = f;
    }

    public Function getMidFunction() {
        return midFunction;
    }

    public void addBackBlock(BackBlock backBlock) {
        this.backBlocks.add(backBlock);
    }

    public void addFirstInstruction(BackInstruction backInstruction) {
        backBlocks.get(0).addFirstInstruction(backInstruction);
    }

    public void addBefore(BackInstruction backInstruction) {
        for (BackBlock bb : retBlocks) {
            bb.addBefore(backInstruction);
        }
    }

    public void setR() {
        this.R = 8;
    }

    public String getName() {
        return name;
    }

    public void setCurStack(int number) {
        this.curStack = curStack + number;
    }

    public int getCurStack() {
        return curStack;
    }

    public Boolean hasCall() {
        if (R == 0) {
            return false;
        } else {
            return true;
        }
    }

    public void setS(int allocas) {
        S = S + allocas;
    }

    public int getStackSize() {
        int size = S + R + A * 8;
        if (size % 16 != 0) {
            size = ((size / 16) + 1) * 16; //向上取整到16的倍数
        }
        return size;
    }


    public void addRetBlocks(BackBlock bb) {
        this.retBlocks.add(bb);
    }

    public void setA(int params) {
        if (params - 8 > A) {
            A = params - 8;
        }
    }

    public int getA() {
        return A;
    }

    public ArrayList<BackBlock> getBackBlocks() {
        return backBlocks;
    }


    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (name.equals("main")) {
            sb.append(".section .text.startup\n");
            sb.append(".align 1\n");
            sb.append(".globl main\n");
        } else {
            sb.append(".section .text\n");
            sb.append(".align 1\n");
        }
        sb.append(name).append(":\n");
        for (BackBlock backBlock : backBlocks) {
            sb.append(backBlock.toString());
        }
        return sb.toString();
    }

}
