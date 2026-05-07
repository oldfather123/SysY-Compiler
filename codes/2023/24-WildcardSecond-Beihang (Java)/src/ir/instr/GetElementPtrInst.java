package ir.instr;

import ir.Value;
import ir.type.Pointer;
import ir.type.Type;
import ir.value.BasicBlock;
import ir.value.ConstNumber;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedList;

public class GetElementPtrInst extends Instr {


    public GetElementPtrInst(LinkedList<Value> uses, Type type, String name,
                             BasicBlock parent) {
        super(uses, type, name, parent);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(
            String.format("%s = getelementptr %s, %s", getFullName(),
                    ((Pointer)getOP(0).type).pointTo, getOP(0).getNameWithType()));
        for(int i = 1;i<getOperands().size();i++){
            Value value = getOP(i);
            sb.append(", ").append(value.getNameWithType());
        }
        return sb.toString();
    }

    @Override
    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap) {
        LinkedList<Value> uses = new LinkedList<>();
        for(int i = 0;i < this.getOperands().size();i++) {
            Value temp = this.getOperands().get(i);
            if(temp instanceof ConstNumber) {
                uses.add(temp);
            } else {
                assert valueHashMap.containsKey(temp);
                uses.add(valueHashMap.get(temp));
            }
        }
        GetElementPtrInst getElementPtrInst = new GetElementPtrInst(uses, this.type, name, basicBlock);
        valueHashMap.put(this, getElementPtrInst);
        return getElementPtrInst;
    }

}
