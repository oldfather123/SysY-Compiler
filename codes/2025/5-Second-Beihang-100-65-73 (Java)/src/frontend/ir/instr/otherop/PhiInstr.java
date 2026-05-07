package frontend.ir.instr.otherop;

import frontend.ir.Value;
import frontend.ir.constant.FloatConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.Type;
import frontend.ir.structure.BasicBlock;
import midend.ConstFold;

import java.util.*;

public class PhiInstr extends Instruction {
    private final Map<BasicBlock, Value> operandMap;
    private boolean isLCSSA;

    public PhiInstr(Type type, Integer regIndex, BasicBlock parentBB) {
        super(type, regIndex, parentBB);
        this.operandMap = new HashMap<>();
        this.isLCSSA = false;
    }

    public PhiInstr(Type type, Integer regIndex, BasicBlock parentBB, boolean isLCSSA) {
        super(type, regIndex, parentBB);
        this.operandMap = new HashMap<>();
        this.isLCSSA = isLCSSA;
    }

    public boolean isLCSSA() {
        return isLCSSA;
    }

    public void setLCSSA(boolean isLCSSA) {
        this.isLCSSA = isLCSSA;
    }

    public void addOperand(BasicBlock srcBlk, Value val) {
        if (this.getUsedList().contains(srcBlk)) {
            throw new RuntimeException("PhiInstr addOperand error");
        }
        this.operandMap.put(srcBlk, val);
        this.setUse(srcBlk);
        this.setUse(val);
    }

    public Map<BasicBlock, Value> getOperandMap() {
        return operandMap;
    }

    public Set<BasicBlock> getOperandBlocks() {
        return operandMap.keySet();
    }

    public void selfFix() {
        Set<BasicBlock> preSet = new HashSet<>(this.getParentBB().getPres());
        Set<BasicBlock> keySet = new HashSet<>(this.operandMap.keySet());
        int counter = 0;
        for (BasicBlock key : keySet) {
            if (!preSet.contains(key)) {
                counter++;
                this.getUsedList().remove(key);
                key.getUserList().remove(this);
                this.getUsedList().remove(this.operandMap.get(key));
                this.operandMap.get(key).getUserList().remove(this);
                this.operandMap.remove(key);
                if (counter >= 2) {
                    throw new RuntimeException("PhiInstr selfFix error");
                }
            }
        }
        this.simplify();
    }

    /**
     * 如果只剩下一个键值对则返回值，否则返回 null
     */
    public Value getOnlyVal() {
        if (this.operandMap.size() != 1) {
            return null;
        }
        for (BasicBlock key : this.operandMap.keySet()) {
            return this.operandMap.get(key);
        }
        return null;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        BasicBlock blkToChange = null;
        for (BasicBlock blk : operandMap.keySet()) {
            if (blk == from) {
                blkToChange = blk;
                break;
            }
            if (operandMap.get(blk) == from) {
                operandMap.put(blk, to);
            }
        }
        if (blkToChange != null) {
            Value val = operandMap.get(blkToChange);
            operandMap.remove(blkToChange);
            operandMap.put((BasicBlock) to, val);
        }
    }

    public void delOpPair(BasicBlock fromBB) {
        Value orginalValue = operandMap.get(fromBB);
        this.delUse(orginalValue);
        this.delUse(fromBB);
        operandMap.remove(fromBB);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(this.value2string()).append(" = phi ").append(this.getType().printIRType()).append(" ");
        boolean isFirst = true;
        for (BasicBlock blk : operandMap.keySet()) {
            if (isFirst) {
                isFirst = false;
            } else {
                sb.append(", ");
            }
            sb.append("[ ").append(operandMap.get(blk).value2string()).append(", ");
            sb.append("%").append(blk.value2string()).append(" ]");
        }
        return sb.toString();
    }

    @Override
    public String myHash() {
        return Integer.toString(this.hashCode());
    }

    @Override
    public Value simplify() {
        if (isRemoved) {
            return this;
        }

        Value valStaged = null;
        for (Value val : operandMap.values()) {
            if (valStaged == null) {
                valStaged = val;
                continue;
            }

            if (!isEquivalent(valStaged, val)) {
                return this;
            }
        }
        if (valStaged == null) {
            return this;
        }
        if (this.getUserList().contains(valStaged)) {
            return this;
        }
        ArrayList<Value> users = new ArrayList<>(this.getUserList());
        this.replaceUseWith(valStaged);
        this.removeFromList();
        if (ConstFold.depthCounter < ConstFold.MAX_DEPTH) {
            for (Value user : users) {
                if (user instanceof Instruction ins) {
                    ConstFold.depthCounter++;
                    ins.simplify();
                } else {
                    throw new RuntimeException("PhiInstr simplify error: user not instruction");
                }
            }
        } else {
            ConstFold.depthCounter = 0;
        }
        return valStaged;
    }

    private boolean isEquivalent(Value a, Value b) {
        if (a instanceof IntConst ai && b instanceof IntConst bi) {
            return ai.getConstVal().intValue() == bi.getConstVal().intValue();
        } else if (a instanceof FloatConst af && b instanceof FloatConst bf) {
            return af.getConstVal().floatValue() == bf.getConstVal().floatValue();
        } else if (a instanceof Instruction ia && b instanceof Instruction ib) {
            return Objects.equals(ia.getRegIndex(), ib.getRegIndex());
        }
        return false;
    }

    public HashSet<BasicBlock> getKeyByValueMulti(Value value) {
        HashSet<BasicBlock> ret = new HashSet<>();
        for (Map.Entry<BasicBlock, Value> entry : this.operandMap.entrySet()) {
            if ((value == null && entry.getValue() == null) ||
                    (value != null && value.equals(entry.getValue()))) {
                ret.add(entry.getKey());
            }
        }
        return ret;
    }

    public void conditionalModifyValue(BasicBlock fromBB, Value oriVal, Value newVal) {
        if (this.operandMap.get(fromBB).equals(oriVal)) {
            this.operandMap.put(fromBB, newVal);
        }
        this.setUse(newVal);
        this.delUse(oriVal);
    }

    public void changeJumpFrom(BasicBlock origin, BasicBlock newBlock) {
        if (!operandMap.containsKey(origin)) {
            throw new RuntimeException("PhiInstr changeJumpFrom error: origin not found");
        }
        Value val = operandMap.get(origin);
        operandMap.remove(origin);
        operandMap.put(newBlock, val);
        this.delUse(origin);
        this.setUse(newBlock);
    }

    public HashSet<Value> getOperands() {
        return new HashSet<>(this.operandMap.values());
    }
}
