package entities;

import java.util.ArrayList;
import java.util.List;

public class Quaternion {

    private final Operator op;

    private final List<Val> vals;

    public Quaternion(Operator op, Val ...val) {
        this.op = op;
        this.vals = new ArrayList<>(List.of(val));
    }

    public Operator getOp() {
        return op;
    }

    public List<Val> getVals() {
        return vals;
    }

    public int getValCount() {
        return vals.size();
    }

    public Val getVal(int index) {
        return vals.get(index);
    }

    public String getName(int index) {
        return vals.get(index).getName();
    }

    public enum Operator {
        ASSIGN,
        GET_RETURN_VALUE,
        CALL,
        RETURN,
        SAVE_ARG, SET_ARG, ADD_SP,
        PLUS, MINU, MULT, DIV, MOD,
        JUMP,
        DEFINE_LABEL,
        SLT, SLE, SGT, SGE, SEQ, SNE,
        EQZ,
        DECLARE,
        DECLARE_ARG,
    }

    @Override
    public String toString() {
        return "op=" + op + ", vals=" + vals;
    }
}
