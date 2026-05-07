package midend.instr;

import midend.BasicBlock;
import midend.Function;
import midend.LLVMType.FloatType;
import midend.LLVMType.PointerType;
import midend.LLVMType.Type;
import midend.Value;

import java.util.ArrayList;

public class
GetElementPtrInstr extends Instruction {
    public GetElementPtrInstr(Type type, Value ptrval, ArrayList<Value> indexs) {
        super(type, "%reg" + (Value.num++), InstrType.GETELEMENTPTR);
        this.addValue(ptrval);
        for (Value index : indexs) {
            this.addValue(index);
        }
    }

    public Value getTarget() {
        return this.getValue(0);
    }

    @Override
    public boolean canUse() {
        return true;
    }

    public ArrayList<Value> getIndexs() {
        return this.getValueList();
    }
    public ArrayList<Value> getIndexes(){
        ArrayList<Value> indexes = new ArrayList<>();
        for(int i = 1; i < getValueList().size(); i++){
            indexes.add(getValue(i));
        }
        return indexes;
    }
    @Override
    public String getInstr() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append(this.getName()).append(" = getelementptr ");
//        stringBuilder.append(this.getType().toString()).append(" ");
        Value target = getTarget();
        stringBuilder.append(((PointerType)target.getType()).getElementType().toString());
        stringBuilder.append(", ");
        stringBuilder.append(target.getType().toString()).append(" ");
        stringBuilder.append(target.getName()).append(", ");
        ArrayList<Value> indexs = getIndexs();
        for (int count = 1; count < indexs.size(); count++) {
            Value value = indexs.get(count);
            stringBuilder.append(value.getType().toString() + " " + value.getName());
            if (count != indexs.size() - 1) {
                stringBuilder.append(", ");
            }
        }
        stringBuilder.append("\n");
        return stringBuilder.toString();
    }

    @Override
    public GetElementPtrInstr clone(BasicBlock block) {
        Value target = getTarget();
        ArrayList<Value> values = new ArrayList<>();
        for (int count = 1; count < getIndexs().size(); count++) {
            Value value = getIndexs().get(count);
            if (Function.cloneMap.containsKey(value)) {
                value = Function.cloneMap.get(value);
            }
            values.add(value);
        }
        if (Function.cloneMap.containsKey(target)) {
            target = Function.cloneMap.get(target);
        }
        GetElementPtrInstr copy = new GetElementPtrInstr(this.getType(), target, values);
        copy.setBasicBlock(block);
        return copy;
    }

    @Override
    public boolean canBeUsed() {
        return true;
    }
}
