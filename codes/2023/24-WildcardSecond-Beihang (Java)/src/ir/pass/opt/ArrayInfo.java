package ir.pass.opt;

import ir.Value;
import ir.instr.GetElementPtrInst;
import ir.instr.Instr;
import ir.instr.Store;
import ir.value.BasicBlock;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;

public class ArrayInfo {
    public ArrayList<Instr> mems = new ArrayList<Instr>();
    public LinkedHashMap<GetElementPtrInst, Store> memout =
        new LinkedHashMap<GetElementPtrInst, Store>();
    public LinkedHashMap<GetElementPtrInst, LinkedHashSet<Store>> memin = new LinkedHashMap<>();
    public LinkedHashMap<Value, LinkedHashSet<BasicBlock>> memFuncsin =
        new LinkedHashMap<>();
    public LinkedHashSet<Value> memFuncsout = new LinkedHashSet<>();
}
