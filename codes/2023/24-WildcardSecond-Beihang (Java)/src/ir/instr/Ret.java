package ir.instr;

import ir.Value;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.VoidValue;

import java.util.LinkedHashMap;
import java.util.LinkedList;

public class Ret extends Instr {
    public Ret(LinkedList< Value > uses, BasicBlock parent) {
        super(uses, null, "", parent); //没有左值 所以没有name
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("ret ");
        if(this.getOperands().size() == 0) {
            sb.append("void");
        } else {
            sb.append(getOperands().get(0).getNameWithType());
        }
        return sb.toString();
    }

    @Override
    public boolean hasName() {
        return false;
    }

    @Override
    public boolean isEnd() {
        return true;
    }

    @Override
    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap) {
        LinkedList<Value> uses = new LinkedList<>();
        for(int i = 0;i < this.getOperands().size();i++) {
            Value temp = this.getOperands().get(i);
            if(temp instanceof ConstNumber) {
                uses.add(temp);
            } else if(temp instanceof VoidValue) {
                uses.add(new VoidValue());
            } else {
                assert valueHashMap.containsKey(temp);
                uses.add(valueHashMap.get(temp));
            }
        }
        Ret ret = new Ret(uses, basicBlock);
        valueHashMap.put(this, ret);
        return ret;
    }
}
