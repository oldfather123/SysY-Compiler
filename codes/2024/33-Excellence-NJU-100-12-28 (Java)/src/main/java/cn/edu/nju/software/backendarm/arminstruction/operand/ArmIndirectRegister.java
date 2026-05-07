package cn.edu.nju.software.backendarm.arminstruction.operand;

public class ArmIndirectRegister implements ArmOperand {
    private String regName;
    private int offset;

    public ArmIndirectRegister(String regName, int offset) {
        this.regName = regName;
        this.offset = offset;
    }

    public void addOffset(int offset) {
        this.offset += offset;
    }

    @Override
    public String toString() {
        return "[" + regName + ", " + "#" + offset + "]";
    }
}
