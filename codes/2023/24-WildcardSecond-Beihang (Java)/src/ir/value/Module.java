package ir.value;

import ir.Value;
import ir.instr.Global;
import ir.type.Type;
import ir.value.user.Function;

import java.util.ArrayList;
import java.util.LinkedHashMap;

public class Module extends User {
    public BasicBlock init;
    public LinkedHashMap<String, Variable> globals = new LinkedHashMap<>();
    public LinkedHashMap<String, ConstVariable> constGlobals = new LinkedHashMap<>();
    public LinkedHashMap<String, Function> functions = new LinkedHashMap<>();


    public Module(User user, Type type, String name) {
        super(user, type, name);
    }

    public void addGlobal(Variable global) {
        this.globals.put(global.name, global);
    }

    public void addConstGlobal(ConstVariable constVar) {
        this.constGlobals.put(constVar.name, constVar);
    }

    public void buildGlobalAssign(BasicBlock initBlock) {
        for (var vari : globals.values()) {
            initBlock.addValue(
                new Global(vari, initBlock, false)); //在Global的初始化函数中设置了vari的lastInst为自身
        }
        for (var vari : constGlobals.values()) {
            initBlock.addValue(
                new Global(vari, initBlock, true)); //在Global的初始化函数中设置了vari的lastInst为自身
        }
    }

    public void addGlobalAssign(ConstVariable v, BasicBlock initBlock) {
        constGlobals.put(v.name, v);
        initBlock.addValue(new Global(v, initBlock, true)); //在Global的初始化函数中设置了vari的lastInst为自身
    }

    public void addFunction(String name, Function function) {
        this.functions.put(name, function);
    }

    public Function getFunction(String name) {
        return functions.get(name);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(init.toStringForGlobals()).append("\n");
        for (var func : functions.values()) {
            if (func.blocks.size() > 0) {
                continue;
            }
            sb.append(func.declareToString());
        }
        sb.append('\n');
        for (var func : functions.values()) {
            if (func.blocks.size() == 0) {
                continue;
            }
            sb.append(func.toString()).append("\n");
        }
        return sb.toString();
    }

    @Override
    public Object clone() throws CloneNotSupportedException {//不克隆variable
        Module clone = new Module(null, null, this.name);
        LinkedHashMap<Value, Value> valueHashMap = new LinkedHashMap<>();
        clone.init = this.init.clone(clone, valueHashMap);
        for (var funcName : functions.keySet()) {
            clone.addFunction(funcName,
                    (Function) functions.get(funcName).clone(clone, valueHashMap, globals, constGlobals));
        }
        return functions;
    }
}
