package midend.instr;

import midend.BasicBlock;
import midend.Function;
import midend.LLVMType.Type;
import midend.Value;

public class ConversionInstr extends Instruction {
    public ConversionInstr(Type type, InstrType instrType, Value value) {//, BasicBlock basicBlock) {
        super(type, "%reg" + (Value.num++), instrType); //ZEXT或TRUNC
        this.addValue(value);
        //this.setBasicBlock(basicBlock);
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
        return this.getName() + " = " + this.getInstrType().toString() +
                " " + value.getType().toString() + " " + value.getName() +
                " to " + this.getType().toString() + "\n";
    }

    @Override
    public ConversionInstr clone(BasicBlock block) {
        Value value = getValue();
        if (Function.cloneMap.containsKey(value)) {
            value = Function.cloneMap.get(value);
        }
        ConversionInstr copy = new ConversionInstr(this.getType(), this.getInstrType(), value);
        copy.setBasicBlock(block);
        return copy;
    }

    @Override
    public boolean canBeUsed() {
        return true;
    }
}
