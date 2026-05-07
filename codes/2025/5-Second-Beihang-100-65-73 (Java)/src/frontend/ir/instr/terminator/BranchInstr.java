package frontend.ir.instr.terminator;

import frontend.ir.Value;
import frontend.ir.constant.BoolConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class BranchInstr extends Terminator {
    private Value condition;
    private BasicBlock thenBlk;
    private BasicBlock elseBlk;
    private boolean hasSetSucAndPre = false;
    
    public BranchInstr(Value condition, BasicBlock thenBlk, BasicBlock elseBlk, BasicBlock parentBB) {
        super(parentBB);
        this.condition = condition;
        this.thenBlk = thenBlk;
        this.elseBlk = elseBlk;
        
        if (parentBB.isNotClosed()) {
            hasSetSucAndPre = true;
            this.setUse(condition);
            this.setUse(thenBlk);
            this.setUse(elseBlk);
            
            parentBB.addSuc(thenBlk);
            parentBB.addSuc(elseBlk);
            thenBlk.addPre(parentBB);
            elseBlk.addPre(parentBB);
            
            parentBB.setUse(this);
        }
        parentBB.close();
    }
    
    public Value getCondition() {
        return condition;
    }
    
    public BasicBlock getThenBlk() {
        return thenBlk;
    }
    
    public BasicBlock getElseBlk() {
        return elseBlk;
    }

    @Override
    public void replaceJumpTarget(BasicBlock original, BasicBlock newTarget) {
        if(elseBlk.equals(original)) {
            if(hasSetSucAndPre) {
                this.setUse(newTarget);
                if(getParentBB().delSuc(elseBlk)) {
                    getParentBB().addSuc(newTarget);
                }
                if(elseBlk.delPre(getParentBB())) {
                    newTarget.delPre(getParentBB());
                }
            }
            elseBlk = newTarget;
        }
        if(thenBlk.equals(original)) {
            if(hasSetSucAndPre) {
                this.setUse(newTarget);
                if(getParentBB().delSuc(thenBlk)) {
                    getParentBB().addSuc(newTarget);
                }
                if(thenBlk.delPre(getParentBB())) {
                    newTarget.delPre(getParentBB());
                }
            }
            thenBlk = newTarget;
        }
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (this.condition == from) {
            this.condition = to;
        }
        if (this.thenBlk == from) {
            BasicBlock parentBB = this.getParentBB();
            parentBB.delSuc(thenBlk);
            thenBlk.delPre(parentBB);
            
            this.thenBlk = (BasicBlock) to;
            
            parentBB.addSuc(thenBlk);
            thenBlk.addPre(parentBB);
        }
        if (this.elseBlk == from) {
            BasicBlock parentBB = this.getParentBB();
            parentBB.delSuc(elseBlk);
            elseBlk.delPre(parentBB);
            
            this.elseBlk = (BasicBlock) to;
            
            parentBB.addSuc(elseBlk);
            elseBlk.addPre(parentBB);
        }
    }
    
    @Override
    public void forceRemoveFromList() {
        BasicBlock parentBB = this.getParentBB();
        parentBB.delSuc(thenBlk);
        parentBB.delSuc(elseBlk);
        thenBlk.delPre(parentBB);
        elseBlk.delPre(parentBB);
        
        super.forceRemoveFromList();
    }
    
    @Override
    public String toString() {
        return "br " + condition.getType().printIRType() + " " + condition.value2string()
                + ", label %" + thenBlk.value2string() + ", label %" + elseBlk.value2string();
    }

    @Override
    public Value simplify() {
        if (isRemoved) {
            return this;
        }
        if (condition instanceof BoolConst cond) {
            BasicBlock targetBB;
            BasicBlock delBB;
            if (thenBlk == elseBlk) {
                targetBB = thenBlk;
                delBB = elseBlk;
                Instruction j = new JumpInstr(targetBB, this.getParentBB());
                j.setUse(targetBB);
                this.replaceUseWith(j);
                this.removeFromList();
                j.insertAfter(this);
                this.getParentBB().close();

                for (Instruction ins : delBB.getInstrList()) {
                    if (!(ins instanceof PhiInstr)) {
                        break;
                    } else {
                        ins.simplify();
                    }
                }

                return j;

            } else {
                if (cond.getConstVal().intValue() == 0) {
                    targetBB = elseBlk;
                    delBB = thenBlk;
                } else {
                    targetBB = thenBlk;
                    delBB = elseBlk;
                }

                Instruction j = new JumpInstr(targetBB, this.getParentBB());
                j.setUse(targetBB);
                j.insertAfter(this);
                this.replaceUseWith(j);
                this.removeFromList();
                this.getParentBB().close();
                j.getParentBB().addSuc(targetBB);
                targetBB.addPre(j.getParentBB());

                for (Instruction ins : delBB.getInstrList()) {
                    if (!(ins instanceof PhiInstr)) {
                        break;
                    } else {
                        if (((PhiInstr) ins).getOperandMap().containsKey(this.getParentBB())) {
                            ((PhiInstr) ins).delOpPair(this.getParentBB());
                            ins.simplify();
                        }
                    }
                }

                return j;
            }
        }
        return this;
    }

    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(condition);
        return operands;
    }
}
