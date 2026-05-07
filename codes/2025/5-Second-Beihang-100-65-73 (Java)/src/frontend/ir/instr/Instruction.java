package frontend.ir.instr;

import frontend.ir.Value;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.objecttype.Type;
import frontend.ir.structure.BasicBlock;
import midend.FunctionInfoCollector;
import util.CustomList;

import java.util.HashSet;

public abstract class Instruction extends Value {
    private final Integer regIndex;
    private BasicBlock parentBB;
    protected boolean isRemoved = false;

    protected Instruction(Type type, Integer regIndex, BasicBlock parentBB) {
        super(type);
        this.regIndex = regIndex;
        if (parentBB == null) {
            throw new NullPointerException();
        }
        if (parentBB.isNotClosed()) {
            parentBB.addInstruction(this);
        }
    }

    public abstract String myHash();

    /**
     * 尝试简化这个指令，如果成功了直接将自己替换使用为对应的指令并删除自身，然后返回取代自己的指令；如果失败返回空
     */
    public abstract Value simplify();

    public Integer getRegIndex() {
        return regIndex;
    }

    /**
     * 从列表中删除，但是不影响 use 关系等，同时清除该节点与两端的连接
     */
    public void pureRemoveFromList() {
        super.removeFromList();
        this.setPrev(null);
        this.setNext(null);
    }

    protected abstract void modifyValue(Value from, Value to);

    @Override
    public void modifyUse(Value from, Value to) {
        super.modifyUse(from, to);
        this.modifyValue(from, to);
    }

    public void modifyUseV2(Value from, Value to) {
        this.modifyUse(from, to);
        from.getUserList().remove(this);
        to.getUserList().add(this);
    }

    @Override
    public void removeFromList() {
        if (!this.getUserList().isEmpty()) {
            throw new RuntimeException("该指令仍被使用，不得删除");
        }

        this.forceRemoveFromList();
    }

    public void liteRemoveFromList() {
        super.removeFromList();
    }

    public void forceRemoveFromList() {
        for (Value used : this.getUsedList()) {
            used.getUserList().remove(this);
        }
        this.getUsedList().clear();
        super.removeFromList();
        this.isRemoved = true;
    }

    @Override
    public void insertAfter(CustomList.Node node) {
        super.insertAfter(node);
        if (node instanceof Instruction ins) {
            this.parentBB = ins.parentBB;
        } else {
            throw new RuntimeException("ins can only be inserted into ins list");
        }
    }

    @Override
    public void insertBefore(CustomList.Node node) {
        super.insertBefore(node);
        if (node instanceof Instruction ins) {
            this.parentBB = ins.parentBB;
        } else {
            throw new RuntimeException("ins can only be inserted into ins list");
        }
    }

    public boolean isPinned() {
        // todo GCM
        if (this instanceof CallInstr callInstr) {
            var info = FunctionInfoCollector.getFuncInfo(callInstr.getCallee());
            return info == null || !info.canGCM();
        }
        if (this instanceof GEPInstr gepInstr && gepInstr.getPtrVal().value2string().contains("parallel_payload")) {
            return true;
        }
        return this instanceof BranchInstr || this instanceof PhiInstr
                || this instanceof ReturnInstr || this instanceof StoreInstr
                || this instanceof LoadInstr || this instanceof JumpInstr || this instanceof AllocaInstr;
    }

    @Override
    public String value2string() {
        return "%reg_" + regIndex;
    }

    public BasicBlock getParentBB() {
        return parentBB;
    }

    public void setParentBB(BasicBlock parentBB) {
        this.parentBB = parentBB;
    }

    public abstract HashSet<Value> getOperands();

    public boolean dominates(Instruction maySub) {
        BasicBlock maySubBlock = maySub.getParentBB();

        if (this.parentBB == maySubBlock) {
            boolean hasMetMaySub = false;
            for (Instruction instr : this.parentBB.getInstrList()) {
                if (instr == this) {
                    return true;
                } else if (instr == maySub) {
                    hasMetMaySub = true;
                    break;
                }
            }
            return !hasMetMaySub;
        }

        return parentBB.dominates(maySubBlock);
    }

}
