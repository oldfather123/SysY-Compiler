package ir.value.user;

import ir.Value;
import ir.pass.analyze.Loop;
import ir.type.FunctionType;
import ir.type.Type;
import ir.type.VoidType;
import ir.value.*;
import ir.value.Module;
import tools.IrRegDispatcher;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;

public class Function extends User {
    private Type retType;
    public ArrayList<BasicBlock> blocks = new ArrayList<BasicBlock>();
    private ArrayList<Type> args = new ArrayList<>();
    private ArrayList<Variable> argv = new ArrayList<>();
    private ArrayList<Arg> argv2 = new ArrayList<>();
    public Value returnValue = null;
    private IrRegDispatcher irRegDispatcher;
    public ArrayList<Loop> loops = new ArrayList<>();
    public LinkedHashSet<Function> getCallees() {
        return callees;
    }
    public LinkedHashSet<Function> getCallers() {
        return callers;
    }
    public IrRegDispatcher getIrRegDispatcher() {
        return irRegDispatcher;
    }

    public void printCallees() {
        for (var calleee : callees) {
            System.out.println(calleee.getFullName());
        }
    }

    public void setCallees(LinkedHashSet<Function> callees) {
        this.callees = callees;
    }

    private LinkedHashSet<Function> callees = new LinkedHashSet<>();
    private LinkedHashSet<Function> callers = new LinkedHashSet<>();
    //是否是运行时库函数
    public boolean isRunTime;
    //是否有副作用
    public boolean hasSideEffect = false;

    public Function(User user, String name, Type retType, IrRegDispatcher irRegDispatcher) {
        super(user, new FunctionType(), name);
        this.retType = retType;
        this.irRegDispatcher = irRegDispatcher;
    }

    public Function(User user, String name, IrRegDispatcher irRegDispatcher) {
        this(user, name, new VoidType(), irRegDispatcher);
    }

    public void setReturnValue(Value value) {
        this.returnValue = value;
    }

    public void addBlock(BasicBlock block) {
        blocks.add(block);
    }

    public ArrayList<Arg> getArgv2() {
        return argv2;
    }

    //用来加返回值
    public BasicBlock getExitBlock() {
        return blocks.get(2);
    }

    public void addArg(Type type, Variable variable, Arg arg) {
        args.add(type);
        argv.add(variable);
        argv2.add(arg);
    }

    public void addArgForBuiltin(Type type) {
        args.add(type);
    }

    public Type getRetType() {
        return retType;
    }

    public ArrayList<Type> getArgs() {
        return args;
    }

    public ArrayList<Variable> getArgv() {
        return argv;
    }

    @Override
    public String getFullName() {
        return "@" + name;
    }

    @Override
    public String getNameWithType() {
        return String.format("%s %s", retType, getFullName());
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("define dso_local ");
        sb.append(getNameWithType());
        sb.append('(');
        for (int i = 0; i < args.size(); i++) {
            sb.append(args.get(i).toString());
            sb.append(" %" + Integer.toString(i));
            if (i != args.size() - 1) {
                sb.append(", ");
            }
        }
        sb.append("){\n");
        for (var v : blocks) {
            sb.append(v.toString());
        }
        sb.append("}\n");
        return sb.toString();
    }

    public String declareToString() { //declare i32 @getfarray (float* )
        StringBuilder sb = new StringBuilder();
        sb.append("declare ");
        sb.append(getNameWithType());
        sb.append('(');
        for (int i = 0; i < args.size(); i++) {
            sb.append(args.get(i).toString());
            if (i != args.size() - 1) {
                sb.append(", ");
            }
        }
        sb.append(")\n");
        return sb.toString();
    }

    public void insertBefore(BasicBlock block, BasicBlock newblock) {
        int pos = blocks.indexOf(block);
        if (pos != -1) {
            blocks.add(pos, newblock);
        } else {
            assert false;
        }
    }

    public void insertAfter(BasicBlock block, BasicBlock newblock) {
        int pos = blocks.indexOf(block);
        if (pos != -1) {
            blocks.add(pos + 1, newblock);
        } else {
            assert false;
        }
    }

    public void deletBlock(BasicBlock block) {
        blocks.remove(block);
    }

    public Function clone(Module module, LinkedHashMap<Value, Value> valueHashMap,
                          LinkedHashMap<String, Variable> globals,
                          LinkedHashMap<String, ConstVariable> constGlobals)
            throws CloneNotSupportedException {
        Function clone = new Function(module, this.name, irRegDispatcher);
        clone.retType = this.retType;
        for (Variable var : globals.values()) {
            valueHashMap.put(var.allocInst, var.allocInst);
        }
        for (Variable var : constGlobals.values()) {
            valueHashMap.put(var.allocInst, var.allocInst);
        }
        for (Variable var : argv) {
            valueHashMap.put(var, var);
        }
        for (Arg var : argv2) {
            valueHashMap.put(var, var);
        }
        for (var block : this.blocks) {
            clone.addBlock(block.clone(clone, valueHashMap));
        }
        //二次遍历 处理Br跳转到后面的Block
        for (var block : clone.blocks) {
            block.reClone(valueHashMap);
        }
        clone.argv = argv;
        clone.args = args;
        clone.argv2 = argv2;
        if (this.returnValue != null) {
            clone.returnValue = valueHashMap.get(returnValue);
        }
        return clone;
    }

    public boolean isEmpty(){
        return blocks.size()==0;
    }
}
