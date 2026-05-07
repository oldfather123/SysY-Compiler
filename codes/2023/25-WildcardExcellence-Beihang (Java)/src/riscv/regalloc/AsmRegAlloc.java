package riscv.regalloc;

import midend.MyModule;
import riscv.value.AsmModule;
import riscv.value.AsmReg;
import riscv.value.ExternFunctions;

import java.util.ArrayList;
import java.util.List;

import static riscv.value.AsmReg.*;

public class AsmRegAlloc {
    public static void runAlloc(MyModule module, AsmModule asmModule) {
        var availableRegs = calcAvailableRegs();
        var availableFRegs = calcAvailableFRegs();

        for (var func : module.getFunctions().values()) {
            String funcName = func.getName();
            if (ExternFunctions.isExternFunc(funcName)) continue;
            LinearScan.runAllocForFunc(
                    func,
                    asmModule.funcMap.get(funcName),
                    new RegPool(availableRegs),
                    new RegPool(availableFRegs)
            );
        }
    }

    private static List<AsmReg> calcAvailableRegs() {
        // t5, t6 留作溢出寄存器
        List<AsmReg> result = new ArrayList<>(List.of(
                t1, t2, t3, t4, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, tp
        ));

        return result;
    }

    private static List<AsmReg> calcAvailableFRegs() {
        // ft10, ft11 留作溢出寄存器
        List<AsmReg> result = new ArrayList<>(List.of(
                ft1, ft2, ft3, ft4, ft5, ft6, ft7, ft8, ft9,
                fs0, fs1, fs2, fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11
        ));

        return result;
    }
}
