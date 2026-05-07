package ir.instr;

import ir.IrInstr;
import ir.value.Value;

// 用于生成注释
public class CommentInstr extends IrInstr {
    private final String comment;

    public CommentInstr(String command) {
        this.comment = command;
    }

    @Override
    public String toString() {
        return "\t\t\t;"+this.comment;
    }

    public Value[] getOperands() {
        return new Value[]{};
    }

    public void replaceOperand(Value old, Value newv) {
    }

    @Override
    public OpCode getOpCode() {
        return OpCode.COMMENT;
    }
}
