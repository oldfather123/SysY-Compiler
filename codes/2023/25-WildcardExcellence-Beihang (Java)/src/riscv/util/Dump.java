package riscv.util;

import riscv.AsmGen;
import riscv.value.*;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public class Dump {
    public static void dumpToFile(AsmModule module, String asmPath) throws IOException {
        StringBuilder sb = new StringBuilder();

        // header
        String header = """
                .global main
                .option norelax
                .data
                data_start: .space %d
                
                .text
                main:
                la gp, data_start
                                
                """.formatted(module.globalVarAllocTable.globalSize());
        sb.append(header);


        // global
        for (var asmp : module.globalInstrs) {
            var asm = asmp.get();
            sb.append(asmToString(asm, 0)).append('\n');
        }
        if (!module.globalInstrs.isEmpty()) sb.append('\n');

        // entry
        sb.append(entry(module.funcMap.get("main").allocTable.frameSize()));

        // funcs
        for (AsmFunction func : module.funcMap.values()) {
            if (ExternFunctions.isExternFunc(func.funcName)) continue;
            sb.append("Func_").append(func.funcName).append(":\n");
            for (var bb : func.bbList) {
                sb.append("  ").append(bb.name).append(":\n");
                for (var asmp : bb.instrs) {
                    AsmInstr asm = asmp.get();
                    sb.append(asmToString(asm, 4)).append('\n');
                }
            }
            sb.append("\n");
        }

        if (module.hasMemset) {
            sb.append(MyMemset.asm);
        }

        Path path = Paths.get(asmPath);
        Files.writeString(path, sb.toString());
    }

    private static String asmToString(AsmInstr asm, int ident) {
        String identStr = " ".repeat(ident);
        if (asm.comment == null) {
            return identStr + asm;
        }
        else {
            return String.format("%s%-20s  # %s", identStr, asm, asm.comment);
        }
    }

    private static String entry(int mainFuncFrameSize) {
        int spMove = AsmGen.ceilTo16(mainFuncFrameSize);
        if (JustifyShortImm.isI12(spMove)) {
            return """
                    addi sp, sp, -%d
                    sd ra, 0(gp)
                    jal Func_main
                    ld ra, 0(gp)
                    addi sp, sp, %d
                    ret
                                    
                    """.formatted(spMove, spMove);
        }
        else {
            return """
                    li t0, %d
                    sub sp, sp, t0
                    sd ra, 0(gp)
                    jal Func_main
                    ld ra, 0(gp)
                    li t0, %d
                    add sp, sp, t0
                    ret
                                    
                    """.formatted(spMove, spMove);
        }
    }

}
