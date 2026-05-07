package backend.component;

public class RiscvGlobalInt implements RiscvGlobalElement{

    private final int value;

    public int getValue() {
        return value;
    }

    public RiscvGlobalInt(int value) {
        this.value = value;
    }

    @Override
    public String toString() {
        return "\t.word\t" + value + "\n";
    }
}
