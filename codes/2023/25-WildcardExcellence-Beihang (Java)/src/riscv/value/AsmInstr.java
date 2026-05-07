package riscv.value;

// WriteType
//     RType (add sub ...)
//     IType (addi ...)
//     LoadType (lw ld)
//     li
// StoreType (sw sd)
// BType (beq bne ...)
// SingleRType (neg not fcvt ...)
// Other (nop j jal call ret ...)
public abstract class AsmInstr {
    public AsmReg[] useList = new AsmReg[0];
    public String comment = null;

    public AsmInstr withComment(String comment) {
        this.comment = comment;
        return this;
    }

    public abstract static class WriteType extends AsmInstr {
        public final AsmReg rd;

        public WriteType(AsmReg rd) {
            this.rd = rd;
        }
    }

    public abstract static class RType extends WriteType {
        public final AsmReg rs1, rs2;
        private final String name;

        public RType(AsmReg rd, AsmReg rs1, AsmReg rs2, String name) {
            super(rd);
            this.rs1 = rs1;
            this.rs2 = rs2;
            this.name = name;
            this.useList = new AsmReg[]{rs1, rs2};
        }

        @Override
        public String toString() {
            return String.format("%s %s, %s, %s", name, rd.name(), rs1.name(), rs2.name());
        }
    }

    public abstract static class IType extends WriteType {
        public final AsmReg rs;
        public final int imm;
        private final String name;

        public IType(AsmReg rd, AsmReg rs, int imm, String name) {
            super(rd);
            this.rs = rs;
            this.imm = imm;
            this.name = name;
            this.useList = new AsmReg[]{rs};
        }

        @Override
        public String toString() {
            return String.format("%s %s, %s, %d", name, rd.name(), rs.name(), imm);
        }
    }

    public abstract static class LoadType extends WriteType {
        public AsmReg rbase;
        public int offset;
        private final String name;

        public LoadType(AsmReg rd, AsmReg rbase, int offset, String name) {
            super(rd);
            this.rbase = rbase;
            this.offset = offset;
            this.name = name;
            this.useList = new AsmReg[]{rbase};
        }

        @Override
        public String toString() {
            return String.format("%s %s, %d(%s)", name, rd.name(), offset, rbase.name());
        }
    }

    public abstract static class StoreType extends AsmInstr {
        public AsmReg rs, rbase;
        public int offset;
        private final String name;

        public StoreType(AsmReg rs, AsmReg rbase, int offset, String name) {
            this.rs = rs;
            this.rbase = rbase;
            this.offset = offset;
            this.name = name;
            this.useList = new AsmReg[]{rs, rbase};
        }

        @Override
        public String toString() {
            return String.format("%s %s, %d(%s)", name, rs.name(), offset, rbase.name());
        }
    }

    public abstract static class BType extends AsmInstr {
        public final AsmReg rs1, rs2;
        public final AsmBasicBlock bb;
        private final String name;

        public BType(AsmReg rs1, AsmReg rs2, AsmBasicBlock bb, String name) {
            this.rs1 = rs1;
            this.rs2 = rs2;
            this.bb = bb;
            this.name = name;
            this.useList = new AsmReg[]{rs1, rs2};
        }

        @Override
        public String toString() {
            return String.format("%s %s, %s, %s", name, rs1.name(), rs2.name(), bb.name);
        }
    }

    public abstract static class SingleRType extends WriteType {
        public final AsmReg rs;
        private final String name;

        public SingleRType(AsmReg rd, AsmReg rs, String name) {
            super(rd);
            this.rs = rs;
            this.name = name;
            this.useList = new AsmReg[]{rs};
        }

        @Override
        public String toString() {
            return String.format("%s %s, %s", name, rd.name(), rs.name());
        }
    }

    public static class Add extends RType {
        public Add(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "add");
        }
    }

    public static class Addi extends IType {
        public Addi(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "addi");
        }
    }

    public static class Sub extends RType {
        public Sub(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "sub");
        }
    }

    public static class Mul extends RType {
        public Mul(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "mul");
        }
    }

    public static class Mulh extends RType {
        public Mulh(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "mulh");
        }
    }

    public static class Div extends RType {
        public Div(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "div");
        }
    }

    public static class Divu extends RType {
        public Divu(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "divu");
        }
    }

    public static class Rem extends RType {
        public Rem(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "rem");
        }
    }

    public static class Remu extends RType {
        public Remu(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "remu");
        }
    }

    public static class And extends RType {
        public And(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "and");
        }
    }

    public static class Andi extends IType {
        public Andi(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "andi");
        }
    }

    public static class Or extends RType {
        public Or(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "or");
        }
    }

    public static class Ori extends IType {
        public Ori(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "ori");
        }
    }

    public static class Xor extends RType {
        public Xor(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "xor");
        }
    }

    public static class Xori extends IType {
        public Xori(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "xori");
        }
    }

    public static class Sll extends RType {
        public Sll(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "sllw");
        }
    }

    public static class Slli extends IType {
        public Slli(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "slliw");
        }
    }

    public static class Slli64 extends IType {
        public Slli64(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "slli");
        }
    }

    public static class Sra extends RType {
        public Sra(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "sraw");
        }
    }

    public static class Srai extends IType {
        public Srai(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "sraiw");
        }
    }

    public static class Srl extends RType {
        public Srl(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "srlw");
        }
    }

    public static class Srli extends IType {
        public Srli(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "srliw");
        }
    }

    public static class Srli64 extends IType {
        public Srli64(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "srli");
        }
    }

    public static class Slt extends RType {
        public Slt(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "slt");
        }
    }

    public static class Slti extends IType {
        public Slti(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "slti");
        }
    }

    public static class Sltu extends RType {
        public Sltu(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "sltu");
        }
    }

    public static class Sltiu extends IType {
        public Sltiu(AsmReg rd, AsmReg rs, int imm) {
            super(rd, rs, imm, "sltiu");
        }
    }

    public static class La extends WriteType {
        public final String symbol;

        public La(AsmReg rd, String symbol) {
            super(rd);
            this.symbol = symbol;
        }

        @Override
        public String toString() {
            return String.format("la %s, %s", rd, symbol);
        }
    }

    public static class Mv extends SingleRType {
        public Mv(AsmReg rd, AsmReg rs) {
            super(rd, rs, "mv");
        }
    }

    public static class Lw extends LoadType {
        public Lw(AsmReg rd, AsmReg rbase, int offset) {
            super(rd, rbase, offset, "lw");
        }
    }

    public static class Ld extends LoadType {
        public Ld(AsmReg rd, AsmReg rbase, int offset) {
            super(rd, rbase, offset, "ld");
        }
    }

    public static class Sw extends StoreType {
        public Sw(AsmReg rs, AsmReg rbase, int offset) {
            super(rs, rbase, offset, "sw");
        }
    }

    public static class Sd extends StoreType {
        public Sd(AsmReg rs, AsmReg rbase, int offset) {
            super(rs, rbase, offset, "sd");
        }
    }

    public static class Nop extends AsmInstr {
        @Override
        public String toString() {
            return "nop";
        }
    }

    public static class Li extends WriteType {
        public final int imm;
        private final boolean hex;

        public Li(AsmReg rd, int imm) {
            super(rd);
            this.imm = imm;
            this.hex = false;
        }

        public Li(AsmReg rd, int imm, boolean hex) {
            super(rd);
            this.imm = imm;
            this.hex = hex;
        }

        @Override
        public String toString() {
            if (!hex) return String.format("li %s, %d", rd.name(), imm);
            else return String.format("li %s, 0x%s", rd.name(), Integer.toHexString(imm));
        }
    }

    public static class Li64 extends WriteType {
        public final long imm;
        private final boolean hex;

        public Li64(AsmReg rd, long imm) {
            super(rd);
            this.imm = imm;
            this.hex = false;
        }

        public Li64(AsmReg rd, long imm, boolean hex) {
            super(rd);
            this.imm = imm;
            this.hex = hex;
        }

        @Override
        public String toString() {
            if (!hex) return String.format("li %s, %d", rd.name(), imm);
            else return String.format("li %s, 0x%s", rd.name(), Long.toHexString(imm));
        }
    }

    public static class J extends AsmInstr {
        public final AsmBasicBlock bb;

        public J(AsmBasicBlock bb) {
            this.bb = bb;
        }

        @Override
        public String toString() {
            return "j " + bb.name;
        }
    }

    public static class Jal extends AsmInstr {
        public final AsmFunction func;

        public Jal(AsmFunction func) {
            this.func = func;
        }

        @Override
        public String toString() {
            return "call Func_" + func.funcName;
        }
    }

    public static class JalByName extends AsmInstr {
        public final String funcName;

        public JalByName(String funcName) {
            this.funcName = funcName;
        }

        @Override
        public String toString() {
            return "call " + funcName;
        }
    }

    public static class Call extends AsmInstr {
        public final String funcName;

        public Call(String funcName) {
            this.funcName = funcName;
        }

        @Override
        public String toString() {
            return "call " + funcName;
        }
    }

    public static class Ret extends AsmInstr {
        @Override
        public String toString() {
            return "ret";
        }
    }

    public static class Beq extends BType {
        public Beq(AsmReg rs1, AsmReg rs2, AsmBasicBlock bb) {
            super(rs1, rs2, bb, "beq");
        }
    }

    public static class Bne extends BType {
        public Bne(AsmReg rs1, AsmReg rs2, AsmBasicBlock bb) {
            super(rs1, rs2, bb, "bne");
        }
    }

    public static class Blt extends BType {
        public Blt(AsmReg rs1, AsmReg rs2, AsmBasicBlock bb) {
            super(rs1, rs2, bb, "blt");
        }
    }

    public static class Bge extends BType {
        public Bge(AsmReg rs1, AsmReg rs2, AsmBasicBlock bb) {
            super(rs1, rs2, bb, "bge");
        }
    }

    // Float Instr

    public static class FAdd extends RType {
        public FAdd(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "fadd.s");
        }
    }

    public static class FSub extends RType {
        public FSub(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "fsub.s");
        }
    }

    public static class FMul extends RType {
        public FMul(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "fmul.s");
        }
    }

    public static class FDiv extends RType {
        public FDiv(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "fdiv.s");
        }
    }

    public static class FEq extends RType {
        public FEq(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "feq.s");
        }
    }

    public static class FLt extends RType {
        public FLt(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "flt.s");
        }
    }

    public static class FLe extends RType {
        public FLe(AsmReg rd, AsmReg rs1, AsmReg rs2) {
            super(rd, rs1, rs2, "fle.s");
        }
    }

    public static class FCvtI2F extends SingleRType {
        public FCvtI2F(AsmReg rd, AsmReg rs) {
            super(rd, rs, "fcvt.s.w");
        }
    }

    public static class FCvtF2I extends SingleRType {
        public FCvtF2I(AsmReg rd, AsmReg rs) {
            super(rd, rs, "fcvt.w.s");
        }
    }

    public static class FMv extends SingleRType {
        public FMv(AsmReg rd, AsmReg rs) {
            super(rd, rs, "fmv.s");
        }
    }

    public static class FMvI2F extends SingleRType {
        public FMvI2F(AsmReg rd, AsmReg rs) {
            super(rd, rs, "fmv.s.x");
        }
    }

    public static class FMvF2I extends SingleRType {
        public FMvF2I(AsmReg rd, AsmReg rs) {
            super(rd, rs, "fmv.x.s");
        }
    }

    public static class FNeg extends SingleRType {
        public FNeg(AsmReg rd, AsmReg rs) {
            super(rd, rs, "fneg.s");
        }
    }

    public static class FAbs extends SingleRType {
        public FAbs(AsmReg rd, AsmReg rs) {
            super(rd, rs, "fabs.s");
        }
    }

    public static class FLw extends LoadType {
        public FLw(AsmReg rd, AsmReg rbase, int offset) {
            super(rd, rbase, offset, "flw");
        }
    }

    public static class FLd extends LoadType {
        public FLd(AsmReg rd, AsmReg rbase, int offset) {
            super(rd, rbase, offset, "flw");
        }
    }

    public static class FSw extends StoreType {
        public FSw(AsmReg rs, AsmReg rbase, int offset) {
            super(rs, rbase, offset, "fsw");
        }
    }

    public static class FSd extends StoreType {
        public FSd(AsmReg rs, AsmReg rbase, int offset) {
            super(rs, rbase, offset, "fsw");
        }
    }

    public static class Pseudo {
        public static Addi Subi(AsmReg rd, AsmReg rs, int imm) {
            return new Addi(rd, rs, -imm);
        }

        public static Sub Neg(AsmReg rd, AsmReg rs) {
            return new Sub(rd, AsmReg.zero, rs);
        }

        public static Xori LogicNot(AsmReg rd, AsmReg rs) {
            return new Xori(rd, rs, 1);
        }

        public static Sltiu Seqz(AsmReg rd, AsmReg rs) {
            return new Sltiu(rd, rs, 1);
        }

        public static Sltu Snez(AsmReg rd, AsmReg rs) {
            return new Sltu(rd, AsmReg.zero, rs);
        }

        public static Blt Bgt(AsmReg rs1, AsmReg rs2, AsmBasicBlock bb) {
            return new Blt(rs2, rs1, bb);
        }

        public static Bge Ble(AsmReg rs1, AsmReg rs2, AsmBasicBlock bb) {
            return new Bge(rs2, rs1, bb);
        }

        // 自动选择合适的 move 指令
        public static SingleRType Move(AsmReg rd, AsmReg rs) {
            boolean rdf = AsmReg.isFloatReg(rd);
            boolean rsf = AsmReg.isFloatReg(rs);
            if (rdf) {
                if (rsf) return new FMv(rd, rs);
                else return new FMvI2F(rd, rs);
            }
            else {
                if (rsf) return new FMvF2I(rd, rs);
                else return new Mv(rd, rs);
            }
        }

        // 自动选择合适的 load/store 指令
        public static LoadType Load(AsmReg rd, AsmReg rbase, int offset, int size) {
            if (size == 8) {
                if (AsmReg.isFloatReg(rd)) return new FLd(rd, rbase, offset);
                else return new Ld(rd, rbase, offset);
            }
            else if (size == 4) {
                if (AsmReg.isFloatReg(rd)) return new FLw(rd, rbase, offset);
                else return new Lw(rd, rbase, offset);
            }
            else throw new AssertionError("错误的对齐 " + size);
        }

        public static StoreType Store(AsmReg rs, AsmReg rbase, int offset, int size) {
            if (size == 8) {
                if (AsmReg.isFloatReg(rs)) return new FSd(rs, rbase, offset);
                else return new Sd(rs, rbase, offset);
            }
            else if (size == 4) {
                if (AsmReg.isFloatReg(rs)) return new FSw(rs, rbase, offset);
                else return new Sw(rs, rbase, offset);
            }
            else throw new AssertionError("错误的对齐 " + size);
        }
    }
}
