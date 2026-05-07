package ir.instr;

import ir.Value;
import ir.type.IntType;
import ir.type.Type;
import ir.value.BasicBlock;
import ir.value.ConstNumber;

import java.util.LinkedHashMap;
import java.util.LinkedList;

public class Fptosi extends Instr {
    public Fptosi(LinkedList<Value> uses, String name, BasicBlock basicBlock) {
        super(uses, new IntType(), name, basicBlock);
    }

    @Override
    public String toString() {
        return String.format("%s = fptosi %s to %s",
            getFullName(), getOP(0).getNameWithType(), this.type.toString());
    }

    @Override
    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap) {
        LinkedList<Value> uses = new LinkedList<>();
        for (int i = 0; i < this.getOperands().size(); i++) {
            Value temp = this.getOperands().get(i);
            if (temp instanceof ConstNumber) {
                uses.add(temp);
            } else {
                assert valueHashMap.containsKey(temp);
                uses.add(valueHashMap.get(temp));
            }
        }
        Fptosi fptosi = new Fptosi(uses, name, basicBlock);
        valueHashMap.put(this, fptosi);
        return fptosi;
    }
}