package midend.value.instrs;

import midend.value.Value;

public class Move extends Instr {
    private Value source;
    public Move(Value source, Value target) {
        this.source = source;
        this.getOperandList().add(source);
        this.addUseOp(source, 0);
        this.getOperandList().add(target);
        this.addUseOp(target, 1);
        setType(target.getType());
        setName(target.getName());
    }

    public String toString() {
        Value source = this.getOperandList().get(0);
        Value target = this.getOperandList().get(1);
        return "move " + source.getName() + " to " + target.getName();
    }

    public Value getSource() {
        return source;
    }

}
