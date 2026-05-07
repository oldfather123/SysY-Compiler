package ir.instr;

import ir.Value;
import ir.type.Type;
import ir.value.BasicBlock;
import ir.value.ConstNumber;

import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedList;

public class Zext extends Instr {

    public Zext(LinkedList<Value> uses, Type type, String name,
                BasicBlock parent) {
        super(uses, type, name, parent);
    }

    @Override
    public String toString() {
        return String.format("%s = zext %s to %s",
            getFullName(), getOP(0).getNameWithType(), getType().toString());
    }

    @Override
    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap) {
        LinkedList<Value> uses = new LinkedList<>();
        for(int i = 0;i < this.getOperands().size();i++) {
            Value temp = this.getOperands().get(i);
            if(temp instanceof ConstNumber) {
                uses.add(temp);
            } else {
                assert valueHashMap.containsKey(temp);
                uses.add(valueHashMap.get(temp));
            }
        }
        Zext zext = new Zext(uses, type, name, basicBlock);
        valueHashMap.put(this, zext);
        return zext;
    }
}
