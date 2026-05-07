package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;

public class MoveIR extends Instruction {
    private final Value originPhi;

    public MoveIR(Phi dest, Value src) {
        super(dest.getReg(), ValueType.NULL);
        originPhi = dest;
        dest.addMoveIr(this);
        use(src);
    }

    public String toString() {
        return "move " + reg + ", " + operandList.get(0).getReg() + "\n";
    }

    public Phi getOriginPhi() {
        return (Phi) originPhi;
    }

    public boolean isUseless() {
        return originPhi.isUseless() || operandList.get(0).equals(originPhi);
    }

    public ArrayList<User> getUserList() {
        //move的user是原phi指令的user
        return originPhi.getUserList();
    }

    public Value getSrc() {
        return operandList.get(0);
    }

    public void destroy() {
        getOriginPhi().getMoveIrs().remove(this);
        operandList.get(0).removeUser(this);
    }
}
