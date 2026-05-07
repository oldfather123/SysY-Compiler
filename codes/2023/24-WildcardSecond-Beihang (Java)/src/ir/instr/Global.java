package ir.instr;

import ir.Value;
import ir.type.ArrayType;
import ir.type.FloatType;
import ir.type.IntType;
import ir.type.Pointer;
import ir.type.Type;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.Variable;
import tools.ErrOutput;

import java.util.ArrayList;
import java.util.LinkedList;

public class Global extends Instr {
    Object initval;
    boolean isConst;
    public boolean isInit;

    public Global(Type type, String name, Boolean inner, BasicBlock parent) {
        super(new LinkedList<>(), type, name, parent);
        isConst = false;
    }

    public Global(Type type, String name, BasicBlock parent) {
        this(new Pointer(type), name, false, parent);
    }

    public Global(Variable v, BasicBlock parent, boolean isConst) {
        this(v.type, v.name, parent);
        v.allocInst = this;
        this.initval = v.value;
        this.isConst = isConst;
        this.isInit = v.isInit;
    }

    public Value getValue(ArrayList<Integer> pos){
        Object tmp = initval;
        Type innertype = ((Pointer)type).pointTo.innerType();
        for(int i = 0;i<pos.size();i++){
            int post = pos.get(i);
            ArrayList<Object> tmpList = (ArrayList<Object>) tmp;
            if(tmpList.size()<=post){
                if(innertype instanceof IntType){
                    return new ConstNumber(0);
                }else{
                    return new ConstNumber((double)0.0);
                }
            }
            tmp = tmpList.get(post);
        }
        if(tmp instanceof Integer){
            return new ConstNumber((Integer)tmp);
        }else{
            return new ConstNumber((Double) tmp);
        }
    }

    public String initvalToString(Object obj, Type t) {
        if(!isInit){
            return t.toString()+" zeroinitializer";
        }
        if (t instanceof IntType) {
            if(obj == null){
                return "i32 0";
            }
            return "i32 " + Integer.toString((Integer) obj);
        } else if (t instanceof FloatType) {
            if(obj == null){
                return (new ConstNumber(0.0).getNameWithType());
            }
            return new ConstNumber((double)((Double)obj).floatValue()).getNameWithType();
            //return "float " + "0x" + Integer.toUnsignedString(Float.floatToIntBits(((Double)obj).floatValue()), 16);
        } else {
            if (!(t instanceof ArrayType) || (obj != null && !(obj instanceof ArrayList<?>))) {
                ErrOutput.outputErr("error: cat initvaltostring");
                return "";
            }
            int num = ((ArrayType) t).size;
            ArrayList<Object> list = (ArrayList<Object>) obj;
            StringBuilder sb = new StringBuilder();
            sb.append(t);
            sb.append(" [");
            for(int i = 0;i<num;i++){
                if(obj == null || i >= ((ArrayList<?>) obj).size()){
                    sb.append(initvalToString(null, ((ArrayType) t).t));
                }else{
                    sb.append(initvalToString(list.get(i), ((ArrayType) t).t));
                }
                if(i != num-1){
                    sb.append(", ");
                }
            }
            sb.append("]");
            return sb.toString();
        }
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(String.format("%s = dso_local global ",
            getFullName()));
        assert type instanceof Pointer;
        sb.append(initvalToString(initval, ((Pointer) type).pointTo));
        return sb.toString();
    }

    @Override
    public String getFullName() {
        return "@" + name;
    }

    public Object getInitval() {
        return initval;
    }
}
