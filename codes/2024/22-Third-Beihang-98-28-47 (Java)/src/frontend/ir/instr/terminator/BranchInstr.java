package frontend.ir.instr.terminator;

import debug.DEBUG;
import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PCInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

public class BranchInstr extends Terminator {
    private Value condition;
    private BasicBlock thenTarget;
    private BasicBlock elseTarget;

    public BranchInstr(Value cond, BasicBlock thenTarget, BasicBlock elseTarget) {
        this.condition = cond;
        this.thenTarget = thenTarget;
        this.elseTarget = elseTarget;
        setUse(thenTarget);
        setUse(elseTarget);
        setUse(cond);
    }

    public void setRelation(BasicBlock prt) {
        prt.getSucs().add(thenTarget);
        thenTarget.getPres().add(prt);
        prt.getSucs().add(elseTarget);
        elseTarget.getPres().add(prt);
    }

    @Override
    public void removeFromList() {
        if (this.getParentBB() == null) {
            throw new RuntimeException("why dont you have parent?");
        }
        BasicBlock prt = this.getParentBB();
        prt.getSucs().remove(thenTarget);
        thenTarget.getPres().remove(prt);
        prt.getSucs().remove(elseTarget);
        elseTarget.getPres().remove(prt);
        super.removeFromList();
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new BranchInstr(condition, thenTarget, elseTarget);
    }
    
    public Value getCond() {
        return condition;
    }

    public BasicBlock getThenTarget() {
        return thenTarget;
    }

    public BasicBlock getElseTarget() {
        return elseTarget;
    }

    @Override
    public DataType getDataType() {
        throw new RuntimeException("no data type");
    }

    @Override
    public String print() {
        return "br " + getCond().getDataType() + " " + condition.value2string() +
                ", label %" + getThenTarget().value2string() + ", label %" + getElseTarget().value2string();
    }

    @Override
    public void modifyValue(Value from, Value to) {
//        DEBUG.dbgPrint(this.print() + " try to replace " + from + " to " + to);
        if (condition == from) {
            condition = to;
        } else if (thenTarget == from) {
            thenTarget = (BasicBlock) to;
            this.getParentBB().getSucs().add((BasicBlock) to);
            this.getParentBB().getSucs().remove((BasicBlock) from);
            ((BasicBlock) to).getPres().add(this.getParentBB());
            ((BasicBlock) from).getPres().remove(this.getParentBB());
        } else if (elseTarget == from) {
            elseTarget = (BasicBlock) to;
            this.getParentBB().getSucs().add((BasicBlock) to);
            this.getParentBB().getSucs().remove((BasicBlock) from);
            ((BasicBlock) to).getPres().add(this.getParentBB());
            ((BasicBlock) from).getPres().remove(this.getParentBB());
        } else {
            throw new RuntimeException();
        }
    }
}

