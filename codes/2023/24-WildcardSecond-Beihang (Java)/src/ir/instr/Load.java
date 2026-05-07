package ir.instr;

import ir.Value;
import ir.type.Pointer;
import ir.value.BasicBlock;
import ir.value.ConstNumber;

import java.util.LinkedHashMap;
import java.util.LinkedList;

public class Load extends Instr{
    private  Value target;
    public Load(LinkedList<Value> uses, String name,
                BasicBlock parent) {
        super(uses, ((Pointer)uses.get(0).type).pointTo, name, parent);
    }

    public Value getTarget() {
        return getOP(0);
    }

    @Override
    public String toString() {
        return String.format("%s = load %s, %s",
            getFullName(), this.type, getOP(0).getNameWithType());
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
        Load load = new Load(uses, name, basicBlock);
        valueHashMap.put(this, load);
        return load;
    }
}
