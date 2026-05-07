package Backend.component;

public class AsmGlobalFloat extends AsmGlobalVariable {
    private static int floatNum = 0;
    private final int floatValue;

    public AsmGlobalFloat(int floatVar) {
        this.name = ".ConstFloat" + floatNum++;
        this.floatValue = floatVar;
    }

    @Override
    public String toString() {
        return SECTION_RODATA +
                name + ":\n" +
                String.format(WORD, floatValue);
    }
}
