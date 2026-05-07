package cn.edu.nju.software.backendarm;

import cn.edu.nju.software.ir.type.*;

public class ArmSpecifications {

    private static final int INT_SIZE = 4;
    private static final int FLOAT_SIZE = 4;
    private static final int POINTER_SIZE = 8; //todo() here arm32 is 4 need to be changed
    private static final int BOOL_SIZE = 4;
    private static final String[] callerSavedRegs = new String[] {
            "lr", // return address
            "r9", "r10", "r11", // general temp var
            "s9", "s10", "s11" // float temp var
    };
    //todo() arm's general register is less than risc-v
    private static final String[] calculateRegs = new String[] {
            "r4", "r5", "r6", "r7", "r8",
            "s4", "s5", "s6", "s7", "s8"
    };
    private static final String[] calleeSavedRegs = new String[] {};
    private static final String[] argRegs = new String[] {"r0", "r1", "r2", "r3"};
    private static final String[] fArgRegs = new String[] {"s0", "s1", "s2", "s3"};
    private static final String[] tempVarRegs = new String[] {
            "r9", "r10", "r11",
            "s9", "s10", "s11"};
    private static final boolean isDebug = false;

    public static String[] getCallerSavedRegs() {
        return callerSavedRegs;
    }

    public static String[] getCalleeSavedRegs() {
        return calleeSavedRegs;
    }

    public static String[] getArgRegs() {
        return argRegs;
    }

    public static String[] getFArgRegs() {
        return fArgRegs;
    }

    public static String[] getTempVarRegs() {
        return tempVarRegs;
    }

    public static boolean isFloatReg(String regName) {
        return regName.startsWith("s") && !regName.equals("sp");
    }

    public static boolean isGeneralReg(String regName) {
        return !isFloatReg(regName);
    }

    public static boolean isGeneralType(TypeRef type) {
        return type instanceof IntType || type instanceof Pointer || type instanceof BoolType;
    }

    public static boolean isFloatType(TypeRef type) {
        return type instanceof FloatType;
    }

    public static int getIntSize() {
        return INT_SIZE;
    }

    public static int getFloatSize() {
        return FLOAT_SIZE;
    }

    public static int getPointerSize() {
        return POINTER_SIZE;
    }

    public static int getBoolSize() {
        return BOOL_SIZE;
    }

    public static boolean getIsDebug() {
        return isDebug;
    }
}
