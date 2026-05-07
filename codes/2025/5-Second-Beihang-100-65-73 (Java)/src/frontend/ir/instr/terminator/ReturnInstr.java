package frontend.ir.instr.terminator;

import frontend.ir.Value;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class ReturnInstr extends Terminator {
    private Value returnValue;

    public ReturnInstr(Value returnValue, BasicBlock parentBB) {
        super(parentBB);
        this.returnValue = returnValue;

        if (parentBB.isNotClosed()) {
            if (returnValue != null) {
                this.setUse(returnValue);
            }

            parentBB.setUse(this);
        }
        parentBB.close();
    }

    public Value getReturnValue() {
        return returnValue;
    }

    @Override
    public void replaceJumpTarget(BasicBlock original, BasicBlock newTarget) {
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (this.returnValue == from) {
            this.returnValue = to;
        }
    }

    @Override
    public String toString() {
        if (returnValue == null) {
            return "ret void";
        } else {
            return "ret " + returnValue.getType().printIRType() + " " + returnValue.value2string();
        }
    }

    public HashSet<Value> getOperands() {
        return new HashSet<>();
    }
}
