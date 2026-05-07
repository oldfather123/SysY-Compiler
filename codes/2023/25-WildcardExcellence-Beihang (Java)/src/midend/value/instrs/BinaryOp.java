package midend.value.instrs;

import midend.Type;
import midend.value.BasicBlock;
import midend.value.Constant;
import midend.value.Value;

import java.util.HashMap;

public class BinaryOp extends Reg {

    HashMap<Op, Op> IopToF = new HashMap<>(){{
        put(Op.Add, Op.FAdd);
        put(Op.Sub, Op.FSub);
        put(Op.Mul, Op.FMul);
        put(Op.Div, Op.FDiv);
        put(Op.Mod, Op.FMod);
    }};

    public BinaryOp(Boolean isI1, Value first, Op op,
                    Value second, BasicBlock parent) {
        super(parent);
        boolean LisFloat = first.getType().isFloatType();
        boolean RisFloat = second.getType().isFloatType();
        boolean LisI32 = first.getType().isIntegerType() && ((Type.IntegerType) first.getType()).isI32();
        boolean RisI32 = second.getType().isIntegerType() && ((Type.IntegerType) second.getType()).isI32();
        boolean LisI1 = first.getType().isIntegerType() && ((Type.IntegerType) first.getType()).isI1();
        boolean RisI1 = second.getType().isIntegerType() && ((Type.IntegerType) second.getType()).isI1();
        boolean isSameType = (LisFloat && RisFloat) || (LisI32 && RisI32) || (LisI1 && RisI1);

        if (!isSameType) {
            if (LisI1) {
                if (RisI32) {
                    first = I1toI32(first, parent);
                } else if (RisFloat) {
                    Value tmp = I1toI32(first, parent);
                    first = I32toF(tmp, parent);
                }
            } else if (LisI32) {
                if (RisI1) {
                    second = I1toI32(second, parent);
                } else if (RisFloat) {
                    first = I32toF(first, parent);
                }
            } else if (LisFloat) {
                if (RisI1) {
                    Value tmp = I1toI32(second, parent);
                    second = I32toF(tmp, parent);
                } else if (RisI32) {
                    second = I32toF(second, parent);
                }
            }
        }

        if (first.getType().isFloatType() && IopToF.containsKey(op)) {
            op = IopToF.get(op);
        }
        this.op = op;
        this.getOperandList().add(first);
        this.addUseOp(first, 0);
        this.getOperandList().add(second);
        this.addUseOp(second, 1);

        if (isI1) {
            setType(Type.IntegerType.I1);
        } else {
            setType(first.getType());
        }
        this.opType = first.getType();

        this.getParent().addInstr(this);
    }

    private final Op op; // op
    private final Type opType; // FirsyOp's type

    public int eval(int a, int b, Op op) {
        switch (op) {
            case Add:
                return a + b;
            case Sub:
                return a - b;
            case Mul:
                return a * b;
            case Div:
                return a / b;
            case Mod:
                return a % b;
            case Le:
                return (a <= b) ? 1 : 0;
            case Lt:
                return (a < b) ? 1 : 0;
            case Ge:
                return (a >= b) ? 1 : 0;
            case Gt:
                return (a > b) ? 1 : 0;
            case Eq:
                return (a == b) ? 1 : 0;
            case Ne:
                return (a != b) ? 1 : 0;
            default:
                break;
        }
        return 0;
    }

    public float evalFloat(float a, float b, Op op) {
        switch (op) {
            case FAdd:
                return a + b;
            case FSub:
                return a - b;
            case FMul:
                return a * b;
            case FDiv:
                return a / b;
            case FMod:
                return a % b;
            case Le:
                return (float) ((a <= b) ? 1 : 0);
            case Lt:
                return (float) ((a < b) ? 1 : 0);
            case Ge:
                return (float) ((a >= b) ? 1 : 0);
            case Gt:
                return (float) ((a > b) ? 1 : 0);
            case Eq:
                return (float) ((a == b) ? 1 : 0);
            case Ne:
                return (float) ((a != b) ? 1 : 0);
            default:
                break;
        }
        return 0;
    }

    /**
        二元表达式可以合并的情况有：
        1.两个运算对象都是常数
        2.加减乘运算，其中一个操作数为0，简化
        3.乘法运算，其中一个操作数为1
        4.除法运算，其中除数为1，或被除数为0
     */

    public Value evaluate() {
        Value first = this.getOperandList().get(0);
        Value second = this.getOperandList().get(1);
        Value ret = null;
        if (first instanceof Constant && second instanceof Constant) {
            if (opType.isIntegerType()) {
                int a = ((Constant.ConstantInteger) first).getConstantIntegerValue();
                int b = ((Constant.ConstantInteger) second).getConstantIntegerValue();
                int res = eval(a, b, op);
                return new Constant.ConstantInteger(getType(), String.valueOf(res));
            } else if (opType.isFloatType()) {
                float a = ((Constant.ConstantFloat) first).getConstantFloatValue();
                float b = ((Constant.ConstantFloat) second).getConstantFloatValue();
                float res = evalFloat(a, b, op);
                if (getType().isIntegerType()) {
                    return new Constant.ConstantInteger(getType(), String.valueOf((int)res));
                } else {
                    return new Constant.ConstantFloat(getType(), res);
                }
            }
        } else {
            if (op.equals(Op.Add)) {
                ret = getTheSecondWhenTheFirstIsIntTar(first, 0, second);
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsIntTar(second, 0, first);
            } else if (op.equals(Op.FAdd)) {
                ret = getTheSecondWhenTheFirstIsFloatTar(first, 0, second);
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsFloatTar(second, 0, first);
            } else if (op.equals(Op.Sub)) {
                ret = getTheSecondWhenTheFirstIsIntTar(second, 0, first);
            } else if (op.equals(Op.FSub)) {
                ret = getTheSecondWhenTheFirstIsFloatTar(second, 0, first);
            } else if (op.equals(Op.Mul)) {
                ret = getTheSecondWhenTheFirstIsIntTar(first, 0, new Constant.ConstantInteger(getType(), "0"));
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsIntTar(first, 1, second);
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsIntTar(second, 0, new Constant.ConstantInteger(getType(), "0"));
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsIntTar(second, 1, first);
            } else if (op.equals(Op.FMul)) {
                ret = getTheSecondWhenTheFirstIsFloatTar(first, 0, new Constant.ConstantFloat(getType(), 0.0F));
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsFloatTar(first, 1, second);
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsFloatTar(second, 0, new Constant.ConstantFloat(getType(), 0.0F));
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsFloatTar(second, 1, first);
            } else if (op.equals(Op.Div)) {
                ret = getTheSecondWhenTheFirstIsIntTar(second, 1, first);
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsIntTar(first, 0, new Constant.ConstantInteger(getType(), "0"));
            } else if (op.equals(Op.FDiv)) {
                ret = getTheSecondWhenTheFirstIsFloatTar(second, 1, first);
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsFloatTar(first, 0, new Constant.ConstantFloat(getType(), 0.0F));
            } else if (op.equals(Op.Mod)) {
                ret = getTheSecondWhenTheFirstIsIntTar(second, 1, new Constant.ConstantInteger(getType(), "0"));
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsIntTar(second, -1, new Constant.ConstantInteger(getType(), "0"));
            } else if (op.equals(Op.FMod)) {
                ret = getTheSecondWhenTheFirstIsFloatTar(second, 1, new Constant.ConstantFloat(getType(), 0.0F));
                if (ret != null) {
                    return ret;
                }
                ret = getTheSecondWhenTheFirstIsFloatTar(second, -1, new Constant.ConstantFloat(getType(), 0.0F));
            }
        }
        return ret;
    }

    public Value getTheSecondWhenTheFirstIsIntTar(Value first, int tar, Value second) {
        if (first instanceof Constant.ConstantInteger) {
            int a = ((Constant.ConstantInteger) first).getConstantIntegerValue();
            if (a == tar) {
                return second;
            }
        }
        return null;
    }

    public Value getTheSecondWhenTheFirstIsFloatTar(Value first, float tar, Value second) {
        if (first instanceof Constant.ConstantFloat) {
            float a = ((Constant.ConstantFloat) first).getConstantFloatValue();
            if (a == tar) {
                return second;
            }
        }
        return null;
    }

    public String unEqualLR() {
        Value first = this.getOperandList().get(0);
        Value second = this.getOperandList().get(1);
        if (!first.getName().equals(second.getName())) {
            return getHash(1, 0);
        }
        return null;
    }

    public Op getOp() {
        return op;
    }

    public Type getOpType() {
        return opType;
    }

    public String getHash(int i1, int i2) {
        StringBuilder sb = new StringBuilder();
        sb.append(getName());
        sb.append(" = ");
        switch (op) {
            case Add -> sb.append("add ");
            case FAdd -> sb.append("fadd ");
            case FMul -> sb.append("fmul ");
            case FSub -> sb.append("fsub ");
            case FDiv -> sb.append("fdiv ");
            case FMod -> sb.append("fmod ");
            case Sub -> sb.append("sub ");
            case Mul -> sb.append("mul ");
            case Div -> sb.append("sdiv ");
            case Mod -> sb.append("srem ");
            case Le -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp ole ");
                } else {
                    sb.append("icmp sle ");
                }
            }
            case Lt -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp olt ");
                } else {
                    sb.append("icmp slt ");
                }
            }
            case Ge -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp oge ");
                } else {
                    sb.append("icmp sge ");
                }
            }
            case Gt -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp ogt ");
                } else {
                    sb.append("icmp sgt ");
                }
            }
            case Eq -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp oeq ");
                } else {
                    sb.append("icmp eq ");
                }
            }
            case Ne -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp one ");
                } else {
                    sb.append("icmp ne ");
                }
            }
            default -> {
            }
        }
        sb.append(opType.toString()).append(" ");
        sb.append(this.getOperandList().get(i1).getName()).append(", ");
        sb.append(this.getOperandList().get(i2).getName());
        return sb.toString();
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(getName());
        sb.append(" = ");
        switch (op) {
            case Add -> sb.append("add ");
            case FAdd -> sb.append("fadd ");
            case FMul -> sb.append("fmul ");
            case FSub -> sb.append("fsub ");
            case FDiv -> sb.append("fdiv ");
            case FMod -> sb.append("fmod ");
            case Sub -> sb.append("sub ");
            case Mul -> sb.append("mul ");
            case Div -> sb.append("sdiv ");
            case Mod -> sb.append("srem ");
            case Le -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp ole ");
                } else {
                    sb.append("icmp sle ");
                }
            }
            case Lt -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp olt ");
                } else {
                    sb.append("icmp slt ");
                }
            }
            case Ge -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp oge ");
                } else {
                    sb.append("icmp sge ");
                }
            }
            case Gt -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp ogt ");
                } else {
                    sb.append("icmp sgt ");
                }
            }
            case Eq -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp oeq ");
                } else {
                    sb.append("icmp eq ");
                }
            }
            case Ne -> {
                if (getOpType().isFloatType()) {
                    sb.append("fcmp one ");
                } else {
                    sb.append("icmp ne ");
                }
            }
            default -> {
            }
        }
        sb.append(opType.toString()).append(" ");
        sb.append(this.getOperandList().get(0).getName()).append(", ");
        sb.append(this.getOperandList().get(1).getName());
        return sb.toString();
    }
}
