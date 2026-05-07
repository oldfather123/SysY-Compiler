package midend.instr;

import midend.LLVMType.Type;
import midend.LLVMType.VoidType;
import midend.Value;

public class MoveInstr extends Instruction {
    public MoveInstr(Value src, Value dst) {
        super(VoidType.voidType, "move", InstrType.MOVE);
        addValue(src);
        addValue(dst);
    }

    public Value getSrc() {
        return getValue(0);
    }

    public Value getDst() {
        return getValue(1);
    }

    public void setSrc(Value value) {
        setValue(0, value);
    }

    public void setDst(Value value) {
        setValue(1, value);
    }

    @Override
    public String getInstr() {
        Value dst = getDst();
        Value src = getSrc();
        return "move " + dst.getType() + " " + dst.getName() + ", " + src.getName() + "\n";
    }
}
