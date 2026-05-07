package utils;

import ir.IrInstr;
import ir.instr.*;
import ir.value.Literal;

import java.util.ArrayList;

import static ir.type.Ty.I1;
import static utils.Calculator.Add;
import static utils.Calculator.Sub;

public class CalculatorInstr {
    public static Literal compute(IrInstr.OpCode op, ArrayList<Literal> value, IrInstr instr, int mode) {
        Literal a = value.get(0);
        Literal b = value.get(1);

        return switch (op) {
            case ADD -> new Literal((Integer) Add(a.asInt(), b.asInt()), a.getType());
            case SUB -> new Literal((Integer) Sub(a.asInt(), b.asInt()), a.getType());
            case MUL -> new Literal((Integer) Calculator.Mul(a.asInt(), b.asInt()), a.getType());
            case SDIV -> new Literal((Integer) Calculator.Div(a.asInt(), b.asInt()), a.getType());
            case SHL -> new Literal(a.asInt() << b.asInt(), a.getType());
            case LSHR -> new Literal(a.asInt() >>> b.asInt(), a.getType());
            case ASHR -> new Literal(a.asInt() >> b.asInt(), a.getType());
            case AND -> new Literal(a.asInt() & b.asInt(), a.getType());
            case OR -> new Literal(a.asInt() | b.asInt(), a.getType());
            case FADD -> new Literal((Float) Add(a.asFloat(), b.asFloat()));
            case FSUB -> new Literal((Float) Sub(a.asFloat(), b.asFloat()));
            case FMUL -> new Literal((Float) Calculator.Mul(a.asFloat(), b.asFloat()));
            case FDIV -> new Literal((Float) Calculator.Div(a.asFloat(), b.asFloat()));
            case ICMP -> {
                Literal ans = null;
                switch (((ICmpInstr)instr).predicate) {
                    case eq -> ans =new Literal((a.asInt() == b.asInt())?1:0, I1);
                    case ne -> ans =new Literal((a.asInt() != b.asInt())?1:0, I1);
                    case slt -> ans =new Literal((a.asInt() < b.asInt())?1:0, I1);
                    case sgt -> ans =new Literal((a.asInt() > b.asInt())?1:0, I1);
                    case sle -> ans =new Literal((a.asInt() <= b.asInt())?1:0, I1);
                    case sge -> ans =new Literal((a.asInt() >= b.asInt())?1:0, I1);
                }
                yield ans;
            }
            case FCMP -> {
                Literal ans = null;
                switch (((FCmpInstr)instr).getPredicate()) {
                    case oeq -> ans =new Literal((a.asFloat() == b.asFloat())?1:0, I1);
                    case ogt -> ans =new Literal((a.asFloat() > b.asFloat())?1:0, I1);
                    case oge -> ans =new Literal((a.asFloat() >= b.asFloat())?1:0, I1);
                    case olt -> ans =new Literal((a.asFloat() < b.asFloat())?1:0, I1);
                    case ole -> ans =new Literal((a.asFloat() <= b.asFloat())?1:0, I1);
                    case one -> ans =new Literal((a.asFloat() != b.asFloat())?1:0, I1);
                }
                yield ans;
            }
            case SREM -> new Literal(a.asInt() % b.asInt(), a.getType());
            case FREM -> new Literal(a.asFloat() % b.asFloat());
            default -> throw new RuntimeException("Unknown opcode");
        };
    }

    public static Literal compute(IrInstr.OpCode op, ArrayList<Literal> v, IrInstr instr) {
        Literal a = v.get(0);
        return switch (op) {
            case NOT -> new Literal(~a.asInt(), a.getType());
            case TRUNC -> new Literal(a.asInt(), ((TruncInstr) instr).toType);
            case SITOFP -> new Literal((float) a.asInt());
            case FPTOSI -> new Literal((int) a.asFloat());
            case ZEXT -> new Literal(a.asInt(), ((ExtInstr) instr).toType);
            case MOVE -> a;
            default -> throw new RuntimeException("Unknown opcode");
        };
    }
}
