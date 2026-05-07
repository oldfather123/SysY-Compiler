package entities;

import java.util.ArrayList;
import java.util.List;

public class Registers {

    private static final List<Register> registerR = new ArrayList<>();
    private static final List<Register> registerF = new ArrayList<>();
    private static final List<Register> registerA = new ArrayList<>();
    private static final List<Register> registerFA = new ArrayList<>();
    public static final int T_SIZE = 7, S_SIZE = 11, FT_SIZE = 12, FS_SIZE = 12;
    public static final int R_SIZE = T_SIZE + S_SIZE, F_SIZE = FT_SIZE + FS_SIZE, ALL_SIZE = R_SIZE + F_SIZE;
    public static final Register SP = Register.sp(), RA = Register.ra(), _SP_ = Register.sp().addr();

    static {
        for (int i = 0; i < 5; i++) {
            registerA.add(Register.a(i));
            registerFA.add(Register.fa(i));
        }
        for (int i = 0; i < FT_SIZE; ++i) {
            registerF.add(Register.ft(i));
        }
        for (int i = 0; i < FS_SIZE; ++i) {
            registerF.add(Register.fs(i));
        }
        for (int i = 0; i < T_SIZE; ++i) {
            registerR.add(Register.t(i));
        }
        for (int i = 1; i <= S_SIZE; ++i) {
            registerR.add(Register.s(i));
        }
    }

    /**
     * t0 - t6
     */
    public static Register t(int num) {
        return registerR.get(num);
    }

    /**
     * s1 - s11
     */
    public static Register s(int num) {
        return registerR.get(num + T_SIZE - 1);
    }

    /**
     * ft0 - ft11
     */
    public static Register ft(int num) {
        return registerF.get(num);
    }

    /**
     * fs0 - fs11
     */
    public static Register fs(int num) {
        return registerF.get(num + FT_SIZE);
    }

    public static Register r(int num) {
        return registerR.get(num);
    }

    public static Register a(int num) {
        return registerA.get(num);
    }

    public static Register f(int num) {
        return registerF.get(num);
    }

    public static Register fa(int num) {
        return registerFA.get(num);
    }

    public static Register sp(int num) {
        return _SP_.copy().setDelta(num);
    }

}
