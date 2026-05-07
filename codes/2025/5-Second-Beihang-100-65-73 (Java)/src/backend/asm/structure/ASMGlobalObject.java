package backend.asm.structure;

import java.util.ArrayList;
import java.util.List;

public class ASMGlobalObject {
    private final String labelName;
    private final List<ASMGlbInitVal> initValues;
    private final String initString;
    private int size; // 字节数
    private static int LC_CNT = 0;
    private final boolean isFloat; // 是否为浮点数

    public ASMGlobalObject(String name, List<ASMGlbInitVal> initValues, boolean isFloat) {
        this.labelName = "glb_obj_" + name;
        this.initValues = mergeZero(initValues);
        this.initString = null;
        this.isFloat = isFloat;

        int tmpSize = 0;
        for (ASMGlbInitVal initVal : this.initValues) {
            if (initVal instanceof ASMGlbInitVal.ASMGlbZero zero) {
                tmpSize += zero.getNumber();
            } else {
                tmpSize += 1;
            }
        }
        this.size = tmpSize * 4;
    }

    public ASMGlobalObject(String name, String irInitString, boolean isFloat) {
        this.labelName = "glb_obj_" + name;
        this.initValues = null;
        this.initString = irStr2asmStr(irInitString);
        this.size = this.initString.length();
        this.isFloat = isFloat;
    }

    public ASMGlobalObject(int floatVar) {
        this.labelName = ".LC" + (LC_CNT++);
        this.initValues = List.of(new ASMGlbInitVal.ASMGlbWord(floatVar));
        this.initString = null;
        this.size = 4;
        this.isFloat = true;
    }

    private List<ASMGlbInitVal> mergeZero(List<ASMGlbInitVal> originalList) {
        List<ASMGlbInitVal> newList = new ArrayList<>();
        for (ASMGlbInitVal initVal : originalList) {
            if (initVal instanceof ASMGlbInitVal.ASMGlbZero oldLast && !newList.isEmpty() &&
                    newList.getLast() instanceof ASMGlbInitVal.ASMGlbZero newLast) {
                newLast.merge(oldLast);
            } else {
                newList.add(initVal);
            }
        }
        return newList;
    }

    private String irStr2asmStr(String irStr) {
        int len = irStr.length();
        if (len < 3 || !irStr.substring(len - 3).equals("\\00")) {
            throw new RuntimeException("IR 的字符串字面量一定以 \\00 结尾");
        }
        return irStr.substring(0, len - 3).replace("\\0A", "\\n");
    }

    public String getLabelName() {
        return labelName;
    }

    public boolean isFloat() {
        return isFloat;
    }

    public int getSize() {
        return size;
    }

    public void addSize(int add) {
        size += add;
    }

    public List<ASMGlbInitVal> getInitValues() {
        return initValues;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();

        if (!isFloat) {
            sb.append("\t.globl\t");
            sb.append(labelName);
            sb.append("\n");
        }

        if (isFloat) {
            sb.append("\t.section .rodata\n");
            sb.append("\t.align 3\n");
        } else if (initValues != null && !initValues.isEmpty()) {
            sb.append("\t.data\n");
            if (size > 4) {
                sb.append("\t.align 4\n");  // 数组要 16 字节对齐
            }
        } else {
            sb.append("\t.bss\n");
        }

        if (!isFloat) {
            sb.append("\t.type\t");
            sb.append(labelName);
            sb.append(",\t@object\n");
            sb.append("\t.size\t");
            sb.append(labelName);
            sb.append(",\t");
            sb.append(size);
            sb.append("\n");
        }
        sb.append(labelName);
        sb.append(":\n");

        if (initValues != null && !initValues.isEmpty()) {
            for (ASMGlbInitVal initVal : initValues) {
                sb.append("\t").append(initVal.toString()).append("\n");
            }
        } else {
            sb.append("\t.zero\t").append(size).append("\n");
        }
        return sb.toString();

//        if (initValues != null && initString == null) {
//            if (initValues.size() == 1 && initValues.getFirst() instanceof ASMGlbInitVal.ASMGlbZero) {
//                stringBuilder.append(".bss\n");
//            } else {
//                stringBuilder.append(".data\n");
//            }
//
//            stringBuilder.append(".global ").append(labelName).append("\n");
//            stringBuilder.append(".type   ").append(labelName).append(", @object\n");
//            stringBuilder.append(".p2align 2\n");
//
//            stringBuilder.append(labelName).append("\n");
//            for (ASMGlbInitVal initVal : initValues) {
//                stringBuilder.append("\t").append(initVal.toString()).append("\n");
//            }
//
//            stringBuilder.append(".size ").append(labelName).append(", ").append(this.size).append("\n");
//        } else if (initValues == null && initString != null) {
//            stringBuilder.append(labelName);
//            stringBuilder.append(": .asciiz \"").append(initString).append("\"");
//        } else {
//            throw new RuntimeException("出现了奇怪的情况");
//        }

//        return sb.toString();
    }
}
