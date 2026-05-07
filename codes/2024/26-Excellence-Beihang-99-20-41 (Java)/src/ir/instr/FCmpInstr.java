package ir.instr;

import frontend.lexer.TokenType;
import ir.IrInstr;
import ir.value.Value;

import java.util.ArrayList;

// 浮点数比较指令
public class FCmpInstr extends IrInstr {
    public enum Predicate {
        oeq, ogt, oge, olt, ole, one, ord, ueq, ugt, uge, ult, ule, une, uno
    }
    private Value lhs;
    private Value rhs;
    private Predicate predicate;
    private Value result;
    OpCode opCode = OpCode.FCMP;

    public FCmpInstr(Value lhs, Value rhs, Predicate predicate, Value result) {
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
        return String.format("%s = fcmp %s %s %s, %s",
                result.getName(), predicate.toString(), lhs.getType(), lhs.getName(), rhs.getName());
    }

    public static FCmpInstr build(Value lhs, Value rhs, TokenType tokenType, Value result) {
        return switch (tokenType) {
            case EQ -> new FCmpInstr(lhs, rhs, Predicate.oeq, result);
            case NEQ -> new FCmpInstr(lhs, rhs, Predicate.one, result);
            case GT -> new FCmpInstr(lhs, rhs, Predicate.ogt, result);
            case GE -> new FCmpInstr(lhs, rhs, Predicate.oge, result);
            case LT -> new FCmpInstr(lhs, rhs, Predicate.olt, result);
            case LE -> new FCmpInstr(lhs, rhs, Predicate.ole, result);
            default -> throw new RuntimeException("Invalid token type for FCmpInstr");
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
