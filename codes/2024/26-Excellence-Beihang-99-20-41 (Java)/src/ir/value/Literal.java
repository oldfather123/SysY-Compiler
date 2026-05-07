package ir.value;

import ir.type.IrType;
import ir.type.Ty;

public class Literal extends Intermediate {
    private final Object literal;

    public Literal(int literal, IrType type) {
        super(String.valueOf(literal), type);
        this.literal = literal;
    }

    public Literal(int literal) {
        super(String.valueOf(literal), Ty.I32);
        this.literal = literal;
    }

    public Literal(float literal) {
        super("0x" + Long.toHexString(Double.doubleToRawLongBits(literal)), Ty.F32);
        this.literal = literal;
    }

    public static Literal zero(IrType type) {
        if (type.is("INT")) {
            return new Literal(0, type);
        } else if (type.is("FLOAT")) {
            return new Literal(0f);
        } else {
            throw new RuntimeException("Unsupported type");
        }
    }

    public static Literal doConvert(Literal literal, IrType type) {
        if (type.is("i1")) {
            if (literal.getType().is("INT")) {
                return new Literal(literal.asInt() != 0 ? 1 : 0, type);
            } else if (literal.getType().is("FLOAT")) {
                return new Literal(literal.asFloat() != 0 ? 1 : 0);
            } else {
                throw new RuntimeException("Unsupported type");
            }
        }
        if (literal.getType().is("INT") && type.is("FLOAT")) {
            return new Literal((float) literal.asInt());
        } else if (literal.getType().is("FLOAT") && type.is("INT")) {
            return new Literal((int) literal.asFloat(), type);
        } else {
            throw new RuntimeException("Unsupported type");
        }
    }

    public int asInt() {
        return (int) literal;
    }

    public float asFloat() {
        return (float) literal;
    }
}
