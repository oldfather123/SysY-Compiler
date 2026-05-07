package riscv.value;

import riscv.util.GlobalAllocTable;
import util.nodelist.NodeList;

import java.util.LinkedHashMap;
import java.util.Map;

public class AsmModule {
    public static final AsmModule module = new AsmModule();

    public final NodeList<AsmInstr> globalInstrs = new NodeList<>();
    public final GlobalAllocTable globalVarAllocTable = new GlobalAllocTable();

    public final Map<String, AsmFunction> funcMap = new LinkedHashMap<>();

    public boolean hasMemset = false; // 是否生成内置函数 memset，见 value/MyMemset.java
}
