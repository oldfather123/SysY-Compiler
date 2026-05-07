package midend.instr;

import midend.BasicBlock;
import midend.LLVMType.PointerType;
import midend.LLVMType.Type;
import midend.Value;

import java.util.ArrayList;

public class AllocaInstr extends Instruction {
    private ArrayList<Value> arrayValues = new ArrayList<>();

    public AllocaInstr(Type type, BasicBlock block) {
        super(type, "%reg" + (Value.num++), InstrType.ALLOCA);
        super.setBasicBlock(block);
    }

    public String getInstr() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append(this.getName() + " = ");
        stringBuilder.append("alloca " + ((PointerType) this.getType()).getElementType() .toString());
        stringBuilder.append("\n");
        return stringBuilder.toString();
    }

    public void setArrayValues(ArrayList<Value> values) {
        this.arrayValues = values;
    }

    public boolean canUse() {
        return true;
    }

    @Override
    public AllocaInstr clone(BasicBlock block) {
        AllocaInstr copy = new AllocaInstr(this.getType(), block);
        copy.setArrayValues(this.arrayValues);
        return copy;
    }

    @Override
    public boolean canBeUsed() {
        return true;
    }
}
