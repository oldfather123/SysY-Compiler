package ir.instr;

import frontend.lexer.TokenType;
import ir.IrInstr;
import ir.value.Value;

import java.util.ArrayList;

// 比较指令
public class ICmpInstr extends IrInstr {
    public enum Predicate {
        eq, ne, slt, sgt, sle, sge, ult, ugt, ule, uge
    }

    private Value lhs;
    private Value rhs;
    public Predicate predicate;
    private Value result;
    OpCode opCode = OpCode.ICMP;

    public ICmpInstr(Value lhs, Value rhs, Predicate predicate, Value result) {
        this.lhs = lhs;
        this.rhs = rhs;
        this.predicate = predicate;
        this.result = result;
    }

    public void resetIntermediate(Value check, Value value) {
        if (this.lhs.getName().equals(check.getName())) {
            this.lhs = value;
        }
        if (this.rhs.getName().equals(check.getName())) {
            this.rhs = value;
        }
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        return new ArrayList<Value>() {{
            add(lhs);
            add(rhs);
        }};
    }

    @Override
    public OpCode getOpCode() {
        return opCode;
    }

    @Override
    public Value getInitialValue() {
        return result;
    }

    @Override
    public String toString() {
        return String.format("%s = icmp %s %s %s, %s",
                result.getName(), predicate.toString(), lhs.getType(), lhs.getName(), rhs.getName());
    }

    public static ICmpInstr build(Value lhs, Value rhs, TokenType tokenType, Value result) {
        return switch (tokenType) {
            case EQ -> new ICmpInstr(lhs, rhs, Predicate.eq, result);
            case NEQ -> new ICmpInstr(lhs, rhs, Predicate.ne, result);
            case GT -> new ICmpInstr(lhs, rhs, Predicate.sgt, result);
            case GE -> new ICmpInstr(lhs, rhs, Predicate.sge, result);
            case LT -> new ICmpInstr(lhs, rhs, Predicate.slt, result);
            case LE -> new ICmpInstr(lhs, rhs, Predicate.sle, result);
            default -> throw new RuntimeException("Invalid token type for ICmpInstr");
        };
    }

    public Predicate getPredicate() {
        return predicate;
    }

    public Value[] getOperands() {
        return new Value[]{result, lhs, rhs};
    }

    public void replaceOperand(Value old, Value newv) {
        if (IrInstr.checkReplace(old, result)) {
            result = newv;
        }
        if (IrInstr.checkReplace(old, lhs)) {
            lhs = newv;
        }
        if (IrInstr.checkReplace(old, rhs)) {
            rhs = newv;
        }
    }
}
