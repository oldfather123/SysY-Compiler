package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;

public class Instruction extends User {
    private BasicBlock block;

    public Instruction(String regName, ValueType VType) {
        super(regName, VType);
        IRManager.getInstance().instrCreated(this);
        block = IRManager.getInstance().getCurBlock();
    }

    public Instruction(String reg) {
        super(reg, ValueType.NULL);
        IRManager.getInstance().instrCreated(this);
        block = IRManager.getInstance().getCurBlock();
    }

    public String toString() {
        return reg + "\n";
    }

    public BasicBlock getBlock() {
        return block;
    }

    public void setBlock(BasicBlock block) {
        this.block = block;
    }

    public boolean isDefInstr() {
        return true;
    }

    public ArrayList<String> GVNHash() {
        String str = toString();
        if (str.contains("=")) {
            ArrayList<String> ret = new ArrayList<>();
            ret.add(str.substring(str.indexOf("=") + 1));
            return ret;
        } else {
            return null;
        }
    }

}
