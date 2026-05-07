package backend.risc_v.entity;


import static backend.risc_v.entity.instrcution.RTypeInstruction.RTypeOperation;

/**
 * &#064;Classname RiscVMacro
 * &#064;Description  TODO
 * &#064;Date 2025/7/29 9:00
 * &#064;Created MuJue
 */
public class RiscVMacro {


    private final RTypeOperation type;

    public RiscVMacro(RTypeOperation type) {
        this.type = type;
    }

    @Override
    public String toString() {
        switch (type) {
            // 整型伪指令
            case SEQ:
                return ".macro seq rd, rs1, rs2\n" +
                        "    xor \\rd, \\rs1, \\rs2\n" +
                        "    seqz \\rd, \\rd\n" +
                        ".endm";
            case SNE:
                return ".macro sne rd, rs1, rs2\n" +
                        "    xor \\rd, \\rs1, \\rs2\n" +
                        "    snez \\rd, \\rd\n" +
                        ".endm";
            case SGT:
                return ".macro sgt rd, rs1, rs2\n" +
                        "    slt \\rd, \\rs2, \\rs1\n" +
                        ".endm";
            case SGE:
                return ".macro sge rd, rs1, rs2\n" +
                        "    slt \\rd, \\rs1, \\rs2\n" +
                        "    xori \\rd, \\rd, 1\n" +
                        ".endm";
            case SLE:
                return ".macro sle rd, rs1, rs2\n" +
                        "    slt \\rd, \\rs2, \\rs1\n" +
                        "    xori \\rd, \\rd, 1\n" +
                        ".endm";
            case SEQZ:
                return ".macro seqz rd, rs\n" +
                        "    sltiu \\rd, \\rs, 1\n" +
                        ".endm";
            case SNEZ:
                return ".macro snez rd, rs\n" +
                        "    sltu \\rd, x0, \\rs\n" +
                        ".endm";

            // 浮点伪指令（单精度）
            case FNE_S:
                return ".macro fne_s rd, fs1, fs2\n" +
                        "    feq.s \\rd, \\fs1, \\fs2\n" +
                        "    xori \\rd, \\rd, 1\n" +
                        ".endm";
            case FLE_S:
                return ".macro fle_s rd, fs1, fs2\n" +
                        "    fle.s \\rd, \\fs1, \\fs2\n" +
                        ".endm";
            case FGE_S:
                return ".macro fge_s rd, fs1, fs2\n" +
                        "    fle.s \\rd, \\fs2, \\fs1\n" +
                        ".endm";
            case FGT_S:
                return ".macro fgt_s rd, fs1, fs2\n" +
                        "    flt.s \\rd, \\fs2, \\fs1\n" +
                        ".endm";

            default:
                return "# Unknown macro type";
        }
    }
}


