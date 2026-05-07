package ir.instr;

import ir.Value;
import ir.type.Pointer;
import ir.type.Type;
import ir.value.BasicBlock;
import tools.IrRegDispatcher;

import java.util.LinkedHashMap;
import java.util.LinkedList;

public class Alloca extends Instr {
    private final Type allocType;

    //TODO：这里的LinkedList可能需要修改维护一下 使用Value内部 allocId来构建名字
    public Alloca(Type allocType, String name, BasicBlock parent, IrRegDispatcher dispatcher) {
        super((new LinkedList<>()), new Pointer(allocType),name+dispatcher.allocId(), parent);
        this.allocType = allocType;
    }

    public Alloca(Type allocType, String name, BasicBlock parent) {
        super((new LinkedList<>()), new Pointer(allocType), name, parent);
        this.allocType = allocType;
    }

    //重载，将类型包装成指针类型
    /*public Alloca(Type type, String name, Boolean inner) {
        super(new LinkedList<>(), type, name);
    }

    public Alloca(Type type, String name){
        this(new Pointer(type), name, false);
    }*/

    @Override
    public String toString() {
        return getFullName() + " = " + "alloca " + allocType;
    }

    @Override
    public Instr clone(BasicBlock basicBlock, LinkedHashMap<Value, Value> valueHashMap) {
        Alloca alloca = new Alloca(allocType, name, basicBlock);
        valueHashMap.put(this, alloca);
        return alloca;
    }

    public Type getAllocType() {
        return allocType;
    }
}
