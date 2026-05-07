package ir.instr;

import ir.Value;
import ir.type.Type;
import ir.type.VoidType;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.user.Function;
import tools.IrRegDispatcher;

import java.util.LinkedHashMap;
import java.util.LinkedList;

public class Call extends Instr {
    private final Function function;
    private boolean isVoid;

    public Call(Function function, LinkedList<Value> uses,
                 BasicBlock parent, IrRegDispatcher irRegDispatcher) {
        super(uses, function.getRetType(),
                function.name + ".ret" + irRegDispatcher.allocId(function.name), parent);

        this.function = function;
        this.isVoid = function.getRetType() instanceof VoidType;
    }

    public Call(Function function, LinkedList<Value> uses,
                BasicBlock parent, String name) {
        super(uses, function.getRetType(), name, parent);
        this.function = function;
        this.isVoid = function.getRetType() instanceof VoidType;
    }

    @Override
    public boolean hasName() {
        return !isVoid;
    }

    @Override
    public String getNameWithType() {
        return type.toString()+" " + getFullName();
    }

    @Override
    public String toString() {
        StringBuilder stringBuilder = new StringBuilder();
        if(!isVoid){
            stringBuilder.append(getFullName()+" = ");
        }
        stringBuilder.append("call ");
        stringBuilder.append(function.getNameWithType()+ "(");
        if(this.getOperands().size() != 0) {
            stringBuilder.append(function.getArgs().get(0) + " " + getOperands().get(0).getFullName());
        }
        for(int i = 1; i < getOperands().size();i++) {
            stringBuilder.append(", " + function.getArgs().get(i) + " " + getOperands().get(i).getFullName());
        }
        stringBuilder.append(")");
        return stringBuilder.toString();
    }

    public Function getFunction() {
        return this.function;
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
        Call call = new Call(function, uses, basicBlock, name);
        valueHashMap.put(this, call);
        return call;
    }
}
