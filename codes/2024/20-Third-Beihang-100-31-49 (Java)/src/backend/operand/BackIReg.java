package backend.operand;

public class BackIReg extends BackOperand {
    private String name;

    public BackIReg(String name) {
        this.name = name;
    }

    public String getName() {
        return this.name;
    }

    @Override
    public String toString() {
        return name;
    }

    public int getRegMaxByteWidth() {
        if (name.startsWith("f")) {
            return 4;
        } else {
            return 8;
        }
    }

    public boolean isParamRegs() {
        return name.startsWith("a") || name.startsWith("fa");
    }

    public boolean isTempRegs() {
        return name.startsWith("t") || name.startsWith("ft");
    }

    public boolean isGlobalRegs() {
        return name.startsWith("s") || name.startsWith("fs");
    }
}
