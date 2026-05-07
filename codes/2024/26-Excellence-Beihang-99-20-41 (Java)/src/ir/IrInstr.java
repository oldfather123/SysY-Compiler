package ir;

import frontend.lexer.TokenType;
import ir.instr.BinaInstr;
import ir.instr.FCmpInstr;
import ir.instr.ICmpInstr;
import ir.value.Value;
import utils.GlobalCounterForInstr;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Map;

import static utils.Panic.panicIfFalse;

abstract public class IrInstr implements Cloneable {
    public enum OpCode {
        // Terminator Instructions
        RET,
        BR,
        CALL,
        // Logical Instructions
        NOT,
        // Comparison Instructions
        ICMP,
        FCMP,
        // Binary Instructions
        ADD,
        FADD,
        SUB,
        FSUB,
        MUL,
        FMUL,
        SDIV,
        FDIV,
        SREM,
        FREM,
        // Bitwise Binary Instructions
        SHL,
        LSHR,
        ASHR,
        AND,
        OR,
        // Memory Instructions
        ALLOCA,
        LOAD,
        STORE,
        // Conversion Instructions
        TRUNC,
        ZEXT,
        SEXT,
        SITOFP,
        FPTOSI,
        // Other Instructions
        MOVE,
        COMMENT, PHI;

        /**
         * 翻转运算符，不完全，仅限于简单的运算指令
         * @return 翻转后的运算符，或者原始运算符（如果无法顺利进行）
         */
        public OpCode inverse() {
            return switch (this) {
                case ADD -> SUB;
                case SUB -> ADD;
                case MUL -> SDIV;
                case SDIV -> MUL;
                case FADD -> FSUB;
                case FSUB -> FADD;
                case FMUL -> FDIV;
                case FDIV -> FMUL;
                default -> this;
            };
        }
    }

    public IrInstr copy() throws CloneNotSupportedException {
        IrInstr newInstr = (IrInstr) this.clone();
        newInstr.globalInstrId = GlobalCounterForInstr.getNewId();
        return newInstr;
    }

    public OpCode getOpCode() {
        return null;
    }

    int globalInstrId = GlobalCounterForInstr.getNewId();

    public static final Map<TokenType, OpCode> intMapper = Map.of(
        TokenType.ADD, OpCode.ADD,
        TokenType.SUB, OpCode.SUB,
        TokenType.MUL, OpCode.MUL,
        TokenType.DIV, OpCode.SDIV,
        TokenType.MOD, OpCode.SREM,
        TokenType.AND, OpCode.AND,
        TokenType.OR, OpCode.OR
    );

    public static final Map<TokenType, OpCode> floatMapper = Map.of(
        TokenType.ADD, OpCode.FADD,
        TokenType.SUB, OpCode.FSUB,
        TokenType.MUL, OpCode.FMUL,
        TokenType.DIV, OpCode.FDIV,
        TokenType.MOD, OpCode.FREM
    );

    private static IrInstr buildBinary(Value lhs, Value rhs, TokenType tokenType, Value result) {
        if (result.getType().is("INT")) {
            OpCode opCode = intMapper.get(tokenType);
            return new BinaInstr(lhs, rhs, opCode, result);
        } else if (result.getType().is("FLOAT")) {
            OpCode opCode = floatMapper.get(tokenType);
            return new BinaInstr(lhs, rhs, opCode, result);
        }
        throw new RuntimeException("Invalid binary operation: " + tokenType);
    }

    public static IrInstr spawnBinaryOp(Value lhs, Value rhs, TokenType tokenType, Value result, TokenType... allowed) {
        for (TokenType type : allowed) {
            if (tokenType == type) {
                return buildBinary(lhs, rhs, tokenType, result);
            }
        }
        throw new RuntimeException("Invalid binary operation: " + tokenType + ". Only " + Arrays.toString(allowed) + " are allowed.");
    }

    private static IrInstr spawnComparisonOp(Value lhs, Value rhs, TokenType tokenType, Value result) {
        if (lhs.getType().is("INT")) {
            return ICmpInstr.build(lhs, rhs, tokenType, result);
        } else if (lhs.getType().is("FLOAT")) {
            return FCmpInstr.build(lhs, rhs, tokenType, result);
        } else {
            throw new RuntimeException("Invalid comparison operation: " + tokenType);
        }
    }

    public static IrInstr spawnComparisonOp(Value lhs, Value rhs, TokenType tokenType, Value result, TokenType... allowed) {
        for (TokenType type : allowed) {
            if (tokenType == type) {
                return spawnComparisonOp(lhs, rhs, tokenType, result);
            }
        }
        throw new RuntimeException("Invalid comparison operation: " + tokenType + ". Only " + Arrays.toString(allowed) + " are allowed.");
    }

    public int getGlobalInstrId() {
        return globalInstrId;
    }

    public Value getInitialValue() {
        return null;
    }

    public ArrayList<Value> getDependentValues() {
        return null;
    }

    public void resetIntermediate(Value check, Value value) {
    }

    /**
     * 得到指令中的操作数，顺序自定，如有结果，放在第一个！
     * @return 全部操作数的数组
     */
    public abstract Value[] getOperands();

    /**
     * 替换操作数，注意！类型需要保持一致，name 可能会变
     * @param oldValue 要被替换的操作数
     * @param newValue 新的操作数
     */
    public abstract void replaceOperand(Value oldValue, Value newValue);

    /**
     * 检查操作数是否满足替换条件
     * @param var1 随便一个操作数
     * @param var2 另一个操作数
     * @return 是否可以替换
     */
    protected static boolean checkReplace(Value var1, Value var2) {
        return var1.getName().equals(var2.getName());
        // panicIfFalse(var1.getType().equals(var2.getType()));
    }
}
