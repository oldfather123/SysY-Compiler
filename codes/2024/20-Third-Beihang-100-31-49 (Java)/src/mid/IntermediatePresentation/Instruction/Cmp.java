package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;

public class Cmp extends Instruction {
    private final String cond;

    public Cmp(String op, Value oprand1, Value oprand2) {
        super(IRManager.getInstance().declareTempVar(), ValueType.I1);
        use(oprand1);
        use(oprand2);
        boolean isFloat = oprand1.isFloat() || oprand2.isFloat();
        cond = switch (op) {
            case "<" -> isFloat ? "olt" : "slt";
            case "<=" -> isFloat ? "ole" : "sle";
            case ">" -> isFloat ? "ogt" : "sgt";
            case ">=" -> isFloat ? "oge" : "sge";
            case "==" -> isFloat ? "oeq" : "eq";
            case "!=" -> isFloat ? "one" : "ne";
            default -> "UNKOWNCMPOP";
        };
    }

    public String toString() {
        String type = operandList.get(0).getTypeString();
        String prefix = type.substring(0, 1);
        return reg + " = " + prefix + "cmp " + cond + " " + type + " " + operandList.get(0).getReg() +
                ", " + operandList.get(1).getReg() + "\n";
    }

    public ArrayList<String> GVNHash() {
        ArrayList<String> ret = new ArrayList<>();
        String type = operandList.get(0).getTypeString();
        String prefix = type.substring(0, 1);
        switch (cond) {
            case "slt":
                ret.add(prefix + "cmp " + cond + " " + type + " " + operandList.get(0).getReg() +
                        ", " + operandList.get(1).getReg() + "\n");
                ret.add(prefix + "cmp " + "sgt" + " " + type + " " + operandList.get(1).getReg() +
                        ", " + operandList.get(0).getReg() + "\n");
                break;
            case "sle":
                ret.add(prefix + "cmp " + cond + " " + type + " " + operandList.get(0).getReg() +
                        ", " + operandList.get(1).getReg() + "\n");
                ret.add(prefix + "cmp " + "sge" + " " + type + " " + operandList.get(1).getReg() +
                        ", " + operandList.get(0).getReg() + "\n");
                break;
            case "sgt":
                ret.add(prefix + "cmp " + cond + " " + type + " " + operandList.get(0).getReg() +
                        ", " + operandList.get(1).getReg() + "\n");
                ret.add(prefix + "cmp " + "slt" + " " + type + " " + operandList.get(1).getReg() +
                        ", " + operandList.get(0).getReg() + "\n");
                break;
            case "sge":
                ret.add(prefix + "cmp " + cond + " " + type + " " + operandList.get(0).getReg() +
                        ", " + operandList.get(1).getReg() + "\n");
                ret.add(prefix + "cmp " + "sle" + " " + type + " " + operandList.get(1).getReg() +
                        ", " + operandList.get(0).getReg() + "\n");
                break;
            case "eq":
                ret.add(prefix + "cmp " + cond + " " + type + " " + operandList.get(0).getReg() +
                        ", " + operandList.get(1).getReg() + "\n");
                ret.add(prefix + "cmp " + "eq" + " " + type + " " + operandList.get(1).getReg() +
                        ", " + operandList.get(0).getReg() + "\n");
                break;
            case "ne":
                ret.add(prefix + "cmp " + cond + " " + type + " " + operandList.get(0).getReg() +
                        ", " + operandList.get(1).getReg() + "\n");
                ret.add(prefix + "cmp " + "ne" + " " + type + " " + operandList.get(1).getReg() +
                        ", " + operandList.get(0).getReg() + "\n");
                break;
            default:
                return super.GVNHash();
        }
        return ret;
    }

    public boolean isConst() {
        return operandList.get(0) instanceof ConstNumber && operandList.get(1) instanceof ConstNumber;
    }

    public String getCond() {
        return cond;
    }

    public String getOperator() {
        return switch (cond) {
            case "sgt", "ogt" -> ">";
            case "sle", "ole" -> "<=";
            case "sge", "oge" -> ">=";
            case "eq", "oeq" -> "==";
            case "ne", "one" -> "!=";
            case "slt", "olt" -> "<";
            default -> "";
        };
    }

    public static String getOppsiteOperator(String op) {
        return switch (op) {
            case ">" -> "<";
            case "<" -> ">";
            case "<=" -> ">=";
            case ">=" -> "<=";
            case "==" -> "==";
            case "!=" -> "!=";
            default -> "";
        };
    }

    public ConstNumber toConstNumber() {
        assert operandList.get(0) instanceof ConstNumber;
        assert operandList.get(1) instanceof ConstNumber;
        Number val1 = ((ConstNumber) operandList.get(0)).getVal();
        Number val2 = ((ConstNumber) operandList.get(1)).getVal();
        boolean res = switch (cond) {
            case "sgt" -> val1.intValue() > val2.intValue();
            case "sle" -> val1.intValue() <= val2.intValue();
            case "sge" -> val1.intValue() >= val2.intValue();
            case "eq" -> val1.intValue() == val2.intValue();
            case "ne" -> val1.intValue() != val2.intValue();
            case "slt" -> val1.intValue() < val2.intValue();
            case "ogt" -> val1.floatValue() > val2.floatValue();
            case "ole" -> val1.floatValue() <= val2.floatValue();
            case "oge" -> val1.floatValue() >= val2.floatValue();
            case "oeq" -> val1.floatValue() == val2.floatValue();
            case "one" -> val1.floatValue() != val2.floatValue();
            case "olt" -> val1.floatValue() < val2.floatValue();
            default -> false;
        };
        if (res) {
            return new ConstNumber(1);
        } else {
            return new ConstNumber(0);
        }
    }

    public static String getOppositeOperator2(String op) {
        return switch (op) {
            case ">" -> "<=";
            case "<" -> ">=";
            case "<=" -> ">";
            case ">=" -> "<";
            case "==" -> "!=";
            case "!=" -> "=";
            default -> "";
        };
    }
}
