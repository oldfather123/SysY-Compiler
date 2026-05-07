package midend.instr;

import midend.BasicBlock;
import midend.Function;
import midend.LLVMType.PointerType;
import midend.LLVMType.Type;
import midend.Value;

public class LoadInstr extends Instruction {
    public LoadInstr(Type type, Value value, BasicBlock block) {
        super(type, "%reg" + (Value.num++), InstrType.LOAD);
        this.addValue(value);
        this.setBasicBlock(block);
    }

    @Override
    public boolean canUse() {
        return true;
    }

    public Value getValue() {
        return this.getValue(0);
    }

    @Override
    public String getInstr() {
        Value value = getValue();
        Type type = ((PointerType) value.getType()).getElementType();
        return this.getName() + " = load " + type.toString()
                + ", " + value.getType().toString() + " " + value.getName() + "\n";
    }

    @Override
    public LoadInstr clone(BasicBlock block) {
        Value value = this.getValue();
        if (Function.cloneMap.containsKey(this.getValue())) {
            value = Function.cloneMap.get(this.getValue());
        }
        LoadInstr loadInstr = new LoadInstr(this.getType(), value, block);
        return loadInstr;
    }

    @Override
    public boolean canBeUsed() {
        return true;
    }
}
