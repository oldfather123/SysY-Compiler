package riscv;

import midend.MyModule;
import midend.Type;
import midend.value.Constant;
import midend.value.Value;
import midend.value.global.GlobalVariable;
import riscv.value.AsmInstr;
import riscv.value.AsmModule;
import riscv.value.AsmReg;

import java.util.List;

class GlobalAsmGen {
    private static AsmModule asmModule;

    static void globalAsmGen(MyModule irModule, AsmModule asmModule) {
        GlobalAsmGen.asmModule = asmModule;
        for (var globalVar : irModule.getGlobalVariables()) {
            genGlobalVar(globalVar);
        }
    }

    private static void genGlobalVar(GlobalVariable globalVar) {
        int length = globalVar.getType().getElementType().getLength();
        int size = length * 4;
        asmModule.globalVarAllocTable.addGlobalVarWithSize(globalVar, size);

        Type initType = globalVar.getInitialType();
        Value initVal = globalVar.getInitialValue();
        int offset = asmModule.globalVarAllocTable.getGlobalVarOffset(globalVar);

        // 对于初值，生成 store 语句
        if (initVal instanceof Constant.ConstantInteger constInt) {
            int initInt = constInt.getConstantIntegerValue();
            if (initInt == 0) {
                addAsmInstr(AsmInstr.Pseudo.Store(AsmReg.zero, AsmReg.gp, offset, 4));
            }
            else {
                addAsmInstrs(List.of(
                        new AsmInstr.Li(AsmReg.rimm, initInt),
                        AsmInstr.Pseudo.Store(AsmReg.rimm, AsmReg.gp, offset, 4)
                ));
            }
        }
        else if (initVal instanceof Constant.ConstantFloat constFloat) {
            int initInt = constFloat.getIEEEInt32();
            addAsmInstrs(List.of(
                    new AsmInstr.Li(AsmReg.rimm, initInt, true).withComment(String.valueOf(constFloat.getConstantFloatValue())),
                    AsmInstr.Pseudo.Store(AsmReg.rimm, AsmReg.gp, offset, 4)
            ));
        }
        else if (initVal instanceof Constant.ConstantArray initArray) {
            // 先清零数组，再填东西
            genMemsetZero(offset, length);

            Type elementType = initType.getElementType();
            while (!elementType.isBasicType()) elementType = elementType.getElementType();
            // int 数组初始化
            if (elementType.isIntegerType()) {
                List<Integer> initIntList = initArray.getBasicValueList().stream()
                        .map(value -> ((Constant.ConstantInteger) value).getConstantIntegerValue()).toList();
                assert length == initIntList.size();
                for (int i = 0; i < initIntList.size(); i++) {
                    int initInt = initIntList.get(i);
                    if (initInt != 0) {
                        addAsmInstrs(List.of(
                                new AsmInstr.Li(AsmReg.rimm, initInt),
                                AsmInstr.Pseudo.Store(AsmReg.rimm, AsmReg.gp, offset + i * 4, 4)
                        ));
                    }
                }
            }
            // 浮点数组初始化
            else {
                for (int i = 0; i < initArray.getBasicValueList().size(); i++) {
                    Constant.ConstantFloat constFloat = (Constant.ConstantFloat) initArray.getBasicValueList().get(i);
                    float initFloat = constFloat.getConstantFloatValue();
                    int ieeeHex = constFloat.getIEEEInt32();
                    if (ieeeHex != 0) {
                        addAsmInstrs(List.of(
                                new AsmInstr.Li(AsmReg.rimm, ieeeHex, true).withComment(String.valueOf(initFloat)),
                                AsmInstr.Pseudo.Move(AsmReg.frtmp, AsmReg.rimm),
                                AsmInstr.Pseudo.Store(AsmReg.frtmp, AsmReg.gp, offset + i * 4, 4)
                        ));
                    }
                }
            }
        }
        else if (initVal instanceof Constant.ZeroInitArray) {
            genMemsetZero(offset, length);
        }
        else throw new AssertionError("意外的全局变量类型 " + initType);
    }


    private static void genMemsetZero(int offset, int length) {
        if (length <= 16) {
            for (int i = 0; i < length / 2; i++) {
                addAsmInstr(AsmInstr.Pseudo.Store(AsmReg.zero, AsmReg.gp, offset + i * 8, 8));
            }
            if (length % 2 == 1) {
                addAsmInstr(AsmInstr.Pseudo.Store(AsmReg.zero, AsmReg.gp, offset + (length - 1) * 4, 4));
            }
        }
        else {
            // 内置函数 my_memset，在 Dump 过程中生成
            addAsmInstrs(List.of(
                    AsmInstr.Pseudo.Store(AsmReg.ra, AsmReg.gp, 0, 8),
                    new AsmInstr.Addi(AsmReg.a0, AsmReg.gp, offset),
                    new AsmInstr.Li(AsmReg.a1, length),
                    new AsmInstr.JalByName("memset_zero"),
                    AsmInstr.Pseudo.Load(AsmReg.ra, AsmReg.gp, 0, 8)
            ));
            asmModule.hasMemset = true;
        }
    }

    private static void addAsmInstr(AsmInstr instr) {
        asmModule.globalInstrs.addLast(instr);
    }

    private static void addAsmInstrs(List<AsmInstr> instrs) {
        for (var instr : instrs) {
            addAsmInstr(instr);
        }
    }
}
