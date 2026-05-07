package ir.instr;

import ir.Value;
import ir.type.Pointer;
import ir.type.Type;
import ir.value.Arg;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.User;

import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedList;

public class Store extends Instr {
    private int num = -1; //专门用于函数存储的 代表%i个参数
    public Value getDest() {
        return getOP(0);
    }

    public boolean checkGlobalStore() {
        if(getOP(0) instanceof Global) return true;
        if(getOP(0) instanceof GetElementPtrInst && ((User)getOP(0)).getOP(0) instanceof Global) {
            return true;
        }
        return false;
    }

    public boolean checkArgPointerStore(){
        if(getOP(1) instanceof Arg && getOP(1).getType() instanceof Pointer) return true;
        for(var op:getOperands()){
            if(op instanceof GetElementPtrInst && ((GetElementPtrInst)op).getOP(0) instanceof Arg) return true;
        }
        return false;
    }

    public Value getVal(){
        return getOP(1);
    }

    public Store(Value dest, Value val, BasicBlock parent) {
        super(new LinkedList<>(Arrays.asList(dest,val)), dest.type, dest.name, parent);
    }

    @Override
    public String toString() {
        return String.format("store %s, %s",
            getVal().getNameWithType(), getDest().getNameWithType()
        );
    }

    @Override
    public boolean hasName() {
        return false;
    }

    @Override
    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap) {
        Value dest = valueHashMap.get(getDest());
        Value val = null;
        if(getVal() instanceof Arg) {
            val = new Arg(getVal().getName(), getVal().getType());
        } else if(getVal() instanceof ConstNumber){
            assert ((ConstNumber) getVal()).value instanceof Double
                    || ((ConstNumber) getVal()).value instanceof Integer;
            if(((ConstNumber) getVal()).value instanceof Double) {
                val = new ConstNumber((double)(((ConstNumber) getVal()).value));
            } else if(((ConstNumber) getVal()).value instanceof Integer) {
                val = new ConstNumber((Integer) (((ConstNumber) getVal()).value));
            }
        } else {
            val = valueHashMap.get(getVal());
        }
        Store store = new Store(dest, val, basicBlock);
        valueHashMap.put(this, store);
        return store;
    }
}
