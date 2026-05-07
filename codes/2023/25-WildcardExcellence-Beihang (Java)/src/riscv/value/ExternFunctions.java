package riscv.value;

import java.util.HashSet;
import java.util.List;


public class ExternFunctions {
    private static final HashSet<String> externFuncSet = new HashSet<>(List.of(
            "getint", "getch", "getfloat", "getarray",
            "getfarray", "putint", "putch", "putarray",
            "putfloat", "putfarray", "putf", "starttime",
            "stoptime"
    ));

    public static boolean isExternFunc(String funcName) {
        return externFuncSet.contains(funcName);
    }
}
