package backend.component;

public class RiscvGlobalFloat implements RiscvGlobalElement{
    private final float value;

    public RiscvGlobalFloat(float value) {
        this.value = value;
    }

    @Override
    public String toString() {
        return "\t.word\t" + Float.floatToIntBits(value) + "\n";
    }
}
