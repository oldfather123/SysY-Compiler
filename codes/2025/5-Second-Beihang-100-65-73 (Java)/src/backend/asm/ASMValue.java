package backend.asm;

import backend.asm.instr.ASMInstruction;
import util.CustomList;

import java.util.LinkedHashSet;
import java.util.Set;

public abstract class ASMValue extends CustomList.Node {
    private final Set<ASMInstruction> userSet = new LinkedHashSet<>();      // 使用 this 的 value
    
    public void replaceWith(ASMValue newValue) {
        Set<ASMInstruction> userSet = new LinkedHashSet<>(this.userSet);
        for (ASMInstruction userIns : userSet) {
            userIns.replaceUsedVal(this, newValue);
        }
    }
    
    public void addUser(ASMInstruction user) {
        this.userSet.add(user);
    }
    
    public void deleteUser(ASMInstruction user) {
        this.userSet.remove(user);
    }
    
    public Set<ASMInstruction> getUserSet() {
        return userSet;
    }
    
    public abstract String printAsOperand();
}
