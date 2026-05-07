package entities;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class Statement extends Assembler {

    private final Op op;
    private final List<Register> args = new ArrayList<>();

    public Statement(Op op, Register... args) {
        this.op = op;
        this.args.addAll(Arrays.asList(args));
    }

    public Op getOp() {
        return op;
    }

    public boolean opIs(Op op) {
        return this.op == op;
    }

    public Register getArg(int index) {
        return args.get(index);
    }

    public List<Register> getArgs() {
        return args;
    }

    public enum Op {
        LI, LA, MV, LD, SD, LW, SW, CALL, RET, J, BEQZ,
        ADD, MUL, ADDI, SLLI,
        ADDW, SUBW, MULW, DIVW, REMW,
        FMV_S, FLW, FSW, FLD, FSD,
        FADD_S, FSUB_S, FMUL_S, FDIV_S,
        _GLOBL, _SECTION, _ZERO, _DWORD, _SPACE, FCVT_S_W, FCVT_W_S,
        FLT_S, FGT_S, FEQ_S,
        SLT, SGT, SEQ, SNE, XOR, XORI, SLTIU,
        CSRRS, SEXT_W;
        private final String name;

        Op() {
            this.name = this.name().toLowerCase().replace("_", ".");
        }

        @Override
        public String toString() {
            return name;
        }
    }

    @Override
    public String toString() {
        StringBuilder stringBuilder = new StringBuilder(" ");
        stringBuilder.append(op);
        for (int i = 0, size = args.size(); i < size; i++) {
            stringBuilder.append(" ").append(args.get(i));
            if (i != size - 1) {
                stringBuilder.append(",");
            }
        }
        return stringBuilder.toString();
    }
}
