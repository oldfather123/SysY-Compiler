package frontend.ir.instr.terminator;

import frontend.ir.Value;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class JumpInstr extends Terminator {
    private BasicBlock target;
    private boolean hasSetSucAndPre = false;

    public JumpInstr(BasicBlock target, BasicBlock parentBB) {
        super(parentBB);
        this.target = target;

        if (parentBB.isNotClosed()) {
            hasSetSucAndPre = true;
            this.setUse(target);

            parentBB.addSuc(target);
            target.addPre(parentBB);

            parentBB.setUse(this);
        }
        parentBB.close();
    }

    @Override
    public void replaceJumpTarget(BasicBlock original, BasicBlock newTarget) {
        if (target.equals(original)) {
            if (hasSetSucAndPre) {
                this.setUse(newTarget);
                if (getParentBB().delSuc(target)) {
                    getParentBB().addSuc(newTarget);
                }
                if (target.delPre(getParentBB())) {
                    newTarget.addPre(getParentBB());
                }
            }

            target = newTarget;
        }
    }

    public BasicBlock getTarget() {
        return target;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (this.target == from) {
            BasicBlock parentBB = this.getParentBB();
            parentBB.delSuc(target);
            target.delPre(parentBB);

            this.target = (BasicBlock) to;

            parentBB.addSuc(target);
            target.addPre(parentBB);
        }
    }

    @Override
    public void forceRemoveFromList() {
        BasicBlock parentBB = this.getParentBB();
        parentBB.delSuc(target);
        target.delPre(parentBB);

        super.forceRemoveFromList();
    }

    @Override
    public String toString() {
        return "br label %" + target.value2string();
    }

    public HashSet<Value> getOperands() {
        return new HashSet<>();
    }
}
