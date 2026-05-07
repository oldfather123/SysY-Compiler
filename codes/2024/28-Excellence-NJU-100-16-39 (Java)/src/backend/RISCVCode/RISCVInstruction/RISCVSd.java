package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.RISCVOperand;

public class RISCVSd extends RISCVInstruction {
    private final RISCVOperand dest;

    private final RISCVOperand operand1;

    private final String opt;

    public RISCVSd(RISCVOperand dest, RISCVOperand operand1) {
        this.dest = dest;
        this.operand1 = operand1;
        if (dest.getName().startsWith("f"))
            opt = "fsw";
        else
            opt = "sd";

    }

    @Override
    public String getString() {
        int stackSize = Integer.parseInt(operand1.getName().substring(0, operand1.getName().length() - 4));
        String baseReg = operand1.getName().substring(operand1.getName().length() - 3, operand1.getName().length() - 1);
        if (stackSize > 2047) {
            return "  li s0, " + stackSize + "\n" +
                    "  add s0, " + baseReg + ", s0\n" +
                    "  " +opt + " " + dest.getName() + ", 0(s0)\n";
        }
        return "  " + opt + " " + dest.getName() + ", " + operand1.getName() + "\n";
    }


    public RISCVOperand getDest() {
        return this.dest;
    }

    public RISCVOperand getOperand1() {
        return this.operand1;
    }
}
