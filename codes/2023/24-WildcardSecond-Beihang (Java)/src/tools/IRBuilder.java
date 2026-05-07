package tools;

import ir.Value;
import ir.instr.*;
import ir.type.*;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.user.Function;

import java.util.LinkedList;

public class IRBuilder {
    //build allocaInst
    public static Alloca buildAlloca(String name, Type type, BasicBlock parent, IrRegDispatcher dispatcher) {
        if (type instanceof IntType || type instanceof ArrayType
                || type instanceof Pointer || type instanceof FloatType) {
            // 局部整数、局部和参数数组、参数中的指针
            Alloca alloca = new Alloca(type, name, parent, dispatcher);
            parent.addValue(alloca);
            return alloca;
        } else {
            // 可拓展类型
            throw new UnsupportedOperationException("Unsupported Local Var Type!");
        }
    }

    public static Alloca buildAlloca1(String name, Type type, BasicBlock parent, IrRegDispatcher dispatcher) {
        if (type instanceof IntType || type instanceof ArrayType
                || type instanceof Pointer || type instanceof FloatType) {
            // 局部整数、局部和参数数组、参数中的指针
            Alloca alloca = new Alloca(type, name, parent, dispatcher);
            return alloca;
        } else {
            // 可拓展类型
            throw new UnsupportedOperationException("Unsupported Local Var Type!");
        }
    }

    public static Store buildAssign(Value dest, Value val, BasicBlock parent, IrRegDispatcher dispatcher) {
        Type destType = dest.getType();
        if(!(destType instanceof Pointer)){
            ErrOutput.outputErr("erro:can't assign a number to non-pointer");
            return null;
        }
        if(((Pointer) destType).pointTo instanceof FloatType && val.getType() instanceof IntType){
            if(val instanceof ConstNumber){
                val = new ConstNumber(Double.valueOf((Integer) ((ConstNumber) val).value));
            }else{
                LinkedList<Value> subuses = new LinkedList<Value>();
                subuses.add(val);
                Instr instr = new Sitofp(subuses, dispatcher.allocId(), parent);
                parent.addValue(instr);
                val = instr;
            }
        }else if(((Pointer) destType).pointTo instanceof IntType && val.getType() instanceof FloatType){
            if(val instanceof ConstNumber){
                val = new ConstNumber((int)(double)((Double) ((ConstNumber) val).value));
            }else{
                LinkedList<Value> subuses = new LinkedList<Value>();
                subuses.add(val);
                Instr instr = new Fptosi(subuses, dispatcher.allocId(), parent);
                parent.addValue(instr);
                val = instr;
            }
        }
        Store ret = new Store(dest, val, parent);
        parent.addValue(ret);
        return ret;
    }

    public static Call buildCall(Function function, LinkedList<Value> uses,
                                 BasicBlock parent, IrRegDispatcher irRegDispatcher) {
        return new Call(function, uses, parent, irRegDispatcher);
    }

    public static Br buildBr(Value judge, Value trueBlock, Value falseBlock, BasicBlock parent) {
        LinkedList<Value> list = new LinkedList<>();
        if(judge != null) {
            list.add(judge);
        }
        list.add(trueBlock);
        if(falseBlock != null) {
            list.add(falseBlock);
        }
        return new Br(list, parent);
    }

    //每个函数应当从统一的位置返回，故不应在此处return，而是在每个函数visit的最后在exit块中统一return。
    //因此，此处改为store到ret并跳转到exit块。
    public static void buildRet(Value value, BasicBlock parent,Function function) {
        Value ret = function.returnValue;
        if(ret == null){
            parent.addValue(buildBr(null, function.getExitBlock(),null,parent));
        } else {
            buildAssign(ret, value, parent, null);
            parent.addValue(buildBr(null, function.getExitBlock(),null,parent));
        }
    }

    public static void buildMemset(Value target, ConstNumber setVal, Type setType,
                                   BasicBlock parent, Function function, IrRegDispatcher irRegDispatcher) {
        LinkedList<Value> uses = new LinkedList<>();
        uses.add(target);
        uses.add(setVal);
        uses.add(new ConstNumber(setType.getOffset()));
        parent.addValue(buildCall(function, uses, parent, irRegDispatcher));
    }

    public static Bitcast buildBitcast(Type castType, Value toCast,
                                       BasicBlock parent, IrRegDispatcher dispatcher) {
        return new Bitcast(castType, toCast, parent, dispatcher);
    }
    public static Zext buildZext(Value target, BasicBlock parent, IrRegDispatcher dispatcher){
        if(!(target.getType() instanceof IntType)){
            return null;
        }
        if(((IntType) target.getType()).getLen() == 32){
            return null;
        }
        LinkedList<Value> uses = new LinkedList<>();
        uses.add(target);
        return new Zext(uses, new IntType(), dispatcher.allocId(), parent);
    }
}
