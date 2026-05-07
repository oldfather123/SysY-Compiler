package cn.edu.bit.newnewcc.backend.asm.operand;

//riscv的立即数即为普通的32位整数表示
public class Immediate extends AsmOperand {
    private int value;

    public Immediate(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }

    public void setValue(int value) {
        this.value = value;
    }

    public String emit() {
        return String.valueOf(value);
    }

    @Override
    public String toString() {
        return String.format("Immediate(%d)", getValue());
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        Immediate immediate = (Immediate) o;

        return value == immediate.value;
    }

    @Override
    public int hashCode() {
        return value;
    }
}
