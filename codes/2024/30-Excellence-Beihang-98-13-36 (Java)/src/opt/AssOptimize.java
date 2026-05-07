package opt;

import entities.Assembler;
import entities.DoubleList;
import entities.Register;
import entities.Statement;

public class AssOptimize {

    public static void opt(DoubleList<Assembler> assembler) {
        DoubleList<Assembler>.Iterator iter = assembler.iterator();
        while (iter.hasNext()) {
            Assembler ass = iter.next();
            if (ass instanceof Statement statement) {
                if (statement.opIs(Statement.Op.ADDI)) {
                    Register r11 = statement.getArg(0), r12 = statement.getArg(1);
                    if (Register.equal(r11, r12)) {
                        Register imm = statement.getArg(2);
                        if (imm.getNum() == 0) {
                            iter.remove();
                            continue;
                        }
                        Assembler ass2 = iter.next();
                        if (ass2 instanceof Statement statement2 && statement2.opIs(Statement.Op.ADDI)) {
                            Register r21 = statement2.getArg(0), r22 = statement2.getArg(1);
                            if (Register.equal(r11, r21) && Register.equal(r21, r22)) {
                                Register imm2 = statement2.getArg(2);
                                int num = imm.getNum() + imm2.getNum();
                                iter.remove();
                                iter.prev();
                                ((Statement) iter.getObj()).getArgs().set(2, Register.num(num));
                                continue;
                            }
                        }
                        iter.prev();
                    }
                } else if (statement.opIs(Statement.Op.LI)) {
                    Assembler ass2 = iter.next();
                    if (ass2 instanceof Statement statement2) {
                        if (statement2.opIs(Statement.Op.MV)) {
                            Register r1 = statement.getArg(0), r2 = statement2.getArg(1);
                            if (Register.equal(r1, r2) && noUseBeforeAssign(r1, assembler.copyIterator(iter))) {
                                Register r3 = statement2.getArg(0);
                                iter.remove();
                                iter.prev();
                                ((Statement) iter.getObj()).getArgs().set(0, r3);
                                continue;
                            }
                        }
                    }
                    iter.prev();
                }
            }
        }
    }

    private static boolean noUseBeforeAssign(Register r, DoubleList<Assembler>.Iterator iter) {
        int cnt = 0;
        while (iter.hasNext() && cnt < 5) {
            cnt++;
            Assembler ass = iter.next();
            if (ass instanceof Statement statement) {
                switch (statement.getOp()) {
                    case LI, LA -> {
                        if (Register.equal(statement.getArg(0), r)) {
                            return true;
                        }
                    }
                    case FCVT_S_W, FCVT_W_S, MV, FMV_S -> {
                        if (Register.equal(statement.getArg(1), r)) {
                            return false;
                        }
                        if (Register.equal(statement.getArg(0), r)) {
                            return true;
                        }
                    }
                    case LD, SD, SW, FSW, FLD, FSD -> {
                        if (Register.equalNotStrict(statement.getArg(0), r)
                                || Register.equalNotStrict(statement.getArg(1), r)) {
                            return false;
                        }
                    }
                    case LW, FLW -> {
                        if (Register.equalNotStrict(statement.getArg(1), r)) {
                            return false;
                        }
                        if (Register.equal(statement.getArg(0), r)) {
                            return true;
                        }
                    }
                    case CALL, RET, J, BEQZ -> {
                        return false;
                    }
                    case ADD, MUL, ADDI, SLLI, ADDW, SUBW, MULW, DIVW, REMW, FADD_S, FSUB_S, FMUL_S, FDIV_S,
                         FLT_S, FGT_S, FEQ_S, SLT, SGT, SEQ, SNE, XOR, XORI, SLTIU -> {
                        if (Register.equal(statement.getArg(1), r) || Register.equal(statement.getArg(2), r)) {
                            return false;
                        }
                        if (Register.equal(statement.getArg(0), r)) {
                            return true;
                        }
                    }
                    case SEXT_W -> {
                        if (Register.equal(statement.getArg(0), r)) {
                            return false;
                        }
                    }
                }
            }
        }
        return false;
    }

}
