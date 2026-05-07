package backend.operand;

import backend.component.RiscvInstr;
import tools.DoubleNode;

import java.util.ArrayList;
import java.util.LinkedHashSet;

public class RiscvOperand {
    private LinkedHashSet<DoubleNode<RiscvInstr>> user = new LinkedHashSet<>();

    // 在翻译时使用该指令前 调用beUsed
    public RiscvOperand beUsed(DoubleNode<RiscvInstr> riscvInstr) {
        this.user.add(riscvInstr);
        return this;
    }

    public void replaceAllUser(DoubleNode<RiscvInstr> self, DoubleNode<RiscvInstr> riscvInstr) {
        LinkedHashSet<DoubleNode<RiscvInstr>> removes = new LinkedHashSet<>();
        for (DoubleNode<RiscvInstr> userInstr : user) {
            if (userInstr.equals(self)) {
                continue;
            }
            removes.add(userInstr);
        }
        for (DoubleNode<RiscvInstr> userInstr : removes) {
            userInstr.getContent().replaceOperands((RiscvReg) this, riscvInstr.getContent().getDefReg(), userInstr);
//            riscvInstr.getContent().getDefReg().beUsed(userInstr);
        }
    }

    public LinkedHashSet<DoubleNode<RiscvInstr>> getUser() {
        return user;
    }
}
