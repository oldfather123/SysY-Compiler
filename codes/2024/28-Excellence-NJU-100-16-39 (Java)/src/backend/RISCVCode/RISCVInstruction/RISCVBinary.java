package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.RISCVOperand;

public class RISCVBinary extends RISCVInstruction {
    private final RISCVOperand dest;

    private final RISCVOperand operand1;

    private final RISCVOperand operand2;

    private final String opt;

//    fadd.s rd, rs1, rs2：单精度浮点加法。
//    fsub.s rd, rs1, rs2：单精度浮点减法。
//    fmul.s rd, rs1, rs2：单精度浮点乘法。
//    fdiv.s rd, rs1, rs2：单精度浮点除法。
    public RISCVBinary(RISCVOperand dest, RISCVOperand operand1, RISCVOperand operand2, String opt) {
        this.dest = dest;
        this.operand1 = operand1;
        this.operand2 = operand2;
        this.opt = opt;
    }

    @Override
    public String getString() {
        if (opt.equals("addi") && !operand2.getName().startsWith("%lo")) {

            int stackSize = Integer.parseInt(operand2.getName());
            if (stackSize < 0) {
                int a = 1;
            }
            if (stackSize > 2047 || stackSize < -2048) {
                return "  li s0, " + stackSize + "\n" +
                        " add " + dest.getName() + ", " + operand1.getName() + ", s0\n";
            }
        }
        return "  " + opt + " " + dest.getName() + ", " + operand1.getName() + ", " + operand2.getName() + "\n";
    }

    public String getOpt() {
        return opt;
    }
    public RISCVOperand getDest() {
        return dest;
    }
    public RISCVOperand getSrc1() {
            return operand1;
    }
    public RISCVOperand getSrc2() {
        return operand2;
    }
}
