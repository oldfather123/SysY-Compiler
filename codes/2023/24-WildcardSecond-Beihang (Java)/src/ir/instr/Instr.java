package ir.instr;

import ir.Value;
import ir.type.Type;
import ir.value.BasicBlock;
import ir.value.User;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedList;

public abstract class Instr extends User {
    private InstType instType;
    private BasicBlock parent;


    private boolean toBeDelete;

    public boolean isToBeDelete() {
        return toBeDelete;
    }

    public void setToBeDelete(boolean toBeDelete) {
        this.toBeDelete = toBeDelete;
    }

    public Instr(LinkedList<Value> uses, Type type, String name, BasicBlock parent) {
        super(uses, type, name);
        this.parent = parent;
    }

    public InstType getInstType() {
        return instType;
    }

    public void setInstType(InstType instType) {
        this.instType = instType;
    }

    public BasicBlock getParent() {
        return parent;
    }

    public void setParent(BasicBlock parent) {
        this.parent = parent;
    }


    public String getNameWithType(){
        return type.toString()+" "+getFullName();
    }

    public boolean isEnd(){
        return false;
    }

    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap){
        return null;
    }

    public void reClone(LinkedHashMap<Value, Value> valueHashMap) {}



}

