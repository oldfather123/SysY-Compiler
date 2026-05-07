package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.stream.Collectors;

public class Br extends Instruction {

    public Br(Value cond, BasicBlock ifTrue, BasicBlock ifFalse) {
        super("BR");
        if (cond instanceof ConstNumber n) {
            Number con = n.getVal();
            if (con.intValue() == 0) {
                use(ifFalse);
            } else {
                use(ifTrue);
            }
        } else {
            use(cond);
            use(ifTrue);
            use(ifFalse);
        }
    }

    public Br(BasicBlock dest) {
        super("BR");
        use(dest);
    }

    public String toString() {
        Value cond = operandList.get(0);
        if (operandList.size() == 3) {
            return "br i1 " + cond.getReg() + ", label %" + operandList.get(1).getReg() +
                    ", label %" + operandList.get(2).getReg() + "\n";
        } else {
            return "br label %" + getDest().getReg() + "\n";
        }
    }

    public BasicBlock getIfTrue() {
        return (BasicBlock) operandList.get(1);
    }

    public BasicBlock getIfFalse() {
        return (BasicBlock) operandList.get(2);
    }

    public BasicBlock getDest() {
        if (operandList.get(0) instanceof BasicBlock) {
            return (BasicBlock) operandList.get(0);
        } else {
            return null;
        }
    }

    public Value getCond() {
        if (operandList.get(0) instanceof BasicBlock) {
            return null;
        } else {
            return operandList.get(0);
        }
    }

    public void redirectTo(BasicBlock originBlock, BasicBlock block) {
        for (int i = 0; i < operandList.size(); i++) {
            if (operandList.get(i).equals(originBlock)) {
                originBlock.removeUser(this);
                operandList.set(i, block);
                block.usedBy(this);
                return;
            }
        }
    }

    public boolean isUseless() {
        return false;
    }

    public boolean isDefInstr() {
        return false;
    }

    public void beReplacedBy(Value v) {
        /*
            br的替换有两种可能：
                1. br的cond确定，变为无条件跳转
                2. br的两个基本块被替代为一个，变为无条件跳转
            总之，br的变化会同时改变流图，因此需要同时对涉及的phi进行改变
            在情况1下，只需在被排除的目标块的phi中，直接将此br所在的块去掉
            在情况2下，相应的块会在bb.beReplacedBy中，被新的目标块代替，不用额外考虑
         */

        super.beReplacedBy(v);
        ArrayList<BasicBlock> blocksToDelete = new ArrayList<>();
        for (Value operand : operandList) {
            if (operand instanceof BasicBlock block) {
                blocksToDelete.add(block);
            }
        }
        blocksToDelete.remove(((Br) v).getDest());
        // btd就是当前br的所有目标块减去新的br的目标块
        if (blocksToDelete.size() > 1) {
            return;
        }
        BasicBlock blockToDelete = blocksToDelete.get(0);

        // 需要改变的phi是，从本块中原来能够跳转到的，之后则跳转不到的（即btd中的）块首部的phi
        ArrayList<User> phiToRemove = new ArrayList<>(this.getBlock().getUserList());
        phiToRemove.retainAll(blockToDelete.getInstructionList());
        for (User user : phiToRemove) {
            if (user instanceof Phi phi) {
                phi.removeOperand(this.getBlock());
            }
        }
    }
}
