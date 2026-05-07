package backend.instruction;

import backend.component.ObjBlock;
import backend.component.ObjInstr;
import backend.operand.ObjOperand;
import utils.Pair;

import java.util.ArrayList;

public class ObjPhi extends ObjInstr {
    private ObjOperand dst;
    private ArrayList<Pair<ObjOperand, ObjBlock>> operands;

    public ObjPhi() {
        super("phi");
        operands = new ArrayList<>();
    }

    public ObjPhi(ObjOperand dst, ArrayList<Pair<ObjOperand, ObjBlock>> operands) {
        super("phi");
        this.dst = dst;
        this.operands = operands;
    }

    public ArrayList<Pair<ObjOperand, ObjBlock>> getOperands() {
        return operands;
    }

    public void addOperand(ObjOperand operand, ObjBlock block) {
        operands.add(new Pair<>(operand, block));
    }

    public ObjOperand getDst() {
        return dst;
    }

    @Override
    public String toString() {
        String string = getType();
        string += "\t" + dst;
        for (int i = 0; i < operands.size(); i++) {
            string += "\t[" + operands.get(i).getFirst() + "," + operands.get(i).getSecond().getName() + "]";
        }
        return string;
    }
}
