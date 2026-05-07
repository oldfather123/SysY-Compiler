package IR.Value;

import IR.Type.ArrayType;
import IR.Type.IntegerType;
import IR.Type.PointerType;
import IR.Type.Type;
import IR.Use;
import IR.Value.Instructions.LoadInst;

import java.awt.*;
import java.util.ArrayList;

public class GlobalVar extends Value{
    private Value value;
    //  代表全局数组的初始值
    private ArrayList<Value> values;
    //  nowVisit用于递归输出toString
    private int nowVisit;
    private final boolean isArray;
    private boolean _isConst;

    //  全局变量
    public GlobalVar(String name, Type type, Value value){
        super(name, type);
        //  这个Value是他的初始值
        this.value = value;
        this.isArray = false;
        this.nowVisit = 0;
    }

    public boolean isConst(){
        return _isConst;
    }

    public void setConst(boolean isConst){
        this._isConst = isConst;
    }

    //  全局数组
    public GlobalVar(String name, Type type, ArrayList<Value> values){
        super(name, type);
        this.values = values;
        this.isArray = true;
    }

    public boolean isArray(){
        return isArray;
    }

    public Value getValue() {
        return value;
    }

    public ArrayList<Value> getValues() {
        return values;
    }

    public void setInitValue(int idx, Value value){
        if(!isArray){
            return;
        }
        values.set(idx, value);
    }

    public boolean isAllZero(){
        if(!isArray){
            return false;
        }
        if(values.size() == 0){
            return true;
        }
        for(Value initValue : values){
            if(!(initValue instanceof ConstInteger constInteger
                    && constInteger.getValue() == 0)
                    && !(initValue instanceof ConstFloat constFloat
                    && constFloat.getValue() == 0.0)){
                return false;
            }
        }
        return true;
    }

    //  根据维度list将一维的values转换成String输出
    private String initValueString(ArrayType type){
        StringBuilder sb = new StringBuilder();
        sb.append(type);
        sb.append(" ");
        sb.append("[");
        int len = type.getNum();
        if(type.getEleType().isArrayType()){
            for(int i = 0; i < len; i++){
                sb.append(initValueString((ArrayType) type.getEleType()));
                if(i != len - 1) sb.append(", ");
            }
        }
        else{
            for(int i = 0; i < len; i++){
                sb.append(values.get(nowVisit++));
                if(i != len - 1) sb.append(", ");
            }
        }
        sb.append("]");
        return sb.toString();
    }

    public String toInstString(){
        String prefix = getName() + " = global ";
        PointerType type = (PointerType) getType();
        Type eleType = type.getEleType();
        if(eleType.isArrayType()){
            if(isAllZero()){
                return prefix + eleType + "zeroinitializer";
            }
            else {
                nowVisit = 0;
                return prefix + initValueString((ArrayType) eleType);
            }
        }
        else {
            return prefix + getValue();
        }
    }

}
