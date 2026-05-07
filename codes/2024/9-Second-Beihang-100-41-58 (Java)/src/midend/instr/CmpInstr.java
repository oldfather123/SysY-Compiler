package midend.instr;

import midend.BasicBlock;
import midend.Function;
import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.Type;
import midend.Value;

import java.util.Arrays;
import java.util.List;

public class CmpInstr extends ALUInstr {
    private OP op;
    private String opStr;

    public CmpInstr(String op, Type type, List<Value> values, InstrType instrType, BasicBlock block) {
        super(type, Arrays.asList(values.get(0), values.get(1)), instrType, block);
        this.opStr = op;
//        super.addValue(left);
//        super.addValue(right);
        this.op = switch (op) {
            case "==" ->  OP.EQ;
            case "!=" -> OP.NE;
            case "<" -> OP.SLT;
            case "<=" -> OP.SLE;
            case ">" -> OP.SGT;
            case ">="-> OP.SGE;
            default -> throw new IllegalStateException("Unexpected value: " + op);
        };
        if (values.get(0).getType() == FloatType.f32) {
            this.op = switch (this.op) {
                case EQ -> OP.OEQ;
                case NE -> OP.ONE;
                case SLT -> OP.OLT;
                case SLE -> OP.OLE;
                case SGT -> OP.OGT;
                case SGE -> OP.OGE;
                case OEQ -> OP.OEQ;
                case ONE -> OP.ONE;
                case OLT -> OP.OLT;
                case OLE -> OP.OLE;
                case OGT -> OP.OGT;
                case OGE -> OP.OGE;
            };
        }
    }

    @Override
    public boolean canUse() {
        return true;
    }

    public String getOpStr() {
        return this.opStr;
    }

    public Value getLeft() {
        return super.getValue(0);
    }

    public Value getRight() {
        return super.getValue(1);
    }

    public OP getOp() {
        return this.op;
    }

    public String getInstr() {
        StringBuilder builder = new StringBuilder();
        builder.append(this.getName() + " = " + getInstrType().toString() + " ");
        builder.append(this.op.toString().toLowerCase() + " ");
        Value left = getLeft();
        Value right = getRight();
        String type = null;
        if (left.getType() == IntegerType.i32) {
            type = "i32 ";
        } else if (left.getType() == FloatType.f32) {
            type = "float ";
        } else if (left.getType() == IntegerType.i1) {
            type = "i1 ";
        }
        builder.append(type + left.getName() + ", " + right.getName()).append("\n");
        return builder.toString();
    }

    @Override
    public CmpInstr clone(BasicBlock block) {
        Value left = getLeft();
        Value right = getRight();
        if (Function.cloneMap.containsKey(left)) {
            left = Function.cloneMap.get(left);
        }
        if (Function.cloneMap.containsKey(right)) {
            right = Function.cloneMap.get(right);
        }
        return new CmpInstr(opStr, this.getType(), Arrays.asList(left, right), this.getInstrType(), block);
    }

    @Override
    public boolean canBeUsed() {
        return true;
    }
}
