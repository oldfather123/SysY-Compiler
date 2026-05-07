package backend.itemStructure;

public class AsmImm32 extends AsmOperand{
    private int value;
    public AsmImm32(int value) {
        this.value = value;
    }
    public int getValue() {
        return value;
    }
    public String toString() {
        return String.valueOf(value);
    }
}
