package cn.edu.bit.newnewcc.frontend;

import cn.edu.bit.newnewcc.ir.type.*;
import cn.edu.bit.newnewcc.ir.value.ExternalFunction;

import java.util.List;

public final class SysYExternalFunctions {
    private SysYExternalFunctions() {
    }

    public static final List<ExternalFunction> EXTERNAL_FUNCTIONS = List.of(
        new ExternalFunction(
            FunctionType.getInstance(IntegerType.getI32(), List.of()), "getint"),
        new ExternalFunction(
            FunctionType.getInstance(VoidType.getInstance(), List.of(IntegerType.getI32())), "putint"),
        new ExternalFunction(
            FunctionType.getInstance(FloatType.getFloat(), List.of()), "getfloat"),
        new ExternalFunction(
            FunctionType.getInstance(VoidType.getInstance(), List.of(FloatType.getFloat())), "putfloat"),
        new ExternalFunction(
            FunctionType.getInstance(IntegerType.getI32(), List.of()), "getch"),
        new ExternalFunction(
            FunctionType.getInstance(VoidType.getInstance(), List.of(IntegerType.getI32())), "putch"),
        new ExternalFunction(
                FunctionType.getInstance(IntegerType.getI32(), List.of(PointerType.getInstance(IntegerType.getI32()))), "getarray"),
            new ExternalFunction(
                    FunctionType.getInstance(VoidType.getInstance(), List.of(IntegerType.getI32(), PointerType.getInstance(IntegerType.getI32()))), "putarray"),
            new ExternalFunction(
                    FunctionType.getInstance(IntegerType.getI32(), List.of(PointerType.getInstance(FloatType.getFloat()))), "getfarray"),
            new ExternalFunction(
                    FunctionType.getInstance(VoidType.getInstance(), List.of(IntegerType.getI32(), PointerType.getInstance(FloatType.getFloat()))), "putfarray"),
            new ExternalFunction(
                    FunctionType.getInstance(VoidType.getInstance(), List.of(IntegerType.getI32())), "_sysy_starttime"),
            new ExternalFunction(
                    FunctionType.getInstance(VoidType.getInstance(), List.of(IntegerType.getI32())), "_sysy_stoptime"),
            new ExternalFunction(
                    FunctionType.getInstance(VoidType.getInstance(), List.of(PointerType.getInstance(IntegerType.getI32()), IntegerType.getI32(), IntegerType.getI32())), "memset"),
            new ExternalFunction(
                    FunctionType.getInstance(IntegerType.getI32(), List.of(IntegerType.getI32(), IntegerType.getI32())), "llvm.smax.i32"),
            new ExternalFunction(
                    FunctionType.getInstance(IntegerType.getI32(), List.of(IntegerType.getI32(), IntegerType.getI32())), "llvm.smin.i32"),
            new ExternalFunction(
                    FunctionType.getInstance(IntegerType.getI64(), List.of(IntegerType.getI64(), IntegerType.getI64())), "llvm.smax.i64"),
            new ExternalFunction(
                    FunctionType.getInstance(IntegerType.getI64(), List.of(IntegerType.getI64(), IntegerType.getI64())), "llvm.smin.i64")
    );
}
