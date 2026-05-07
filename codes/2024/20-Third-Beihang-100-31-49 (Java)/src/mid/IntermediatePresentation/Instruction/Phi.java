package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;

public class Phi extends Instruction {
    private final Alloca originAddr;
    private final HashSet<MoveIR> toMoveIrs = new HashSet<>();


    public Phi(Alloca originAddr, String reg) {
        super(reg, originAddr.getRefType());
        this.originAddr = originAddr;
    }

    public Phi(boolean isInt, String reg) {
        super(reg, isInt ? ValueType.I32 : ValueType.FLT);
        this.originAddr = null;
    }

    public Phi(ValueType vType, String reg) {
        super(reg, vType);
        this.originAddr = null;
    }

    public void addCond(Value val, BasicBlock label) {
        if (operandList.contains(label)) {
            return;
        }
        if (!vType.isPointer() && val instanceof ConstNumber constNumber) {
            boolean isInt = vType == ValueType.I32;
            val = constNumber.withType(isInt);
        }
        use(val);
        use(label);
    }

    public ArrayList<BasicBlock> getSrcBlockWhen(Value value) {
        ArrayList<BasicBlock> sources = new ArrayList<>();
        for (int i = 0; i < operandList.size(); i += 2) {
            if (operandList.get(i).equals(value)) {
                sources.add((BasicBlock) operandList.get(i + 1));
            }
        }
        return sources;
    }

    public Alloca getPhiAddr() {
        return originAddr;
    }

    public Value valueFromBlock(BasicBlock label) {
        assert operandList.size() % 2 == 0;
        for (int i = 1; i < operandList.size(); i += 2) {
            Value v = operandList.get(i);
            if (v.equals(label)) {
                return operandList.get(i - 1);
            }
        }
        return null;
    }

    public String toString() {
        int size = operandList.size();
        assert size % 2 == 0;
        StringBuilder sb = new StringBuilder();
        sb.append(reg).append(" = ");
        sb.append("phi ").append(vType).append(" ");
        for (int i = 0; i < size; i += 2) {
            sb.append("[ ").append(operandList.get(i).getReg()).append(", %")
                    .append(operandList.get(i + 1).getReg()).append("], ");
        }
        sb.delete(sb.length() - 2, sb.length());
        sb.append("\n");
        return sb.toString();
    }

    public void redirectFrom(BasicBlock originBlock, BasicBlock midBlock) {
        originBlock.removeUser(this);
        int idx = operandList.indexOf(originBlock);
        operandList.set(idx, midBlock);
        midBlock.usedBy(this);
    }

    public void removeOperand(Value value) {
        if (!operandList.contains(value)) {
            value.removeUser(this);
            return;
        }
        if (value instanceof BasicBlock) {
            Value v = operandList.get(operandList.indexOf(value) - 1);
            operandList.remove(operandList.indexOf(value) - 1);
            if (!operandList.contains(v)) {
                // block是唯一的，而引用值不一定是
                v.removeUser(this);
            }
            value.removeUser(this);
            operandList.remove(value);
        } else if (value instanceof Alloca) {
            operandList.remove(value);
            if (!operandList.contains(value)) {
                value.removeUser(this);
            }
        } else {
            operandList.get(operandList.indexOf(value) + 1).removeUser(this);
            operandList.remove(operandList.indexOf(value) + 1);
            operandList.remove(value);
            if (!operandList.contains(value)) {
                value.removeUser(this);
            }
        }
    }

    public void replaceValueFrom(BasicBlock block, Value newValue) {
        for (int i = 0; i < operandList.size(); i++) {
            if (operandList.get(i).equals(block)) {
                operandList.get(i - 1).removeUser(this);
                operandList.set(i - 1, newValue);
                break;
            }
        }
        newValue.usedBy(this);
    }

    public void addMoveIr(MoveIR moveIR) {
        toMoveIrs.add(moveIR);
    }

    public HashSet<MoveIR> getMoveIrs() {
        return toMoveIrs;
    }

    public boolean isConst() {
        if (operandList.isEmpty()) {
            return false;
        }
        Value val = operandList.get(0);
        for (int i = 2; i < operandList.size(); i += 2) {
            // 这里ConstNumber类是重写了equals的
            if (!val.equals(operandList.get(i))) {
                return false;
            }
        }

        return true;
    }

    public ArrayList<String> GVNHash() {
        ArrayList<String> ret = new ArrayList<>();
        String str = toString();
        String hash = str.substring(str.indexOf("=") + 1);
        hash += getBlock().getReg();
        ret.add(hash);
        return ret;
    }
}
