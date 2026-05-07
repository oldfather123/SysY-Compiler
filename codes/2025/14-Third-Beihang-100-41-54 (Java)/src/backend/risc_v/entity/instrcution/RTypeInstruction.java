package backend.risc_v.entity.instrcution;

import backend.risc_v.entity.register.Register;

public class RTypeInstruction extends RiscVInstruction {
    public enum RTypeOperation {
        ADD("add"),
        SUB("sub"),
        MUL("mul"),
        DIV("div"),
        REM("rem"),
        XOR("xor"),
        // 真不知道多写几个比较类型会死是吗？
        SLT("slt"),
        SLE("sle"),
        SGT("sgt"),
        SGE("sge"),
        SEQ("seq"),
        SNE("sne"),
        SEQZ("seqz"),
        SNEZ("snez"),
        FADD_S("fadd.s"),
        FSUB_S("fsub.s"),
        FMUL_S("fmul.s"),
        FDIV_S("fdiv.s"),
        FGT_S("fgt_s"),
        FGE_S("fge_s"),
        FLT_S("flt.s"),
        FLE_S("fle_s"),
        FNE_S("fne_s"),
        FEQ_S("feq.s"),
        FCVT_S_W("fcvt.s.w"),
        FCVT_W_S("fcvt.w.s"),
        FMV_S_X("fmv.s.x");


        private final String name;

        RTypeOperation(String name) {
            this.name = name;
        }

        @Override
        public String toString() {
            return name;
        }
    }

    private final RTypeOperation operation;
    private final Register rd;
    private final Register rs1;
    private final Register rs2;

    public RTypeInstruction(RTypeOperation operation, Register rd, Register rs1, Register rs2) {
        this.operation = operation;
        this.rd = rd;
        this.rs1 = rs1;
        this.rs2 = rs2;
    }

    public RTypeInstruction(RTypeOperation operation, Register rd, Register rs1) {
        this(operation, rd, rs1, null);
    }

    @Override
    public String toString() {
        if (rs2 != null)
            return operation + " " + rd + ", " + rs1 + ", " + rs2;
        else if (operation == RTypeOperation.FCVT_W_S)
            return operation + " " + rd + ", " + rs1 + ", rtz";
        else
            return operation + " " + rd + ", " + rs1;
    }
}
