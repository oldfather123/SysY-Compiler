package Utils;

import midend.Function;
import midend.LLVMType.*;
import midend.Param;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class LibFunction {
    public static HashSet<Function> libFunc = new HashSet<>();
    private static Function GETINT = new Function(FunctionType.functionType, "@getint", IntegerType.i32);
    private static Function GETCH = new Function(FunctionType.functionType, "@getch", IntegerType.i32);
    private static Function GETFLOAT = new Function(FunctionType.functionType, "@getfloat", FloatType.f32);
    private static Function GETARRAY = new Function(FunctionType.functionType, "@getarray", IntegerType.i32);
    private static Function GETFARRAY = new Function(FunctionType.functionType, "@getfarray", IntegerType.i32);
    private static Function PUTINT = new Function(FunctionType.functionType, "@putint", VoidType.voidType);
    private static Function PUTCH = new Function(FunctionType.functionType, "@putch", VoidType.voidType);
    private static Function PUTFLOAT = new Function(FunctionType.functionType, "@putfloat", VoidType.voidType);
    private static Function PUTARRAY = new Function(FunctionType.functionType, "@putarray", VoidType.voidType);
    private static Function PUTFARRAY = new Function(FunctionType.functionType, "@putfarray", VoidType.voidType);
    private static Function PUTF = new Function(FunctionType.functionType, "@putf", VoidType.voidType);
    private static Function STARTTIME = new Function(FunctionType.functionType, "@starttime", VoidType.voidType);
    private static Function STOPTIME = new Function(FunctionType.functionType, "@stoptime", VoidType.voidType);
    private static Function MEMSET = new Function(FunctionType.functionType, "@memset", VoidType.voidType);
    private static Function PARALLEL_START = new Function(FunctionType.functionType, "@parallel_start", IntegerType.i32);
    private static Function PARALLEL_END = new Function(FunctionType.functionType, "@parallel_end", IntegerType.i32);

    public static Function findFunc(String ident) {
        return switch (ident) {
            case "getint" -> GETINT;
            case "getch" -> GETCH;
            case "getfloat" -> GETFLOAT;
            case "getarray" -> GETARRAY;
            case "getfarray" -> GETFARRAY;
            case "putint" -> PUTINT;
            case "putch" -> PUTCH;
            case "putfloat" -> PUTFLOAT;
            case "putarray" -> PUTARRAY;
            case "putfarray" -> PUTFARRAY;
            case "putf" -> PUTF;
            case "starttime" -> STARTTIME;
            case "stoptime" -> STOPTIME;
            case "memset" -> MEMSET;
            case "parallel_start" -> PARALLEL_START;
            case "parallel_end" -> PARALLEL_END;
            default -> null;
        };
    }

    public static void initLib() {
        libFunc.add(GETINT);
        GETINT.setSideEffect();
        libFunc.add(GETCH);
        GETCH.setSideEffect();
        libFunc.add(GETFLOAT);
        GETFLOAT.setSideEffect();
        libFunc.add(PUTF);
        PUTF.setSideEffect();

        ArrayList<Param> params = new ArrayList<>();
        params.add(new Param(new PointerType((IntegerType.i32))));
        GETARRAY.setParams(params);
        libFunc.add(GETARRAY);
        GETARRAY.setSideEffect();

        params = new ArrayList<>();
        params.add(new Param(new PointerType(FloatType.f32)));
        GETFARRAY.setParams(params);
        libFunc.add(GETFARRAY);
        GETFARRAY.setSideEffect();

        params = new ArrayList<>();
        params.add(new Param(IntegerType.i32));
        PUTINT.setParams(params);
        libFunc.add(PUTINT);
        PUTINT.setSideEffect();

        params = new ArrayList<>();
        params.add(new Param(IntegerType.i32));
        PUTCH.setParams(params);
        libFunc.add(PUTCH);
        PUTCH.setSideEffect();

        params = new ArrayList<>();
        params.add(new Param(FloatType.f32));
        PUTFLOAT.setParams(params);
        libFunc.add(PUTFLOAT);
        PUTFLOAT.setSideEffect();

        params = new ArrayList<>();
        params.add(new Param(IntegerType.i32));
        params.add(new Param(new PointerType(IntegerType.i32)));
        PUTARRAY.setParams(params);
        libFunc.add(PUTARRAY);
        PUTARRAY.setSideEffect();

        params = new ArrayList<>();
        params.add(new Param(IntegerType.i32));
        params.add(new Param(new PointerType(FloatType.f32)));
        PUTFARRAY.setParams(params);
        libFunc.add(PUTFARRAY);
        PUTFARRAY.setSideEffect();

        params = new ArrayList<>();
        params.add(new Param(new PointerType(IntegerType.i32)));
        params.add(new Param(IntegerType.i32));
        params.add(new Param(IntegerType.i32));
        MEMSET.setParams(params);
        libFunc.add(MEMSET);
        MEMSET.setSideEffect();

        params = new ArrayList<>();
        params.add(new Param(IntegerType.i32));
        STARTTIME.setParams(params);
        libFunc.add(STARTTIME);
        STARTTIME.setSideEffect();

        params = new ArrayList<>();
        params.add(new Param(IntegerType.i32));
        STOPTIME.setParams(params);
        libFunc.add(STOPTIME);
        STOPTIME.setSideEffect();

        libFunc.add(PARALLEL_START);
        PARALLEL_START.setSideEffect();

        params = new ArrayList<>();
        params.add(new Param(IntegerType.i32));
        PARALLEL_END.setParams(params);
        libFunc.add(PARALLEL_END);
        PARALLEL_END.setSideEffect();
    }
}
