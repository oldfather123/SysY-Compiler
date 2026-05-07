package cn.edu.bit.newnewcc.backend.asm.util;

public class ImmediateValues {
    public static boolean bitLengthNotInLimit(long value) {
        return value < -2048 || value >= 2048;
    }

    public static boolean isIntValue(long value) {
        int intMax = 0x7fffffff, intMin = -intMax - 1;
        return intMin <= value && value <= intMax;
    }
}
