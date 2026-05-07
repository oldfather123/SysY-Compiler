package midend.instr;

import midend.*;
import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.Type;
import midend.LLVMType.VoidType;

public class RetInstr extends Instruction {
    public RetInstr(Type type, String name, Value value, BasicBlock block) {
        super(type, name, InstrType.RET);
        this.addValue(value);
        super.setBasicBlock(block);
    }

    @Override
    public String getInstr() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("ret ");
        Value value = getValue(0);
        Type type = getType();
        if (type instanceof IntegerType) {
            stringBuilder.append(type.toString() + " ");
        } else if (type instanceof FloatType) {
            stringBuilder.append(type.toString() + " ");
        } else if (type instanceof VoidType){
            return "ret void\n";
        } else {
            throw new RuntimeException("RetInstruction return value type is not int or float");
        }
        if (value instanceof ConstInt) {
            stringBuilder.append(((ConstInt) value).getValue());
        } else if (value instanceof ConstFloat) {
            stringBuilder.append(((ConstFloat) value).getValue());
        } else {
            stringBuilder.append(value.getName());
        }
        stringBuilder.append("\n");
        return stringBuilder.toString();
    }
    public Value getValue(){
        return this.getValue(0);
    }

    @Override
    public RetInstr clone(BasicBlock block) {
        Value value = getValue();
        if (Function.cloneMap.containsKey(value)) {
            value = Function.cloneMap.get(value);
        }
        return new RetInstr(this.getType(), this.getName(), value, block);
    }
}
