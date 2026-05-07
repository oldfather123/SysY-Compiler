package ir.instr;

import ir.Value;
import ir.type.Type;
import ir.type.VoidType;
import ir.value.BasicBlock;

import java.util.LinkedList;

public class Move extends Instr{

    public Move(LinkedList<Value> uses,
                BasicBlock parent) {
        super(uses, new VoidType(), "", parent);
    }

    @Override
    public String toString() {
        return String.format("MOVE %s <-- %s", getOP(0).getNameWithType(), getOP(1).getNameWithType());
    }

    @Override
    public boolean hasName() {
        return false;
    }
}
