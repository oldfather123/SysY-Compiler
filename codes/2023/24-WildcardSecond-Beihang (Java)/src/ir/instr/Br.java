package ir.instr;

import ir.Value;
import ir.type.Type;
import ir.value.BasicBlock;

import java.util.LinkedHashMap;
import java.util.LinkedList;

public class Br extends Instr {
    public Br(LinkedList< Value > uses, BasicBlock parent) {
        super(uses, null, "", parent); //没有左值 所以没有name
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if(this.getOperands().size() == 1) {//直接跳
            sb.append("br label " + this.getOperands().get(0).getFullName());
        }else{
            sb.append("br " + this.getOperands().get(0).getNameWithType()
                + ", label " + this.getOperands().get(1).getFullName() +
                ", label " + this.getOperands().get(2).getFullName());
        }
        return sb.toString();
    }

    @Override
    public boolean hasName() {
        return false;
    }

    @Override
    public boolean isEnd() {
        return true;
    }

    @Override
    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap) {
        LinkedList<Value> uses = new LinkedList<>();
        for(int i = 0;i < this.getOperands().size();i++) {
            if(valueHashMap.containsKey(this.getOperands().get(i))) {
                uses.add(valueHashMap.get(this.getOperands().get(i)));
            } else {
                uses.add(this.getOperands().get(i));
            }
        }
        Br newBr = new Br(uses, basicBlock);
        return newBr;
    }

    @Override
    public void reClone(LinkedHashMap<Value, Value> valueHashMap) {
        for(int i = 0;i < this.getOperands().size();i++) {
            if(valueHashMap.containsKey(this.getOperands().get(i))) {
                Value temp = this.getOperands().get(i);
                this.getOperands().remove(i);
                this.getOperands().add(i, valueHashMap.get(temp));
            }
        }
    }
}
