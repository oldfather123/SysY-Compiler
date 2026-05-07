package ir.type;

public class Ty {
    public static IrType I32 = new IntegerType(32);
    public static IrType I1 = new IntegerType(1);
    public static IrType F32 = new FloatType();
    public static IrType VOID = new VoidType();
}
