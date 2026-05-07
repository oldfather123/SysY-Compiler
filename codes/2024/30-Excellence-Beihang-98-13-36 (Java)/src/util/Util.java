package util;

import entities.*;

import java.util.*;

import static entities.Registers.*;

public class Util {

    public static final Set<String> localArrayToGlobalArray = new HashSet<>();

    public static boolean hasGetIntArray = false, hasGetFloatArray = false, hasPutIntArray = false, hasPutFloatArray = false;
    private static final Register GETINT = Register.label("getint"), GETFLOAT = Register.label("getfloat");
    private static final Register PUTINT = Register.label("putint"), PUTFLOAT = Register.label("putfloat");
    private static final Register PUTCH = Register.label("putch");

    public static final DoubleList<Assembler> getIntArray = DoubleList.of(
            new Label("getarray_______________"),
            new Statement(Statement.Op.ADDI, SP, SP, Register.num(-32)),
            new Statement(Statement.Op.SD, RA, _SP_),
            new Statement(Statement.Op.SD, s(3), sp(8)),
            new Statement(Statement.Op.SD, s(1), sp(16)),
            new Statement(Statement.Op.SD, s(2), sp(24)),

            new Statement(Statement.Op.MV, s(1), a(0)),
            new Statement(Statement.Op.CALL, GETINT),
            new Statement(Statement.Op.MV, s(2), a(0)),
            new Statement(Statement.Op.MV, s(3), a(0)),
            new Label("getarray_While_0"),
            new Statement(Statement.Op.BEQZ, s(2), Register.label("getarray_While_1")),
            new Statement(Statement.Op.CALL, GETINT),
            new Statement(Statement.Op.SW, a(0), s(1).addr()),
            new Statement(Statement.Op.ADDI, s(1), s(1), Register.num(8)),
            new Statement(Statement.Op.ADDI, s(2), s(2), Register.num(-1)),
            new Statement(Statement.Op.J, Register.label("getarray_While_0")),
            new Label("getarray_While_1"),
            new Statement(Statement.Op.MV, a(0), s(3)),

            new Statement(Statement.Op.LD, RA, _SP_),
            new Statement(Statement.Op.LD, s(3), sp(8)),
            new Statement(Statement.Op.LD, s(1), sp(16)),
            new Statement(Statement.Op.LD, s(2), sp(24)),
            new Statement(Statement.Op.ADDI, SP, SP, Register.num(32)),
            new Statement(Statement.Op.RET)
    );

    public static final DoubleList<Assembler> getFloatArray = DoubleList.of(
            new Label("getfarray_______________"),
            new Statement(Statement.Op.ADDI, SP, SP, Register.num(-32)),
            new Statement(Statement.Op.SD, RA, _SP_),
            new Statement(Statement.Op.SD, s(3), sp(8)),
            new Statement(Statement.Op.SD, s(1), sp(16)),
            new Statement(Statement.Op.SD, s(2), sp(24)),

            new Statement(Statement.Op.MV, s(1), a(0)),
            new Statement(Statement.Op.CALL, GETINT),
            new Statement(Statement.Op.MV, s(2), a(0)),
            new Statement(Statement.Op.MV, s(3), a(0)),
            new Label("getfarray_While_0"),
            new Statement(Statement.Op.BEQZ, s(2), Register.label("getfarray_While_1")),
            new Statement(Statement.Op.CALL, GETFLOAT),
            new Statement(Statement.Op.FSW, fa(0), s(1).addr()),
            new Statement(Statement.Op.ADDI, s(1), s(1), Register.num(8)),
            new Statement(Statement.Op.ADDI, s(2), s(2), Register.num(-1)),
            new Statement(Statement.Op.J, Register.label("getfarray_While_0")),
            new Label("getfarray_While_1"),
            new Statement(Statement.Op.MV, a(0), s(3)),

            new Statement(Statement.Op.LD, RA, _SP_),
            new Statement(Statement.Op.LD, s(3), sp(8)),
            new Statement(Statement.Op.LD, s(1), sp(16)),
            new Statement(Statement.Op.LD, s(2), sp(24)),
            new Statement(Statement.Op.ADDI, SP, SP, Register.num(32)),
            new Statement(Statement.Op.RET)
    );

    public static final DoubleList<Assembler> putIntArray = DoubleList.of(
            new Label("putarray_______________"),
            new Statement(Statement.Op.ADDI, SP, SP, Register.num(-24)),
            new Statement(Statement.Op.SD, RA, _SP_),
            new Statement(Statement.Op.SD, s(3), sp(8)),
            new Statement(Statement.Op.SD, s(1), sp(16)),

            new Statement(Statement.Op.MV, s(1), a(0)),
            new Statement(Statement.Op.MV, s(3), a(1)),
            new Statement(Statement.Op.CALL, PUTINT),
            new Statement(Statement.Op.LI, a(0), Register.num(58)),
            new Statement(Statement.Op.CALL, PUTCH),
            new Label("putarray_While_0"),
            new Statement(Statement.Op.BEQZ, s(1), Register.label("putarray_While_1")),
            new Statement(Statement.Op.LI, a(0), Register.num(32)),
            new Statement(Statement.Op.CALL, PUTCH),
            new Statement(Statement.Op.LW, a(0), s(3).addr()),
            new Statement(Statement.Op.CALL, PUTINT),
            new Statement(Statement.Op.ADDI, s(3), s(3), Register.num(8)),
            new Statement(Statement.Op.ADDI, s(1), s(1), Register.num(-1)),
            new Statement(Statement.Op.J, Register.label("putarray_While_0")),
            new Label("putarray_While_1"),
            new Statement(Statement.Op.LI, a(0), Register.num(10)),
            new Statement(Statement.Op.CALL, PUTCH),

            new Statement(Statement.Op.LD, RA, _SP_),
            new Statement(Statement.Op.LD, s(3), sp(8)),
            new Statement(Statement.Op.LD, s(1), sp(16)),
            new Statement(Statement.Op.ADDI, SP, SP, Register.num(24)),
            new Statement(Statement.Op.RET)
    );

    public static final DoubleList<Assembler> putFloatArray = DoubleList.of(
            new Label("putfarray_______________"),
            new Statement(Statement.Op.ADDI, SP, SP, Register.num(-24)),
            new Statement(Statement.Op.SD, RA, _SP_),
            new Statement(Statement.Op.SD, s(3), sp(8)),
            new Statement(Statement.Op.SD, s(1), sp(16)),

            new Statement(Statement.Op.MV, s(1), a(0)),
            new Statement(Statement.Op.MV, s(3), a(1)),
            new Statement(Statement.Op.CALL, PUTINT),
            new Statement(Statement.Op.LI, a(0), Register.num(58)),
            new Statement(Statement.Op.CALL, PUTCH),
            new Label("putfarray_While_0"),
            new Statement(Statement.Op.BEQZ, s(1), Register.label("putfarray_While_1")),
            new Statement(Statement.Op.LI, a(0), Register.num(32)),
            new Statement(Statement.Op.CALL, PUTCH),
            new Statement(Statement.Op.FLW, fa(0), s(3).addr()),
            new Statement(Statement.Op.CALL, PUTFLOAT),
            new Statement(Statement.Op.ADDI, s(3), s(3), Register.num(8)),
            new Statement(Statement.Op.ADDI, s(1), s(1), Register.num(-1)),
            new Statement(Statement.Op.J, Register.label("putfarray_While_0")),
            new Label("putfarray_While_1"),
            new Statement(Statement.Op.LI, a(0), Register.num(10)),
            new Statement(Statement.Op.CALL, PUTCH),

            new Statement(Statement.Op.LD, RA, _SP_),
            new Statement(Statement.Op.LD, s(3), sp(8)),
            new Statement(Statement.Op.LD, s(1), sp(16)),
            new Statement(Statement.Op.ADDI, SP, SP, Register.num(24)),
            new Statement(Statement.Op.RET)
    );
}
