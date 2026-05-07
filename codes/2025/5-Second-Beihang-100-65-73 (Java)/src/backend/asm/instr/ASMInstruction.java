package backend.asm.instr;

import backend.asm.register.Reg;
import backend.asm.register.vir.VirReg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.ASMValue;
import util.CustomList;

import java.util.*;

public abstract class ASMInstruction extends CustomList.Node implements Cloneable {
    private ASMBasicBlock parentBlock;
    private Reg register;
    private final Set<VirReg> liveOutSet;
    private final List<ASMValue> usedValList = new LinkedList<>();   // this 使用的 value

    protected ASMInstruction(ASMBasicBlock parentBlock, Reg register) {
        this.parentBlock = parentBlock;
        this.register = register;
        if (register instanceof VirReg virReg) {
            virReg.addDefIns(this);
        }
        if (parentBlock != null) {
            parentBlock.insertInstr2Tail(this);
        }

        this.liveOutSet = new LinkedHashSet<>();
    }

    public void addUsedVal(ASMValue usedVal) {
        this.usedValList.add(usedVal);
        usedVal.addUser(this);
    }
    
    public void replaceUsedVal(ASMValue from, ASMValue to) {
        List<ASMValue> newOperands = new ArrayList<>();
        for (ASMValue oldOperand : this.getOperands()) {
            if (oldOperand == from) {
                newOperands.add(to);
            } else {
                newOperands.add(oldOperand);
            }
        }
        this.resetOperands(newOperands);
    }
    
    public void changeRegTo(Reg newReg) {
        if (this.register instanceof VirReg virReg) {
            virReg.deleteDefIns(this);
        }
        if (newReg instanceof VirReg newVirReg) {
            newVirReg.addDefIns(this);
        }
        this.register = newReg;
    }

    public List<ASMValue> getUsedValList() {
        return usedValList;
    }

    public void modifyUse(ASMValue from, ASMValue to) {
        if (!this.usedValList.contains(from)) {
            throw new RuntimeException("no such used value to modify");
        }
        for (int i = 0; i < this.usedValList.size(); i++) {
            if (this.usedValList.get(i) == from) {
                this.usedValList.set(i, to);
                break;
            }
        }

        from.deleteUser(this);
        to.addUser(this);
    }

    public Reg getRegister() {
        return this.register;
    }

    public abstract List<ASMValue> getOperands();

    public abstract void resetOperands(List<ASMValue> values);

    public String hash() {
        String str = this.printIns();
        int firstSpace = str.indexOf(' ');
        if (firstSpace == -1) return str; // 没有空格

        int secondSpace = str.indexOf(' ', firstSpace + 1);
        if (secondSpace == -1) return str; // 只有一个空格

        // 拼接前半部分和后半部分
        return str.substring(0, firstSpace + 1) + str.substring(secondSpace);
    }

    public Set<VirReg> getLiveOutSet() {
        return liveOutSet;
    }

    @Override
    public void insertAfter(CustomList.Node node) {
        super.insertAfter(node);
        if (node instanceof ASMInstruction ins) {
            this.parentBlock = ins.parentBlock;
        } else {
            throw new RuntimeException("ins can only be inserted into ins list");
        }
    }

    @Override
    public void insertBefore(CustomList.Node node) {
        super.insertBefore(node);
        if (node instanceof ASMInstruction ins) {
            this.parentBlock = ins.parentBlock;
        } else {
            throw new RuntimeException("ins can only be inserted into ins list");
        }
    }

    public ASMBasicBlock getParentBlock() {
        return parentBlock;
    }

    protected abstract String printIns();

    public String printAsOperand() {
        return this.register.toString();
    }

    @Override
    public void removeFromList() {
        for (ASMValue val : this.usedValList) {
            val.deleteUser(this);
        }
        if (this.register instanceof VirReg virReg) {
            virReg.deleteDefIns(this);
        }
        super.removeFromList();
    }

    @Override
    public String toString() {
        return this.printIns();
    }

    @Override
    public ASMInstruction clone() throws CloneNotSupportedException {
        return (ASMInstruction) super.clone();
    }
}
