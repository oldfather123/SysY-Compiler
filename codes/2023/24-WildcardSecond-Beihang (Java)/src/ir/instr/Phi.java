package ir.instr;

import ir.type.Type;
import ir.value.BasicBlock;
import ir.Value;
import ir.value.ConstNumber;

import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.Map;

public class Phi extends Instr {
    public LinkedHashMap<BasicBlock, Value> values;   //指令块和value的键值对
    private final Type phiType;

    private Alloca owner;

    public Alloca getOwner() {
        return owner;
    }

    public void setOwner(Alloca owner) {
        this.owner = owner;
    }

    public LinkedHashMap<BasicBlock, Value> getValues() {
        return values;
    }

    public void setValues(LinkedHashMap<BasicBlock, Value> values) {
        this.values = values;
    }

    public Phi(Type phiType, BasicBlock parent) {
        super(new LinkedList<>(), phiType, "tmpx", parent);
        this.phiType = phiType;
        this.values = new LinkedHashMap<>();
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(getFullName()).append(" = ");
        sb.append("phi ").append(phiType).append(" ");
        for (BasicBlock basicBlock : values.keySet()) {
            sb.append("[ ");
            sb.append(values.get(basicBlock).getFullName());
            sb.append(", ");
            sb.append("%" + basicBlock.getName());
            sb.append(" ], ");
        }
        sb.delete(sb.length() - 2, sb.length());
        return sb.toString();
    }


    public void addBlock2Value(BasicBlock block, Value value) {
        values.put(block, value);
        if(!this.getOperands().contains(value)){
            this.addValue(value);
            value.addUser(this);
        }
    }

    public void removeRedundant() {
        //TODO: 2023/7/20
    }

    @Override
    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap) {
        Phi newPhi = new Phi(phiType, basicBlock);
        for(Map.Entry<BasicBlock, Value> entry : values.entrySet()) {
            BasicBlock block = entry.getKey();
            Value value = entry.getValue();
            BasicBlock block1 = block;
            Value value1 = value;
            if (valueHashMap.containsKey(entry.getKey())) {
                block1 = (BasicBlock) valueHashMap.get(entry.getKey());
            }
            if (valueHashMap.containsKey(entry.getValue())) {
                value1 = valueHashMap.get(entry.getValue());
            }
            newPhi.addBlock2Value(block1, value1);
        }
        valueHashMap.put(this, newPhi);
        return newPhi;
    }

    @Override
    public void reClone(LinkedHashMap<Value, Value> valueHashMap) {
        LinkedHashMap<BasicBlock, Value> needAdd = new LinkedHashMap<>();
        for(Map.Entry<BasicBlock, Value> entry : values.entrySet()) {
            BasicBlock block = entry.getKey();
            Value value = entry.getValue();
            if (valueHashMap.containsKey(entry.getKey())) {
                block = (BasicBlock) valueHashMap.get(entry.getKey());
            }
            if (valueHashMap.containsKey(entry.getValue())) {
                value = valueHashMap.get(entry.getValue());
            }
            needAdd.put(block,value);
        }
        this.values.clear();
        this.getOperands().forEach(ope -> ope.users.remove(this));
        this.getOperands().clear();
        for (Map.Entry<BasicBlock, Value> basicBlockValueEntry : needAdd.entrySet()) {
            this.addBlock2Value(basicBlockValueEntry.getKey(), basicBlockValueEntry.getValue());
        }
    }

    public void checkAndRemoveBlock(BasicBlock basicBlock) {
        if (values.containsKey(basicBlock)) {
            this.getOperands().remove(values.get(basicBlock));
            values.get(basicBlock).users.remove(this);
            values.remove(basicBlock);
        }
    }

    public void replaceBlockWith(BasicBlock ori, BasicBlock newblock) {
        if(values.containsKey(ori)){
            Value tmp = values.get(ori);
            values.remove(ori);
            values.put(newblock, tmp);
        }
    }

    @Override
    public void replaceOp(Value old, Value newop) {
        super.replaceOp(old, newop);
        for(var bb : values.keySet()){
            if(values.get(bb) == old){
                values.put(bb, newop);
            }
        }
    }
}
