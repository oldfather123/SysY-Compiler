package ir.value;

import ir.IrFactory;
import ir.instr.*;
import ir.type.ArrayType;
import ir.type.IrType;
import ir.type.PointerType;
import ir.type.Ty;
import utils.GlobalCounter;

public class Intermediate extends Value {
    public final String name;
    public final IrType type;
    public static IrFactory builder = null;

    public Intermediate(String name, IrType type) {
        super(name, type);
        this.name = name;
        this.type = type;
    }

    public Intermediate(IrType type) {
        this(GlobalCounter.gen("%").toString(), type);
    }

    public Intermediate(Value value) {
        this(value.getName(), value.getType());
    }

    private static Intermediate update(String name, IrType type) {
        return new Intermediate(name, type);
    }

    private Intermediate update(IrType type) {
        if (this instanceof Literal) {
            return Literal.doConvert((Literal) this, type);
        }
        String newName = GlobalCounter.gen("%").toString();
        addInstr(this.name, newName, this.type, type);
        return Intermediate.update(newName, type);
    }

    private void addInstr(String fromName, String toName, IrType fromType, IrType toType) {
        if (fromType.is("INT") && toType.is("INT")) {
            // int -> int, trunc or zext
            if (fromType.sizeof() < toType.sizeof()) {
                builder.insertInstr(new ExtInstr(
                    new Intermediate(toName, toType), fromType, new Intermediate(fromName, fromType), toType, false
                ));
            } else {
//                builder.insertInstr(new TruncInstr(
//                    new Intermediate(toName, toType), fromType, new Intermediate(fromName, fromType), toType
//                ));
                builder.insertInstr(new ICmpInstr(
                    new Intermediate(fromName, fromType), Literal.zero(fromType), ICmpInstr.Predicate.ne, new Intermediate(toName, toType)
                ));
            }
        } else if (fromType.is("INT") && toType.is("FLOAT")) {
            // int -> float, sitofp
            builder.insertInstr(new I2fInstr(new Intermediate(fromName, fromType), new Intermediate(toName, toType)));
        } else if (fromType.is("FLOAT") && toType.is("INT")) {
            // float -> int, fptosi
            if (toType.is("i1")) {
                builder.insertInstr(new FCmpInstr(new Intermediate(fromName, fromType), Literal.zero(fromType), FCmpInstr.Predicate.one, new Intermediate(toName, toType)));
            } else {
                builder.insertInstr(new F2iInstr(new Intermediate(fromName, fromType), new Intermediate(toName, toType)));
            }
        } else if (fromType.is("ARRAY") && toType.is("POINTER")) {
            IrType var1Base = ((ArrayType) fromType).getFinalBase();
            IrType var2Base = ((PointerType) toType).getBaseType();
            if (var2Base.is("ARRAY")) {
                var2Base = ((ArrayType) var2Base).getFinalBase();
            }
            if (!var1Base.equals(var2Base)) {
                throw new RuntimeException("Invalid Type Convert: " + var1Base + " -> " + var2Base);
            }
        } else  {
            throw new RuntimeException("Invalid Type Convert: " + fromType + " -> " + toType);
        }
    }

    public static Intermediate doConvertRevCond(Intermediate cond) {
        if (cond.type.is("i1")) {
            // Just reverse the condition
            Intermediate result = new Intermediate(Ty.I1);
            builder.insertInstr(new NotInstr(cond, result));
            return result;
        } else if (cond.type.is("INT")) {
            Intermediate result = new Intermediate(Ty.I1);
            builder.insertInstr(new ICmpInstr(cond, Literal.zero(cond.type), ICmpInstr.Predicate.ne, result));
            return result;
        } else if (cond.type.is("FLOAT")) {
            Intermediate result = new Intermediate(Ty.I1);
            builder.insertInstr(new FCmpInstr(cond, Literal.zero(cond.type), FCmpInstr.Predicate.one, result));
            return result;
        } else {
            throw new RuntimeException("Invalid Type: " + cond.type);
        }
    }

    private static void checkCalculable(Intermediate var1) {
        if (!var1.type.is("INT") && !var1.type.is("FLOAT") && !var1.type.is("POINTER")) {
            throw new RuntimeException("Invalid Type: " + var1.type);
        }
    }

    private static Intermediate[] uniteType(Intermediate var1, Intermediate var2) {
        if (var1.type.equals(var2.type)) {
            return new Intermediate[] { var1, var2 };
        }
        if (var1.type.is("i1") || var2.type.is("i1")) {
            if (var1.type.is("i32")) {
                return new Intermediate[] { var1.update(Ty.I1), var2 };
            }
            if (var1.type.is("f32")) {
                return new Intermediate[] { var1.update(Ty.I1), var2 };
            }
            if (var2.type.is("i32")) {
                return new Intermediate[] { var1, var2.update(Ty.I1) };
            }
            if (var2.type.is("f32")) {
                return new Intermediate[] { var1, var2.update(Ty.I1) };
            }
        }
        if (var1.type.is("INT") && var2.type.is("FLOAT")) {
            return new Intermediate[] { var1.update(Ty.F32), var2 };
        }
        if (var1.type.is("FLOAT") && var2.type.is("INT")) {
            return new Intermediate[] { var1, var2.update(Ty.F32) };
        }
        throw new RuntimeException("Invalid Type: " + var1.type + " " + var2.type);
    }

    public static Intermediate[] doArithmetic(Intermediate var1, Intermediate var2) {
        Intermediate[] res = new Intermediate[3];
        checkCalculable(var1);
        checkCalculable(var2);
        Intermediate[] vars = uniteType(var1, var2);
        res[1] = vars[0];
        res[2] = vars[1];
        res[0] = new Intermediate(vars[0].type);
        return res;
    }

    public static Intermediate[] doComparison(Intermediate var1, Intermediate var2) {
        Intermediate[] res = new Intermediate[3];
        checkCalculable(var1);
        checkCalculable(var2);
        Intermediate[] vars = uniteType(var1, var2);
        res[1] = vars[0];
        res[2] = vars[1];
        res[0] = new Intermediate(Ty.I1);
        return res;
    }

    public static Intermediate doConvert(Intermediate var, IrType type) {
        if (var.type.equals(type) || (var.type.is("POINTER") && type.is("POINTER"))) {
            return var;
        }
        if (var instanceof Literal) {
            return Literal.doConvert((Literal) var, type);
        } else {
            return var.update(type);
        }
    }

    @Override
    public String toString() {
        return name + ": " + type;
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public IrType getType() {
        return type;
    }
}
