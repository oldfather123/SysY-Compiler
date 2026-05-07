package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;

public class ALU extends Instruction {
    private String instr;

    boolean resFromHi = false;

    public ALU(Value operand1, String operator, Value operand2, boolean isInt) {
        super(IRManager.getInstance().declareTempVar(), isInt ? ValueType.I32 : ValueType.FLT);
        use(operand1);
        use(operand2);
        switch (operator) {
            case "+" -> instr = "add";
            case "-" -> instr = "sub";
            case "*" -> instr = "mul";
            case "/" -> instr = "sdiv";
            case "%" -> instr = "srem";
            default -> instr = "UNKOWN OP";
        }
        if (!isInt) {
            instr = "f" + instr;
            if (instr.equals("fsdiv")) {
                instr = "fdiv";
            } else if (instr.equals("fsrem")) {
                instr = "frem";
            }
        }
    }


    public String toString() {
        String type = vType.toString();
        if (resFromHi) {
            return reg + " = mulhs " + type + " " + operandList.get(0).getReg()
                    + ", " + operandList.get(1).getReg() + "\n";
        }
        return reg + " = " + instr + " " + type + " " + operandList.get(0).getReg()
                + ", " + operandList.get(1).getReg() + "\n";
    }

    public String getAluType() {
        return instr;
    }

    public Value getOperand1() {
        return operandList.get(0);
    }

    public Value getOperand2() {
        return operandList.get(1);
    }

    public String getOperator() {
        return switch (instr) {
            case "add", "fadd" -> "+";
            case "sub", "fsub" -> "-";
            case "mul", "fmul" -> "*";
            case "sdiv", "fdiv" -> "/";
            case "srem", "frem" -> "%";
            default -> "UNKOWN OP";
        };
    }

    public ArrayList<String> GVNHash() {
        //ALU里+/*是符合交换律的，所以单独写一下
        String type = vType.toString();
        if (resFromHi) {
            ArrayList<String> ret = new ArrayList<>();
            ret.add("mulhs * " + operandList.get(0).getReg()
                    + ", " + operandList.get(1).getReg() + "\n");
            ret.add("mulhs * " + operandList.get(1).getReg()
                    + ", " + operandList.get(0).getReg() + "\n");
            return ret;
        }
        if (instr.equals("add") || instr.equals("mul") || instr.equals("fadd") || instr.equals("fmul")) {
            ArrayList<String> ret = new ArrayList<>();
            ret.add(instr + " " + type + " " + operandList.get(0).getReg()
                    + ", " + operandList.get(1).getReg() + "\n");
            ret.add(instr + " " + type + " " + operandList.get(1).getReg()
                    + ", " + operandList.get(0).getReg() + "\n");
            return ret;
        } else {
            return super.GVNHash();
        }
    }

    public boolean isConst() {
        boolean flag = true;
        if (instr.equals("sdiv") || instr.equals("srem") || instr.equals("fdiv") || instr.equals("frem")) {
            flag = operandList.get(1) instanceof ConstNumber n && n.getVal().floatValue() != 0;
        }
        return operandList.get(0) instanceof ConstNumber &&
                operandList.get(1) instanceof ConstNumber && flag;
    }

    public ConstNumber toConstNumber() {
        assert operandList.get(0) instanceof ConstNumber;
        assert operandList.get(1) instanceof ConstNumber;
        if (isFloat()) {
            float v1 = ((ConstNumber) operandList.get(0)).getVal().floatValue();
            float v2 = ((ConstNumber) operandList.get(1)).getVal().floatValue();
            if (v2 == 0 && (instr.equals("fdiv") || instr.equals("frem"))) {
                return new ConstNumber(0.0);
            }
            float number = switch (instr) {
                case "fadd" -> v1 + v2;
                case "fsub" -> v1 - v2;
                case "fmul" -> v1 * v2;
                case "fdiv" -> v1 / v2;
                case "frem" -> v1 % v2;
                default -> 0;
            };

            return (ConstNumber) new ConstNumber(number).withType(!isFloat());
        } else {
            int v1 = ((ConstNumber) operandList.get(0)).getVal().intValue();
            int v2 = ((ConstNumber) operandList.get(1)).getVal().intValue();
            if (v2 == 0 && (instr.equals("sdiv") || instr.equals("srem"))) {
                return new ConstNumber(0);
            }
            if (resFromHi && (instr.equals("mul"))) {
                return new ConstNumber(((long) v1 * (long) v2) >> 32);
            }
            int number = switch (instr) {
                case "add" -> v1 + v2;
                case "sub" -> v1 - v2;
                case "mul" -> v1 * v2;
                case "sdiv" -> v1 / v2;
                case "srem" -> v1 % v2;
                default -> 0;
            };

            return (ConstNumber) new ConstNumber(number).withType(!isFloat());
        }
    }

    public void setResFromHi(boolean resFromHi) {
        this.resFromHi = resFromHi;
    }

    public Boolean getResFromHi() {
        return this.resFromHi;
    }
}
