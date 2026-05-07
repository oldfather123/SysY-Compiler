package utils;

import ir.instr.LoadInstr;
import ir.value.Value;
import pass.ir.AliasAnalysis;

import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

public class AliasContainer {
    // 存储基本指针
    public HashSet<String> basicPointers = new HashSet<>();
    // 存储每个function里的间接指针
    public HashMap<String,HashSet<AliasAnalysis.IndirectPointer>> indirectPointers = new HashMap<>();
    // 基本指针名字到Value的映射
    public HashMap<String, Value> basicPointerMap = new HashMap<>();
    // 间接指针到Value的映射
    public HashMap<AliasAnalysis.IndirectPointer,Value> indirectPointerMap = new HashMap<>();
    // 间接指针到基本指针的映射
    public HashMap<AliasAnalysis.IndirectPointer,String> indirectToBasic = new HashMap<>();
    // 别名映射
    public HashMap<Pair<AliasAnalysis.IndirectPointer,String>,Pair<AliasAnalysis.IndirectPointer,String>> aliasMap = new HashMap<>();
    // 名字到间接指针的映射
    public HashMap<String, AliasAnalysis.IndirectPointer> nameToIndirect = new HashMap<>();
    // 全局基本指针
    public HashSet<String> globalBasicPointers = new HashSet<>();
    // 函数改变的全局变量
    public HashMap<String,HashSet<String>> functionChangedGlobal = new HashMap<>();
    public boolean ifReady = false;
    // 单例模式
    private AliasContainer() {

    }
    private static AliasContainer instance = new AliasContainer();
    public static AliasContainer getInstance() {
        return instance;
    }

    public void initialize(HashSet<String> basicPointers, HashMap<String,HashSet<AliasAnalysis.IndirectPointer>> indirectPointers,
                      HashMap<String, Value> basicPointerMap, HashMap<AliasAnalysis.IndirectPointer,Value> indirectPointerMap,
                      HashMap<AliasAnalysis.IndirectPointer,String> indirectToBasic, HashMap<Pair<AliasAnalysis.IndirectPointer,String>,Pair<AliasAnalysis.IndirectPointer,String>> aliasMap,
                      HashMap<String, AliasAnalysis.IndirectPointer> nameToIndirect, HashSet<String> globalBasicPointers, HashMap<String,HashSet<String>> functionChangedGlobal) {
        ifReady = true;
        this.basicPointers = basicPointers;
        this.indirectPointers = indirectPointers;
        this.basicPointerMap = basicPointerMap;
        this.indirectPointerMap = indirectPointerMap;
        this.indirectToBasic = indirectToBasic;
        this.aliasMap = aliasMap;
        this.nameToIndirect = nameToIndirect;
        this.globalBasicPointers = globalBasicPointers;
        this.functionChangedGlobal = functionChangedGlobal;
    }

    public HashSet<String> externalFunctions = new HashSet<>(List.of(new String[]{
            "getint",
            "getch",
            "getarray",
            "getfloat",
            "getfarray",
            "putint",
            "putch",
            "putfloat",
            "putarray",
            "putfarray",
            "starttime",
            "stoptime",
    }));

    public String getPointerName(String name) {
        if (nameToIndirect.containsKey(name)) {
            return indirectToBasic.get(nameToIndirect.get(name));
        }
        else {
            return name;
        }
    }
}
