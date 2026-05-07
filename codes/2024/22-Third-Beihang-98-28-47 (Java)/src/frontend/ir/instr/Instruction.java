package frontend.ir.instr;

import Utils.CustomList;
import frontend.ir.Use;
import frontend.ir.structure.BasicBlock;
import frontend.ir.Value;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

import java.util.ArrayList;

public abstract class Instruction extends Value {
    private BasicBlock parentBB;
    protected ArrayList<Use> useList;
    protected ArrayList<Value> useValueList;
    
    public Instruction() {
        this.useList = new ArrayList<>();
        this.useValueList = new ArrayList<>();
    }
    
    public void setUse(Value value) {
        Use use = new Use(this,value);
        value.insertAtTail(use);
        useList.add(use);
        useValueList.add(value);
    }

    public void setParentBB(BasicBlock parentBB) {
        this.parentBB = parentBB;
    }
    
    public abstract String print();
    
    public BasicBlock getParentBB() {
        return parentBB;
    }
    
    @Override
    public String value2string() {
        return "%reg_" + this.getNumber();
    }
    
    /**
     * 修改当前的指令的use，将原来对from的使用改为对to的使用,以及对from的值的使用会变为对to的值的使用
     */
    public void modifyUse(Value from, Value to) {
        for (Use one: useList) {
            if (one.getUsed() == from) {
                one.removeFromList();
                useList.remove(one);
                useValueList.remove(from);
                // useValueList.add(to);
                setUse(to);
                modifyValue(from, to);
                return;
            }
        }
    }
    public abstract void modifyValue(Value from, Value to);

    public void removeFromList() {
        super.removeFromList();
        for (Use use : useList) {
            use.removeFromList();
        }
    }
    
    public void removeFromListWithUseRemain() {
        super.removeFromList();
        this.setNext(null);
        this.setPrev(null);
    }

    public void removeUse(Value value) {
        useValueList.remove(value);
        for (Use use : useList) {
            if (use.getUsed() == value) {
                useList.remove(use);
                use.removeFromList();
                return;
            }
        }
    }

    public ArrayList<Value> getUseValueList() {
        return useValueList;
    }
    
    public abstract String myHash();
    
    /**
     * 用来简化运算指令,如果可以化成常数或进行部分简化则做简化,否则返回 null 说明无法简化
     */
    public abstract Value operationSimplify();
    
    @Override
    public void insertAfter(CustomList.Node node) {
        super.insertAfter(node);
        this.parentBB = ((Instruction) node).parentBB;
    }
    
    public void insertBefore(CustomList.Node node) {
        super.insertBefore(node);
        this.parentBB = ((Instruction) node).parentBB;
    }

    @Override
    public void setParent(CustomList.Node node) {
        assert node instanceof Instruction;
        this.parentBB = ((Instruction) node).parentBB;
        super.setParent(node);
    }
    
    public Instruction cloneShell(Function parentFunc) {
        return this.cloneShell(parentFunc.getProcedure());
    }
    
    public abstract Instruction cloneShell(Procedure procedure);
}
