package frontend.ir.instr.otherop;

import frontend.ir.DataType;
import frontend.ir.Use;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstValue;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Procedure;

import java.util.*;

public class PhiInstr extends Instruction {
    private final int result;
    private final DataType type;
    private ArrayList<Value> values;
    private ArrayList<BasicBlock> prtBlks;

    public PhiInstr(int result, DataType type, ArrayList<Value> values, ArrayList<BasicBlock> prtBlks) {
        this.result = result;
        this.type = type;
        this.values = values;
        this.prtBlks = prtBlks;
        for (Value value : values) {
            setUse(value);
        }
        //TODO: to set or not to set, is a question
        //TODO：刪除块时我的phi怎么办
//        for (BasicBlock block : prtBlks) {
//            setUse(block);
//        }
    }

    public ArrayList<Value> getValues() {
        return values;
    }

    public ArrayList<BasicBlock> getPrtBlks() {
        return prtBlks;
    }

    @Override
    public String value2string() {
        return "%phi_" + result;
    }

    @Override
    public Number getNumber() {
        throw new RuntimeException("Phi: There is no value!");
    }

    @Override
    public DataType getDataType() {
        return type;
    }

    @Override
    public String print() {
        StringBuilder ret = new StringBuilder(value2string() + " = phi " + getDataType() + " ");
        int len = values.size();
        for (int i = 0; i < len; i++) {
            Value value = values.get(i);
            //TODO：maybe异常处理？
            ret.append("[ ").append(value.value2string()).append(", ").append("%").append(prtBlks.get(i).value2string()).append(" ]");
            if (i < len - 1) {
                ret.append(", ");
            }
        }
        return ret.toString();
    }

    public void addUse(Value value, BasicBlock prtBlk) {
        setUse(value);
        values.add(value);
        prtBlks.add(prtBlk);
    }

    public void modifyUse(Value to, BasicBlock block) {
        int index = prtBlks.indexOf(block);
        Value from = values.get(index);
        super.modifyUse(from, to);
    }

    @Override
    public void modifyValue(Value from, Value to) {
        for (int i = 0; i < values.size(); i++) {
            if (values.get(i) == from) {
                values.set(i, to);
                return;
            }
        }
    }
    
    @Override
    public String myHash() {
        return Integer.toString(this.hashCode());
    }

    @Override
    public Value operationSimplify() {
        return null;
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new PhiInstr(procedure.getAndAddPhiIndex(), type, new ArrayList<>(values), new ArrayList<>(prtBlks));
    }

    public boolean simplify2const() {
        Value value = values.get(0);
        if (!(value instanceof ConstValue)) {
            return false;
        }
        for (Value other : values) {
            if (value != other) {
                return false;
            }
        }
        return true;
    }

    public boolean canBeReplaced() {
        if (values.size() != 1) {
            return false;
        }
        Value value = values.get(0);
        Use use = this.getBeginUse();
        while (use != null) {
            if (use.getUser() == value) {
                return false;
            }
            use = (Use) use.getNext();
        }
        return true;
    }

    public boolean canSimplify() {
        return simplify2const() | canBeReplaced();
    }
    
    public void renewBlocks(HashMap<Value, Value> old2new) {
        for (int i = 0; i < prtBlks.size(); i++) {
            BasicBlock oldBlk = prtBlks.get(i);
            if (old2new.containsKey(oldBlk)) {
                prtBlks.set(i, (BasicBlock) old2new.get(oldBlk));
            }
        }
    }
    @Override /*TODO: 删掉最后一个use？*/
    public void removeUse(Value value) {
        int index;
        if (value instanceof BasicBlock) {
            index = prtBlks.indexOf(value);
            super.removeUse(values.get(index));
        } else {
            index = values.indexOf(value);
            super.removeUse(value);
        }
        prtBlks.remove(index);
        values.remove(index);
    }

    public void modifyPrtBlk(BasicBlock oldBlk, BasicBlock newBlk) {
        prtBlks.set(prtBlks.indexOf(oldBlk),newBlk);
    }
}
