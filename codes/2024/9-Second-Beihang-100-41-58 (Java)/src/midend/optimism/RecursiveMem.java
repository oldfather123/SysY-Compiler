package midend.optimism;

import frontend.AST.Func;
import midend.*;
import midend.LLVMType.*;
import midend.Module;
import midend.instr.*;

import java.util.ArrayList;
import java.util.Arrays;

public class RecursiveMem {
    private Module module;
    private int num = 0;
    private int FloatToIntScale = 1000;

    public RecursiveMem(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            if (!function.isRecursive()) {
                continue;
            }
            ArrayList<Function> callList = function.getCalledList();
            boolean only = true;
            for (Function function1 : callList) {
                if (!function1.equals(function)) {
                    only = false;
                }
            }
            if (!only) {
                continue;
            }
            if (function.getParams().size() > 4) {
                continue;
            }
            boolean array = false;
            for (Param param : function.getParams()) {
                if (param.getType().isPointer()) {
                    array = true;
                    break;
                }
            }
            if (array) {
                continue;
            }
            //仅调用自身
            if (check(function)) {
                recursiveMem(function);
            }

        }
    }

    public void recursiveMem(Function function) {
        //构造全局变量
        ArrayList<Value> values = new ArrayList<>();
        GlobalVar hashTable = new GlobalVar(new PointerType(new ArrayType(function.getRetType(), 10000)), "hashTable" + String.valueOf(num), values);
        ArrayList<Value> values1 = new ArrayList<>();
        GlobalVar visited = new GlobalVar(new PointerType(new ArrayType(IntegerType.i32, 10000)), "visited" + String.valueOf(num++), values1);
        module.addGlobalVar(hashTable);
        module.addGlobalVar(visited);
        BasicBlock hashBlock1 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock loadBlock = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock hashBlock2 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock storeBlock = new BasicBlock(UndefinedType.undefined, function);
        Param param1 = null, param2 = null, param3 = null, param4 = null;
        Value result = null;
        Value result2 = null;
        boolean isFloat = false;
        //hashBlock1
        if (function.getParams().size() == 1) {
            param1 = function.getParams().get(0);
            if (param1.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul1 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param1, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul1);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul1);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            result = hashBlock1.getInstrList().getLast();
        } else if (function.getParams().size() == 2) {
            param1 = function.getParams().get(0);
            param2 = function.getParams().get(1);
            if (param1.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul1 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param1, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul1);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul1);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }

            ALUInstr ashr = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 2)), InstrType.ASHR, hashBlock1);
            ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 6)), InstrType.SHL, hashBlock1);
            ALUInstr add1 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr, shl), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr);
            hashBlock1.addInstr(shl);
            hashBlock1.addInstr(add1);

            isFloat = false;
            if (param2.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul2 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param2, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul2);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul2);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add2 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add1), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add2);

            result = hashBlock1.getInstrList().getLast();
        } else if (function.getParams().size() == 3) {
            param1 = function.getParams().get(0);
            param2 = function.getParams().get(1);
            param3 = function.getParams().get(2);
            if (param1.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul1 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param1, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul1);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul1);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }

            ALUInstr ashr = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 2)), InstrType.ASHR, hashBlock1);
            ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 6)), InstrType.SHL, hashBlock1);
            ALUInstr add1 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr, shl), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr);
            hashBlock1.addInstr(shl);
            hashBlock1.addInstr(add1);

            isFloat = false;
            if (param2.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul2 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param2, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul2);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul2);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add2 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add1), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add2);

            ALUInstr ashr1 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 2)), InstrType.ASHR, hashBlock1);
            ALUInstr shl1 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 6)), InstrType.SHL, hashBlock1);
            ALUInstr add3 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr1, shl1), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr1);
            hashBlock1.addInstr(shl1);
            hashBlock1.addInstr(add3);

            isFloat = false;
            if (param3.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul3 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param3, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul3);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul3);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add4 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add3), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add4);

            result = hashBlock1.getInstrList().getLast();
        } else if (function.getParams().size() == 4) {
            param1 = function.getParams().get(0);
            param2 = function.getParams().get(1);
            param3 = function.getParams().get(2);
            param4 = function.getParams().get(3);
            if (param1.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul1 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param1, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul1);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul1);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }

            ALUInstr ashr = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 2)), InstrType.ASHR, hashBlock1);
            ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 6)), InstrType.SHL, hashBlock1);
            ALUInstr add1 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr, shl), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr);
            hashBlock1.addInstr(shl);
            hashBlock1.addInstr(add1);

            isFloat = false;
            if (param2.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul2 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param2, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul2);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul2);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add2 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add1), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add2);

            ALUInstr ashr1 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 2)), InstrType.ASHR, hashBlock1);
            ALUInstr shl1 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 6)), InstrType.SHL, hashBlock1);
            ALUInstr add3 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr1, shl1), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr1);
            hashBlock1.addInstr(shl1);
            hashBlock1.addInstr(add3);

            isFloat = false;
            if (param3.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul3 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param3, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul3);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul3);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add4 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add3), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add4);

            ALUInstr ashr2 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 2)), InstrType.ASHR, hashBlock1);
            ALUInstr shl2 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 6)), InstrType.SHL, hashBlock1);
            ALUInstr add5 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr2, shl2), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr2);
            hashBlock1.addInstr(shl2);
            hashBlock1.addInstr(add5);

            isFloat = false;
            if (param4.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul4 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param4, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul4);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul4);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add6 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add5), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add4);

            result = hashBlock1.getInstrList().getLast();
        }

        //hash2
        FloatToIntScale = 10000;
        if (function.getParams().size() == 1) {
            param1 = function.getParams().get(0);
            if (param1.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul1 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param1, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul1);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul1);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            result2 = hashBlock1.getInstrList().getLast();
        } else if (function.getParams().size() == 2) {
            param1 = function.getParams().get(0);
            param2 = function.getParams().get(1);
            if (param1.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul1 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param1, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul1);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul1);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }

            ALUInstr ashr = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 3)), InstrType.ASHR, hashBlock1);
            ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 7)), InstrType.SHL, hashBlock1);
            ALUInstr add1 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr, shl), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr);
            hashBlock1.addInstr(shl);
            hashBlock1.addInstr(add1);

            isFloat = false;
            if (param2.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul2 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param2, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul2);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul2);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add2 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add1), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add2);

            result2 = hashBlock1.getInstrList().getLast();
        } else if (function.getParams().size() == 3) {
            param1 = function.getParams().get(0);
            param2 = function.getParams().get(1);
            param3 = function.getParams().get(2);
            if (param1.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul1 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param1, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul1);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul1);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }

            ALUInstr ashr = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 3)), InstrType.ASHR, hashBlock1);
            ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 7)), InstrType.SHL, hashBlock1);
            ALUInstr add1 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr, shl), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr);
            hashBlock1.addInstr(shl);
            hashBlock1.addInstr(add1);

            isFloat = false;
            if (param2.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul2 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param2, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul2);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul2);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add2 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add1), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add2);

            ALUInstr ashr1 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 3)), InstrType.ASHR, hashBlock1);
            ALUInstr shl1 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 7)), InstrType.SHL, hashBlock1);
            ALUInstr add3 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr1, shl1), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr1);
            hashBlock1.addInstr(shl1);
            hashBlock1.addInstr(add3);

            isFloat = false;
            if (param3.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul3 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param3, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul3);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul3);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add4 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add3), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add4);

            result2 = hashBlock1.getInstrList().getLast();
        } else if (function.getParams().size() == 4) {
            param1 = function.getParams().get(0);
            param2 = function.getParams().get(1);
            param3 = function.getParams().get(2);
            param4 = function.getParams().get(3);
            if (param1.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul1 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param1, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul1);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul1);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }

            ALUInstr ashr = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 3)), InstrType.ASHR, hashBlock1);
            ALUInstr shl = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 7)), InstrType.SHL, hashBlock1);
            ALUInstr add1 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr, shl), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr);
            hashBlock1.addInstr(shl);
            hashBlock1.addInstr(add1);

            isFloat = false;
            if (param2.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul2 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param2, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul2);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul2);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add2 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add1), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add2);

            ALUInstr ashr1 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 3)), InstrType.ASHR, hashBlock1);
            ALUInstr shl1 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 7)), InstrType.SHL, hashBlock1);
            ALUInstr add3 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr1, shl1), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr1);
            hashBlock1.addInstr(shl1);
            hashBlock1.addInstr(add3);

            isFloat = false;
            if (param3.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul3 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param3, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul3);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul3);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add4 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add3), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add4);

            ALUInstr ashr2 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 3)), InstrType.ASHR, hashBlock1);
            ALUInstr shl2 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), new ConstInt(IntegerType.i32, 7)), InstrType.SHL, hashBlock1);
            ALUInstr add5 = new ALUInstr(IntegerType.i32, Arrays.asList(ashr2, shl2), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(ashr2);
            hashBlock1.addInstr(shl2);
            hashBlock1.addInstr(add5);

            isFloat = false;
            if (param4.getType().isFloat()) {
                isFloat = true;
            }
            ALUInstr mul4 = new ALUInstr(isFloat? FloatType.f32 : IntegerType.i32, Arrays.asList(param4, isFloat? new ConstFloat(FloatType.f32, FloatToIntScale) : new ConstInt(IntegerType.i32, FloatToIntScale)), isFloat? InstrType.FMUL : InstrType.MUL, hashBlock1);
            hashBlock1.addInstr(mul4);
            if (isFloat) {
                ConversionInstr conversionInstr = new ConversionInstr(IntegerType.i32, InstrType.FPTOSI, mul4);
                conversionInstr.setBasicBlock(hashBlock1);
                hashBlock1.addInstr(conversionInstr);
            }
            ALUInstr add6 = new ALUInstr(IntegerType.i32, Arrays.asList(hashBlock1.getInstrList().getLast(), add5), InstrType.ADD, hashBlock1);
            hashBlock1.addInstr(add6);

            result2 = hashBlock1.getInstrList().getLast();
        }

        /*
        if (hash < 0) {
            hash = hash * -1;
        }
         */
        CmpInstr slt_hash_1 = new CmpInstr("<", IntegerType.i1, Arrays.asList(result, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, hashBlock1);
        hashBlock1.addInstr(slt_hash_1);
        BasicBlock beforeLoadBlock1 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock beforeLoadBlock2 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr br_hash_1 = new BrInstr(slt_hash_1, Arrays.asList(beforeLoadBlock1, beforeLoadBlock2));
        br_hash_1.setBasicBlock(hashBlock1);
        hashBlock1.addInstr(br_hash_1);

        //beforeLoadBlock1
        ALUInstr sub_before1_1 = new ALUInstr(IntegerType.i32, Arrays.asList(new ConstInt(IntegerType.i32, 0), result), InstrType.SUB, beforeLoadBlock1);
        beforeLoadBlock1.addInstr(sub_before1_1);
        BrInstr brInstr = new BrInstr(beforeLoadBlock2);
        brInstr.setBasicBlock(beforeLoadBlock1);
        beforeLoadBlock1.addInstr(brInstr);

        //beforeLoadBlock2
        ArrayList<BasicBlock> basicBlocks = new ArrayList<>();
        basicBlocks.add(hashBlock1);
        basicBlocks.add(beforeLoadBlock1);
        PhiInstr phi_before2 = new PhiInstr(IntegerType.i32, basicBlocks);
        phi_before2.addOption(result, hashBlock1);
        phi_before2.addOption(sub_before1_1, beforeLoadBlock1);
        phi_before2.setBasicBlock(beforeLoadBlock2);
        beforeLoadBlock2.addInstr(phi_before2);
        BasicBlock beforeLoadBlock3 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr1 = new BrInstr(beforeLoadBlock3);
        brInstr1.setBasicBlock(beforeLoadBlock2);
        beforeLoadBlock2.addInstr(brInstr1);

        //beforeLoadBlock3
        BasicBlock beforeLoadBlock7 = new BasicBlock(UndefinedType.undefined, function);
        ArrayList<BasicBlock> basicBlocks1 = new ArrayList<>();
        basicBlocks1.add(beforeLoadBlock2);
        basicBlocks1.add(beforeLoadBlock7);
        PhiInstr phi_before3_1 = new PhiInstr(IntegerType.i32, basicBlocks1);
        phi_before3_1.addOption(new ConstInt(IntegerType.i32, 0), beforeLoadBlock2);
        ArrayList<BasicBlock> basicBlocks2 = new ArrayList<>();
        basicBlocks2.add(beforeLoadBlock2);
        basicBlocks2.add(beforeLoadBlock7);
        PhiInstr phi_before3_2 = new PhiInstr(IntegerType.i32, basicBlocks2);
        phi_before3_2.addOption(phi_before2, beforeLoadBlock2);
        phi_before3_1.setBasicBlock(beforeLoadBlock3);
        phi_before3_2.setBasicBlock(beforeLoadBlock3);
        beforeLoadBlock3.addInstr(phi_before3_1);
        beforeLoadBlock3.addInstr(phi_before3_2);
        CmpInstr slt_before3 = new CmpInstr("<", IntegerType.i1, Arrays.asList(phi_before3_1, new ConstInt(IntegerType.i32, 10000)), InstrType.ICMP, beforeLoadBlock3);
        beforeLoadBlock3.addInstr(slt_before3);
        BasicBlock beforeLoadBlock4 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock beforeLoadBlock5 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr2 = new BrInstr(slt_before3, Arrays.asList(beforeLoadBlock4, beforeLoadBlock5));
        brInstr2.setBasicBlock(beforeLoadBlock3);
        beforeLoadBlock3.addInstr(brInstr2);

        //beforeLoadBlock4
        ALUInstr add_before4_1 = new ALUInstr(IntegerType.i32, Arrays.asList(phi_before3_2, new ConstInt(IntegerType.i32, 1)), InstrType.ADD, beforeLoadBlock4);
        beforeLoadBlock4.addInstr(add_before4_1);
        ALUInstr sdiv_before4_1 = new ALUInstr(IntegerType.i32, Arrays.asList(add_before4_1, new ConstInt(IntegerType.i32, 10000)), InstrType.SDIV, beforeLoadBlock4);
        beforeLoadBlock4.addInstr(sdiv_before4_1);
        ALUInstr mul_before4_1 = new ALUInstr(IntegerType.i32, Arrays.asList(sdiv_before4_1, new ConstInt(IntegerType.i32, 10000)), InstrType.MUL, beforeLoadBlock4);
        beforeLoadBlock4.addInstr(mul_before4_1);
        ALUInstr sub_before4_1 = new ALUInstr(IntegerType.i32, Arrays.asList(add_before4_1, mul_before4_1), InstrType.SUB, beforeLoadBlock4);
        beforeLoadBlock4.addInstr(sub_before4_1);
        ArrayList<Value> values2 = new ArrayList<>();
        values2.add(new ConstInt(IntegerType.i32, 0));
        values2.add(sub_before4_1);
        GetElementPtrInstr gep_before4_1 = new GetElementPtrInstr(new PointerType(IntegerType.i32), visited, values2);
        gep_before4_1.setBasicBlock(beforeLoadBlock4);
        beforeLoadBlock4.addInstr(gep_before4_1);
        LoadInstr load_before4_1 = new LoadInstr(IntegerType.i32, gep_before4_1, beforeLoadBlock4);
        beforeLoadBlock4.addInstr(load_before4_1);
        CmpInstr ne_before4_1 = new CmpInstr("==", IntegerType.i1, Arrays.asList(load_before4_1, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, beforeLoadBlock4);
        beforeLoadBlock4.addInstr(ne_before4_1);
        BasicBlock beforeLoadBlock6 = new BasicBlock(UndefinedType.undefined, function);
        BasicBlock beforeLoadBlock8 = new BasicBlock(UndefinedType.undefined, function);
        BrInstr brInstr3 = new BrInstr(ne_before4_1, Arrays.asList(beforeLoadBlock6, beforeLoadBlock8));
        brInstr3.setBasicBlock(beforeLoadBlock4);
        beforeLoadBlock4.addInstr(brInstr3);

        //beforeLoadBlock5(exitBlock-loadBlock)
        ArrayList<BasicBlock> basicBlocks3 = new ArrayList<>();
        basicBlocks3.add(beforeLoadBlock3);
        basicBlocks3.add(beforeLoadBlock6);
        PhiInstr phi_before5_1 = new PhiInstr(IntegerType.i32, basicBlocks3);
        phi_before5_1.addOption(phi_before3_2, beforeLoadBlock3);
        phi_before5_1.addOption(sub_before4_1, beforeLoadBlock6);
        phi_before5_1.setBasicBlock(beforeLoadBlock5);
        beforeLoadBlock5.addInstr(phi_before5_1);
        ArrayList<Value> values3 = new ArrayList<>();
        values3.add(new ConstInt(IntegerType.i32, 0));
        values3.add(phi_before5_1);
        GetElementPtrInstr gep_before5_1 = new GetElementPtrInstr(new PointerType(IntegerType.i32), visited, values3);
        gep_before5_1.setBasicBlock(beforeLoadBlock5);
        beforeLoadBlock5.addInstr(gep_before5_1);
        LoadInstr load_before5_1 = new LoadInstr(IntegerType.i32, gep_before5_1, beforeLoadBlock5);
        beforeLoadBlock5.addInstr(load_before5_1);
        CmpInstr ne_before5_1 = new CmpInstr("!=", IntegerType.i1, Arrays.asList(load_before5_1, new ConstInt(IntegerType.i32, 0)), InstrType.ICMP, beforeLoadBlock5);
        beforeLoadBlock5.addInstr(ne_before5_1);
        BasicBlock afterLoadBlock = function.getBlockList().get(0);
        BrInstr brInstr7 = new BrInstr(ne_before5_1, Arrays.asList(loadBlock, afterLoadBlock));
        brInstr7.setBasicBlock(beforeLoadBlock5);
        beforeLoadBlock5.addInstr(brInstr7);
        //TODO

        //beforeLoadBlock6
        BrInstr brInstr4 = new BrInstr(beforeLoadBlock5);
        brInstr4.setBasicBlock(beforeLoadBlock6);
        beforeLoadBlock6.addInstr(brInstr4);

        //beforeLoadBlock7
        ALUInstr add_before7_1 = new ALUInstr(IntegerType.i32, Arrays.asList(phi_before3_1, new ConstInt(IntegerType.i32, 1)), InstrType.ADD, beforeLoadBlock7);
        beforeLoadBlock7.addInstr(add_before7_1);
        BrInstr brInstr5 = new BrInstr(beforeLoadBlock3);
        brInstr5.setBasicBlock(beforeLoadBlock7);
        beforeLoadBlock7.addInstr(brInstr5);

        //beforeLoadBlock8
        LoadInstr load_before8_1 = new LoadInstr(IntegerType.i32, gep_before4_1, beforeLoadBlock8);
        beforeLoadBlock8.addInstr(load_before8_1);
        CmpInstr eq_before8_1 = new CmpInstr("==", IntegerType.i1, Arrays.asList(load_before8_1, result2), InstrType.ICMP, beforeLoadBlock8);
        beforeLoadBlock8.addInstr(eq_before8_1);
        BrInstr brInstr6 = new BrInstr(eq_before8_1, Arrays.asList(beforeLoadBlock6, beforeLoadBlock7));
        brInstr6.setBasicBlock(beforeLoadBlock8);
        beforeLoadBlock8.addInstr(brInstr6);

        phi_before3_1.addOption(add_before7_1, beforeLoadBlock7);
        phi_before3_2.addOption(sub_before4_1, beforeLoadBlock7);

        //loadBlock
        ArrayList<Value> values4 = new ArrayList<>();
        values4.add(new ConstInt(IntegerType.i32, 0));
        values4.add(phi_before5_1);
        GetElementPtrInstr gep_loadBlock_1 = new GetElementPtrInstr(new PointerType(function.getRetType()), hashTable, values4);
        gep_loadBlock_1.setBasicBlock(loadBlock);
        loadBlock.addInstr(gep_loadBlock_1);
        LoadInstr load_loadBlock_1 = new LoadInstr(function.getRetType(), gep_loadBlock_1, loadBlock);
        loadBlock.addInstr(load_loadBlock_1);
        BrInstr brInstr8 = new BrInstr(function.getExitBlock());
        brInstr8.setBasicBlock(loadBlock);
        loadBlock.addInstr(brInstr8);

        //storeBlock
        Value retValue = function.getExitBlock().getInstrList().getFirst();
        if (retValue instanceof PhiInstr) {
            ((PhiInstr) retValue).addOp(load_loadBlock_1, loadBlock);
        } else {
            System.out.println("There sames something wrong\n");
        }

        RetInstr retInstr = (RetInstr) function.getExitBlock().getInstrList().getLast();
        BasicBlock newExitBlock = new BasicBlock(UndefinedType.undefined, function);
        retInstr.setBasicBlock(newExitBlock);
        newExitBlock.addInstr(retInstr);
        function.getExitBlock().getInstrList().removeLast();
        BrInstr brInstr10 = new BrInstr(storeBlock);
        brInstr10.setBasicBlock(function.getExitBlock());
        function.getExitBlock().addInstr(brInstr10);
        function.setExitBlock(newExitBlock);

        ArrayList<Value> values5 = new ArrayList<>();
        values5.add(new ConstInt(IntegerType.i32, 0));
        values5.add(phi_before5_1);
        GetElementPtrInstr gep_store_1 = new GetElementPtrInstr(new PointerType(function.getRetType()), hashTable, values5);
        gep_store_1.setBasicBlock(storeBlock);
        storeBlock.addInstr(gep_store_1);
        StoreInstr store_storeBlock_1 = new StoreInstr(IntegerType.i32, retValue, gep_store_1, storeBlock);
        storeBlock.addInstr(store_storeBlock_1);
        StoreInstr store_storeBlock_2 = new StoreInstr(IntegerType.i32, result2, gep_before5_1, storeBlock);
        storeBlock.addInstr(store_storeBlock_2);
        BrInstr brInstr9 = new BrInstr(newExitBlock);
        brInstr9.setBasicBlock(storeBlock);
        storeBlock.addInstr(brInstr9);

        function.getBlockList().addFirst(loadBlock);
        function.getBlockList().addFirst(beforeLoadBlock8);
        function.getBlockList().addFirst(beforeLoadBlock7);
        function.getBlockList().addFirst(beforeLoadBlock6);
        function.getBlockList().addFirst(beforeLoadBlock5);
        function.getBlockList().addFirst(beforeLoadBlock4);
        function.getBlockList().addFirst(beforeLoadBlock3);
        function.getBlockList().addFirst(beforeLoadBlock2);
        function.getBlockList().addFirst(beforeLoadBlock1);
        function.getBlockList().addFirst(hashBlock1);

        function.getBlockList().addLast(storeBlock);
        function.getBlockList().addLast(newExitBlock);
    }


    public boolean check(Function function) {
        if (function.isMem()) {
            return false;
        }
        if (function.getRetType().isVoid()) {
            return false;
        }
        for (BasicBlock block : function.getBlockList()) {
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof LoadInstr) {
                    if (((LoadInstr) instruction).getValue() instanceof GlobalVar) {
                        return false;
                    }
                    if (((LoadInstr) instruction).getValue() instanceof Param) {
                        return false;
                    }
                    if (((LoadInstr) instruction).getValue() instanceof GetElementPtrInstr) {
                        Value value = ((LoadInstr) instruction).getValue();
                        while (value instanceof GetElementPtrInstr) {
                            value = ((GetElementPtrInstr) value).getTarget();
                        }
                        return !(value instanceof GlobalVar || value instanceof Param);
                    }
                }
                if (instruction instanceof StoreInstr) {
                    if (((StoreInstr) instruction).getPointer() instanceof GlobalVar) {
                        return false;
                    }
                    if (((StoreInstr) instruction).getPointer() instanceof Param) {
                        return false;
                    }
                    if (((StoreInstr) instruction).getPointer() instanceof GetElementPtrInstr) {
                        Value value = ((StoreInstr) instruction).getPointer();
                        while (value instanceof GetElementPtrInstr) {
                            value = ((GetElementPtrInstr) value).getTarget();
                        }
                        return !(value instanceof Param || value instanceof GlobalVar);
                    }
                }
            }
        }
        if (function.getExitBlock().getInstrList().size() == 2) {
            return function.getExitBlock().getInstrList().get(0) instanceof PhiInstr;
        }
        return true;
    }
}
