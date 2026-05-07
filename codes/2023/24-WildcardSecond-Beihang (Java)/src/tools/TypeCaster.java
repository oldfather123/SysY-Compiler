package tools;

import ir.Value;
import ir.instr.Bitcast;
import ir.instr.Fptosi;
import ir.instr.InstType;
import ir.instr.Instr;
import ir.instr.Sitofp;
import ir.type.ArrayType;
import ir.type.FloatType;
import ir.type.IntType;
import ir.type.Pointer;
import ir.type.Type;
import ir.type.VoidType;
import ir.value.BasicBlock;
import ir.value.ConstNumber;

import java.util.LinkedList;

public class TypeCaster {

    //处理二元计算指令中的类型隐式转换
    //存在指针、arr或者void说明类型错误，直接报错
    //否则返回正确的目标类型对象
    public static Type castTypeFrom(Type t1, Type t2) {
        if (t1 instanceof ArrayType || t2 instanceof ArrayType) {
            ErrOutput.outputErr("error: casting type from array && array");
        }
        if (t1 instanceof VoidType || t2 instanceof VoidType) {
            ErrOutput.outputErr("error: casting type from void && *");
        }
        if ((t1 instanceof Pointer) || (t2 instanceof Pointer)) {
            ErrOutput.outputErr("error: casting type from pointer, we don't support pointer ope");
        }
        boolean haveFloat = false;
        if ((t1 instanceof FloatType) || (t2 instanceof FloatType)) {
            haveFloat = true;
        }
        if (haveFloat) {
            return new FloatType();
        } else {
            return new IntType();
        }
    }

    public static Value generateCastNumTypeInstr(Value v0, Type t0, Type t1,
                                                 IrRegDispatcher dispatcher,
                                                 BasicBlock basicBlock) {
        if (t0 instanceof IntType) {
            if (t1 instanceof IntType) {
                return null;
            } else if (t1 instanceof FloatType) {
                LinkedList<Value> uses = new LinkedList<>();
                uses.add(v0);
                return new Sitofp(uses, dispatcher.allocId(), basicBlock);
            } else {
                ErrOutput.outputErr("error: cast intType to un-Num");
            }
        } else if (t0 instanceof FloatType) {
            if (t1 instanceof FloatType) {
                return null;
            } else if (t1 instanceof IntType) {
                LinkedList<Value> uses = new LinkedList<>();
                uses.add(v0);
                return new Fptosi(uses, dispatcher.allocId(), basicBlock);
            } else {
                ErrOutput.outputErr("error: cast floatType to un-Num");
            }
        } else {
            ErrOutput.outputErr("error: cast un-Num type with generateCastNumTypeInstr");
        }
        return null;
    }

    public static Value generateCastNumTypeInstrTry(Value v0, Type t0, Type t1,
                                                 IrRegDispatcher dispatcher,
                                                 BasicBlock basicBlock) {
        if (t0 instanceof IntType) {
            if (t1 instanceof IntType) {
                return null;
            } else if (t1 instanceof FloatType) {
                LinkedList<Value> uses = new LinkedList<>();
                uses.add(v0);
                return new Sitofp(uses, dispatcher.allocId(), basicBlock);
            }
        } else if (t0 instanceof FloatType) {
            if (t1 instanceof FloatType) {
                return null;
            } else if (t1 instanceof IntType) {
                LinkedList<Value> uses = new LinkedList<>();
                uses.add(v0);
                return new Fptosi(uses, dispatcher.allocId(), basicBlock);
            }
        }else if(t0 instanceof Pointer){
            if(t1 instanceof Pointer && !t0.equals(t1)){
                var bitcast = new Bitcast(t1, v0,basicBlock,dispatcher.allocId());
                bitcast.crossCast = true;
                Debugger.noCrossArray = false;
                return bitcast;
            }
        }
        return null;
    }

    public static Value generateCastIntTo32Len(Value v0, IrRegDispatcher dispatcher,
                                               BasicBlock basicBlock) {
        Type t = v0.getType();
        if (!(t instanceof IntType)) {
            return null;
        }
        if (((IntType) t).getLen() < 32) {
            return IRBuilder.buildZext(v0, basicBlock, dispatcher);
        } else {
            return null;
        }
    }

    public static boolean definitely32Len(Value v) {
        Type t = v.getType();
        if (t instanceof IntType && v instanceof ConstNumber) {
            return (Integer) ((ConstNumber) v).value != 1 && (Integer) ((ConstNumber) v).value != 0;
        }
        if (t instanceof IntType) {
            return ((IntType) t).getLen() != 1;
        }
        return true;
    }


    public static Type castTypeAndSet(LinkedList<Value> uses, IrRegDispatcher dispatcher,
                                      BasicBlock basicBlock, boolean need32) {
        Type typea = uses.get(0).type;
        Type typeb = uses.get(1).type;
        Type targetType = castTypeFrom(typea, typeb);
        Value tmpinstr;
        if (definitely32Len(uses.get(0)) || definitely32Len(uses.get(1)) || need32) {
            tmpinstr = generateCastIntTo32Len(uses.get(0), dispatcher, basicBlock);
            if (tmpinstr != null) {
                basicBlock.addValue(tmpinstr);
                uses.set(0, tmpinstr);
            }
            tmpinstr = generateCastIntTo32Len(uses.get(1), dispatcher, basicBlock);
            if (tmpinstr != null) {
                basicBlock.addValue(tmpinstr);
                uses.set(1, tmpinstr);
            }
        }

        if (!typea.equals(targetType)) {
            if (uses.get(0) instanceof ConstNumber) {
                uses.set(0,
                    new ConstNumber(Double.valueOf((Integer) ((ConstNumber) uses.get(0)).value))
                );
            } else {
                LinkedList<Value> subuses = new LinkedList<Value>();
                subuses.add(uses.get(0));
                Instr instr = new Sitofp(subuses, dispatcher.allocId(), basicBlock);
                basicBlock.addValue(instr);
                uses.set(0, instr);
            }

        }
        if (!typeb.equals(targetType)) {
            if (uses.get(1) instanceof ConstNumber) {
                uses.set(1,
                    new ConstNumber(Double.valueOf((Integer) ((ConstNumber) uses.get(1)).value))
                );
            } else {
                LinkedList<Value> subuses = new LinkedList<Value>();
                subuses.add(uses.get(1));
                Instr instr = new Sitofp(subuses, dispatcher.allocId(), basicBlock);
                basicBlock.addValue(instr);
                uses.set(1, instr);
            }
        }
        return targetType;
    }

    public static Type realType(Type t) {
        if (t instanceof Pointer) {
            return new ArrayType(-1, ((Pointer) t).pointTo);
        } else {
            return t;
        }
    }
}
