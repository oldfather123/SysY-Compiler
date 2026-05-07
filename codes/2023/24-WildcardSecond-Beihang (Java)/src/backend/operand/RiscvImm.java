package backend.operand;

import ir.value.ConstNumber;

import java.util.ArrayList;

public class RiscvImm extends RiscvOperand{
    private int value;

    public int getValue() {
        return value;
    }

    public RiscvImm(ConstNumber number) {
        super();
        assert number.value instanceof Integer;
        value = (int)number.value;
    }

    public RiscvImm(int number) {
        super();
        value = number;
    }

    public RiscvImm() {
        value = 1;
    }

    public void opposite() {
        value = -1 * value;
    }

    @Override
    public String toString() {
        return Integer.toString(value);
    }
}
