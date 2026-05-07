package midend.instr;

import midend.BasicBlock;
import midend.Function;
import midend.LLVMType.Type;
import midend.Value;

import java.util.ArrayList;

public class CallInstr extends Instruction {
    private Function function;
    public CallInstr(Type type, Function function, ArrayList<Value> values) {
        super(type, "%reg" + (Value.num++), InstrType.CALL);
        this.function = function;
        for (Value value : values) {
            this.addValue(value);
        }
    }

    public Function getFunction() {
        return this.function;
    }

    public ArrayList<Value> getValues() {
        return this.getValueList();
    }

    @Override
    public boolean canUse() {
        return !getType().isVoid();
    }

    @Override
    public String getInstr() {
        Function function1 = getFunction();
        StringBuilder stringBuilder = new StringBuilder();
        if (!function1.getRetType().isVoid()) {
            stringBuilder.append(this.getName()).append(" = ");
        }
        stringBuilder.append("call ").append(function1.getRetType().toString());
        stringBuilder.append(" " + function1.getName() + "(");
        ArrayList<Value> values = getValues();
        for (int count = 0; count < values.size(); count++) {
            Value value = values.get(count);
            stringBuilder.append(value.getType().toString() + " " + value.getName());
            if (count != values.size() - 1) {
                stringBuilder.append(", ");
            }
        }
        stringBuilder.append(")\n");
        return stringBuilder.toString();
    }

    @Override
    public CallInstr clone(BasicBlock block) {
        ArrayList<Value> values = new ArrayList<>();
        for (Value value : getValues()) {
            Value value1 = value;
            if (Function.cloneMap.containsKey(value1)) {
                value1 = Function.cloneMap.get(value1);
            }
            values.add(value1);
        }
        CallInstr copy = new CallInstr(this.getType(), this.getFunction(), values);
        copy.setBasicBlock(block);
        return copy;
    }

    @Override
    public boolean canBeUsed() {
        return false;
    }
}
