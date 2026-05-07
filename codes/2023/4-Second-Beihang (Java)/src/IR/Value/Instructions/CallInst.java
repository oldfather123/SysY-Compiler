package IR.Value.Instructions;

import IR.Value.BasicBlock;
import IR.Value.Function;
import IR.Value.Value;

import java.util.ArrayList;

public class CallInst extends Instruction{
    private boolean hasName = false;
    private Function function;

    public CallInst(Function function, ArrayList<Value> values) {
        super("%" + (++Value.valNumber), function.getType(), OP.Call);
        this.hasName = !function.getType().isVoidTy();
        this.function = function;
        for (Value value : values) {
            this.addOperand(value);
        }
    }

    public Function getFunction(){
        return function;
    }

    public ArrayList<Value> getParams(){
        return getUseValues();
    }

    @Override
    public boolean hasName(){
        return this.hasName;
    }

    @Override
    public String getInstString(){
        StringBuilder sb = new StringBuilder();
        if(!this.getType().isVoidTy()){
            sb.append(getName()).append(" = ");
        }
        sb.append("call ").append(getFunction().getType()).append(' ').append(getFunction().getName()).append("(");
        ArrayList<Value> operands = getOperands();
        for(int i = 0; i < operands.size(); i++){
            Value value = operands.get(i);
            sb.append(value.getType()).append(" ").append(value.getName());
            if(i != operands.size() - 1){
                sb.append(", ");
            }
        }
        sb.append(")");
        return sb.toString();
    }

}
