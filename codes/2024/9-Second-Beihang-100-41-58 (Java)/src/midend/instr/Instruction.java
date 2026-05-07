package midend.instr;

import midend.BasicBlock;
import midend.LLVMType.Type;
import midend.User;

import java.util.LinkedList;

public class Instruction extends User {
    private InstrType instrType;
    private BasicBlock basicBlock;

    public Instruction(Type type, String name, InstrType instrType) {
        super(type, name);
        this.instrType = instrType;
    }

    public boolean canUse() {
        return false;
    }

    public InstrType getInstrType() {
        return this.instrType;
    }

    public String getInstr() {
        return "";
    }

    public void setInstrType(InstrType type) {
        this.instrType = type;
    }

    public BasicBlock getBasicBlock() {
        return basicBlock;
    }

    public void setBasicBlock(BasicBlock block) {
        this.basicBlock = block;
    }

    public void remove() {
        removeUseOfValue();
        if (this.basicBlock == null) {
            System.out.println("");
        }
        this.basicBlock.getInstrList().remove(this);
    }


    public Instruction clone(BasicBlock block) {
        return null;
    }

    public boolean canBeUsed() {
        return false;
    }
}
