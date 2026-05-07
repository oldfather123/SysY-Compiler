package midend.instr;

import midend.BasicBlock;
import midend.Function;
import midend.LLVMType.Type;
import midend.Value;
import midend.Visitor;

public class StoreInstr extends Instruction {
    public StoreInstr(Type type, Value value, Value pointer, BasicBlock block) {
        super(type, "", InstrType.STORE);
        this.addValue(value);
        this.addValue(pointer);
        this.setBasicBlock(block);
    }

    public Value getValue() {
        return this.getValue(0);
    }

    public Value getPointer() {
        return this.getValue(1);
    }

    @Override
    public String getInstr() {
        StringBuilder stringBuilder = new StringBuilder();
        Value value = getValue();
        Value pointer = getPointer();
        stringBuilder.append("store ");
        stringBuilder.append(value.getType().toString() + " " + value.getName());
        stringBuilder.append(", ");
        stringBuilder.append(value.getType() + "* ");
        stringBuilder.append(pointer.getName());
        stringBuilder.append("\n");
        return stringBuilder.toString();
    }

    @Override
    public StoreInstr clone(BasicBlock block) {
        Value value = this.getValue();
        Value pointer = this.getPointer();
        if (Function.cloneMap.containsKey(value)) {
            value = Function.cloneMap.get(getValue());
        }
        if (Function.cloneMap.containsKey(pointer)) {
            pointer = Function.cloneMap.get(getPointer());
        }
        return new StoreInstr(this.getType(), value, pointer, block);
    }
}
