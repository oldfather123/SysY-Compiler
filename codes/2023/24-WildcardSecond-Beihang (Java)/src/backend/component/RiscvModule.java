package backend.component;

import backend.operand.RiscvFloatConst;
import backend.operand.RiscvLabel;
import ir.Value;
import ir.value.Module;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashMap;

public class RiscvModule {
    //管理所有全局变量
    private final ArrayList<RiscvGlobalVariable> dataGlobalVariables = new ArrayList<>();
    private final ArrayList<RiscvGlobalVariable> bssGlobalVariables = new ArrayList<>();
    //管理所有函数
    private final LinkedHashMap<String, RiscvFunction> functions = new LinkedHashMap<>();
    private LinkedHashMap<Double, RiscvFloatConst> floats = new LinkedHashMap<>();
    private LinkedHashMap<Value, RiscvLabel> value2Label;

    public void setFloats(LinkedHashMap<Double, RiscvFloatConst> floats) {
        this.floats = floats;
    }

    public ArrayList<RiscvGlobalVariable> getDataGlobalVariables() {
        return dataGlobalVariables;
    }

    public ArrayList<RiscvGlobalVariable> getBssGlobalVariables() {
        return bssGlobalVariables;
    }

    public void addFunction(String name, RiscvFunction function) {
        this.functions.put(name, function);
    }

    public RiscvFunction getFunction(String name) {
        return this.functions.get(name);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        //TODO 貌似这个代码是没法直接跑起来的,再前面貌似需要加上一些别的关键字来指定体系结构
        if(!dataGlobalVariables.isEmpty()) {
            sb.append(".data\n");
            sb.append(".align\t4\n");
            for (var globalVariable : dataGlobalVariables) {
                sb.append(globalVariable.decl());
            }
        }

        if(!bssGlobalVariables.isEmpty()) {
            sb.append(".bss\n");
            sb.append(".align\t4\n");
            for (var globalVariable : bssGlobalVariables) {
                sb.append(globalVariable.decl());
            }
        }
        sb.append(".text\n");
        for(var function:functions.values()){
            if(function.getName().equals("main")){
                sb.append(function.toPrint());
                break;
            }
        }
        for(var function:functions.values()){
            if(!(function.getName().equals("main")))
                sb.append(function.toPrint());
        }
        for(RiscvFloatConst floatConst: floats.values()) {
            sb.append(floatConst);
        }
        return sb.toString();
    }

    public LinkedHashMap<String, RiscvFunction> getFunctions() {
        return functions;
    }

    public LinkedHashMap<Value, RiscvLabel> getValue2Label() {
        return value2Label;
    }

    public void setValue2Label(LinkedHashMap<Value, RiscvLabel> value2Label) {
        this.value2Label = value2Label;
    }
}
