package Backend.operand;

public class AsmReg extends AsmOperand {
    private int useFrequency = 0;

    public void addFrequency() {
        useFrequency++;
    }

    public int getUseFrequency() {
        return useFrequency;
    }
}
