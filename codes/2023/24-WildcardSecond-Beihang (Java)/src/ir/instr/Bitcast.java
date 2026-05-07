package ir.instr;

import ir.Value;
import ir.type.Pointer;
import ir.type.Type;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import tools.IrRegDispatcher;

import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedList;

public class Bitcast extends Instr {
    public boolean crossCast = false;
    public Bitcast(Type castType, Value value, BasicBlock parent, IrRegDispatcher dispatcher) {
        super(new LinkedList<>(Arrays.asList(value)), castType, "cast" + dispatcher.allocId(), parent);
    }

    public Bitcast(Type castType, Value value, BasicBlock parent, String name) {
        super(new LinkedList<>(Arrays.asList(value)), castType, name, parent);
    }

    public String toString() {
        return getFullName() + " = " + "bitcast " +
                this.getOperands().get(0).getNameWithType() + " to " + type;
    }

    @Override
    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap) {
        assert valueHashMap.containsKey(this.getOperands().get(0));
        Bitcast bitcast = new Bitcast(type, valueHashMap.get(this.getOperands().get(0)),
                basicBlock, name);
        valueHashMap.put(this, bitcast);
        return bitcast;
    }
}
