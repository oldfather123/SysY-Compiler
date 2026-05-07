package IR.IRValueRef;

import IR.IRConst;
import IR.IRType.IRType;

import java.util.ArrayList;
import java.util.List;

public class IRArrayRef implements IRValueRef{
    String name;
    IRType type;
    List<Float> initFloatValue; // 初始化float数组
    List<Integer> initIntValue; // 初始化int数组
    boolean isAllZero = false;
    public IRArrayRef(String name, IRType type){
        this.name = name;
        this.type = type;
        this.initFloatValue = new ArrayList<>();
        this.initIntValue = new ArrayList<>();
    }
    public void setInitValue(float value){
        initFloatValue.add(value);
    }
    public void setZero(boolean zero) {
        isAllZero = zero;
    }
    public boolean isAllZero() {
        return isAllZero;
    }

    public void setInitIntValue(List<Integer> initIntValue) {
        this.initIntValue = initIntValue;
    }

    public void setInitValue(int value){
        initIntValue.add(value);
    }
    public void setInitFloatValue(List<Float> initFloatValue) {
        this.initFloatValue = initFloatValue;
    }
    public List<Float> getInitFloatValue() {
        return initFloatValue;
    }
    public List<Integer> getInitIntValue() {
        return initIntValue;
    }
    public String getText() {
        return name;
    }
    public IRType getType() {
        return type;
    }/*返回该值的类型*/
    public int getTypeKind(){
        /*返回该值的类型*/
        return IRConst.IRConstantArrayValueKind;
    }
}
