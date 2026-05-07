package IR.Value.Instructions;

import IR.Type.PointerType;
import IR.Type.Type;
import IR.Value.BasicBlock;
import IR.Value.ConstFloat;
import IR.Value.ConstInteger;
import IR.Value.Value;

import java.util.ArrayList;

//  局部数组都是allocInst，为此添加了一些属性
public class AllocInst extends Instruction{
    //  用于记录局部数组初始值的values
    ArrayList<Value> initValues;

    public AllocInst(Type type) {
        super("%" + (++Value.valNumber), type, OP.Alloca);
    }

    public Type getAllocType(){
        return ((PointerType) getType()).getEleType();
    }

    public boolean isArray(){
        return getAllocType().isArrayType();
    }

    public boolean isZeroInit(){
        if(initValues == null) return false;
        for(Value value : initValues){
            if(!((value instanceof ConstInteger constInt && constInt.getValue() == 0)
                    || (value instanceof ConstFloat constFloat && constFloat.getValue() == 0.0))){
                return false;
            }
        }
        return true;
    }

    public void setInitValues(ArrayList<Value> initValues){
        this.initValues = initValues;
    }

    public ArrayList<Value> getInitValues(){
        return this.initValues;
    }

    @Override
    public String getInstString(){
        Type EleType = ((PointerType) getType()).getEleType();
        return getName() + " = " + "alloca " + EleType.toString();
    }
}
