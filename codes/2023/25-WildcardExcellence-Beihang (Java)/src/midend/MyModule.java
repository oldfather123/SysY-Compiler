package midend;

import midend.value.BasicBlock;
import midend.value.Function;
import midend.value.global.GlobalString;
import midend.value.global.GlobalVariable;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.util.*;

/*
   顶层类，采用单例模式
 */
public class MyModule {
    public static final MyModule myModule = new MyModule();
    private final boolean debug = true;

    private ArrayList<GlobalVariable> globalVariables = new ArrayList<>();
    private ArrayList<GlobalString> globalStrings = new ArrayList<>();
    private Map<String, Function> functions = new LinkedHashMap<>();

    public HashSet<String> getInFunc() {
        return inFunc;
    }

    private HashSet<String> inFunc = new HashSet<>();

    public MyModule() {
        // 内部函数
        functions.put("getint", new Function(Type.IntegerType.I32, "getint", new ArrayList<>()));
        functions.put("getch", new Function(Type.IntegerType.I32, "getch", new ArrayList<>()));
        functions.put("getfloat", new Function(new Type.FloatType(), "getfloat", new ArrayList<>()));
        functions.put("getarray", new Function(Type.IntegerType.I32, "getarray", new ArrayList<>()));
        functions.put("getfarray", new Function(Type.IntegerType.I32, "getfarray", new ArrayList<>()));
        functions.put("putint", new Function(new Type.VoidType(), "putint", new ArrayList<>()));
        functions.put("putch", new Function(new Type.VoidType(), "putch", new ArrayList<>()));
        functions.put("putfloat", new Function(new Type.VoidType(), "putfloat", new ArrayList<>()));
        functions.put("putarray", new Function(new Type.VoidType(), "putarray", new ArrayList<>()));
        functions.put("putfarray", new Function(new Type.VoidType(), "putfarray", new ArrayList<>()));
        functions.put("putf", new Function(new Type.VoidType(), "putf", new ArrayList<>()));
        functions.put("starttime", new Function(new Type.VoidType(), "starttime", new ArrayList<>()));
        functions.put("stoptime", new Function(new Type.VoidType(), "stoptime", new ArrayList<>()));
        inFunc.addAll(functions.keySet());
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (GlobalVariable var : globalVariables) {
            sb.append(var).append("\n");
        }
        for (GlobalString string : globalStrings) {
            sb.append(string).append("\n");
        }
        for (Function function : functions.values()) {
            switch (function.getName()) {
                case "getint" -> sb.append("declare i32 @getint()\n");
                case "getch" -> sb.append("declare i32 @getch()\n");
                case "getfloat" -> sb.append("declare float @getfloat()\n");
                case "getarray" -> sb.append("declare i32 @getarray(i32*)\n");
                case "getfarray" -> sb.append("declare i32 @getfarray(float*)\n");
                case "putint" -> sb.append("declare void @putint(i32)\n");
                case "putch" -> sb.append("declare void @putch(i32)\n");
                case "putfloat" -> sb.append("declare void @putfloat(float)\n");
                case "putarray" -> sb.append("declare void @putarray(i32, i32*)\n");
                case "putfarray" -> sb.append("declare void @putfarray(i32, float*)\n");
                case "putf" -> sb.append("declare void @putf(i8*, ...)\n");
                case "starttime" -> sb.append("declare void @starttime()\n");
                case "stoptime" -> sb.append("declare void @stoptime()\n");
                default -> {
                }
            }
        }
        for (Function function : functions.values()) {
            if (!inFunc.contains(function.getName()) && !function.getName().equals("main")) {
                sb.append("define ");
                sb.append(function);
                sb.append("{\n");
                for (var block : function.getBasicBlocks()) {
                    sb.append(block.get().toString());
                }
                sb.append("}\n");
            }
        }
        Function function = functions.get("main");
        sb.append("define ");
        sb.append(function);
        sb.append("{\n");
        for (var block : function.getBasicBlocks()) {
            sb.append(block.get().toString());
        }
        sb.append("}\n");
        return sb.toString();
    }

    public String toMidOut(ArrayList<Function> functions, String filename) {
        StringBuilder sb = new StringBuilder();
        for (GlobalVariable var : globalVariables) {
            sb.append(var).append("\n");
        }
        for (GlobalString string : globalStrings) {
            sb.append(string).append("\n");
        }
        for (Function function : this.functions.values()) {
            switch (function.getName()) {
                case "getint" -> sb.append("declare i32 @getint()\n");
                case "getch" -> sb.append("declare i32 @getch()\n");
                case "getfloat" -> sb.append("declare float @getfloat()\n");
                case "getarray" -> sb.append("declare i32 @getarray(i32*)\n");
                case "getfarray" -> sb.append("declare i32 @getfarray(float*)\n");
                case "putint" -> sb.append("declare void @putint(i32)\n");
                case "putch" -> sb.append("declare void @putch(i32)\n");
                case "putfloat" -> sb.append("declare void @putfloat(float)\n");
                case "putarray" -> sb.append("declare void @putarray(i32, i32*)\n");
                case "putfarray" -> sb.append("declare void @putfarray(i32, float*)\n");
                case "putf" -> sb.append("declare void @putf(i8*, ...)\n");
                case "starttime" -> sb.append("declare void @starttime()\n");
                case "stoptime" -> sb.append("declare void @stoptime()\n");
                default -> {
                }
            }
        }
        for (Function function : functions) {
            sb.append("define ");
            sb.append(function);
            sb.append("{\n");
            for (var node : function.getBasicBlocks()) {
                BasicBlock block = node.get();
                sb.append(block.toString());
            }
            sb.append("}\n");
        }
        if (debug) {
            try {
                BufferedWriter out = new BufferedWriter(new FileWriter(filename));
                out.write(sb.toString());
                out.close();
            } catch (IOException e) {
                System.out.println("Something wrong!");
            }
        }
        return sb.toString();
    }

    public void addGlobalVariable(GlobalVariable golbalVariable) {
        this.globalVariables.add(golbalVariable);
    }

    public void addGlobalString(GlobalString globalString) {
        this.globalStrings.add(globalString);
    }

    public void addFunction(Function function) {
        this.functions.put(function.getName(), function);
    }

    public ArrayList<GlobalVariable> getGlobalVariables() {
        return globalVariables;
    }

    public void setGlobalVariables(ArrayList<GlobalVariable> globalVariables) {
        this.globalVariables = globalVariables;
    }

    public ArrayList<GlobalString> getGlobalStrings() {
        return globalStrings;
    }

    public void setGlobalStrings(ArrayList<GlobalString> globalStrings) {
        this.globalStrings = globalStrings;
    }

    public Map<String, Function> getFunctions() {
        return functions;
    }

    public void setFunctions(HashMap<String, Function> functions) {
        this.functions = functions;
    }

    public ArrayList<Function> getDefineFunc() {
        ArrayList<Function> functions = new ArrayList<>();
        for (Function function : MyModule.myModule.getFunctions().values()) {
            if (!inFunc.contains(function.getName())) {
                functions.add(function);
            }
        }
        return functions;
    }
}
