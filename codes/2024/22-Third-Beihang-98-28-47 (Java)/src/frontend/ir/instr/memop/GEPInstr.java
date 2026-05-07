package frontend.ir.instr.memop;

import backend.itemStructure.Group;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.constvalue.ConstValue;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.structure.FParam;
import frontend.ir.structure.Function;
import frontend.ir.structure.GlobalObject;
import frontend.ir.structure.Procedure;
import frontend.ir.symbols.Symbol;

import java.util.ArrayList;
import java.util.List;

/**
 * getelementptr
 * 获取指针的指令，主要用于数组操作
 * 目前的设计是无论几层都一个指令取出来，之后可能需要改成分不同情况分开或合并
 */
public class GEPInstr extends MemoryOperation {
    private final int result;
    private final List<Value> indexList;
    private final String arrayTypeName;
    private Value ptrVal;    // 指针基质：全局变量名，或者局部变量申请指令，或上一条 GEP
    private final int type;
    
    public GEPInstr(int result, List<Value> indexList, Symbol symbol) {
        super(symbol);
        type = 1;
        if (indexList == null) {
            throw new NullPointerException();
        }
        this.result = result;
        this.indexList = indexList;
        this.pointerLevel = symbol.getDim() + 1 - indexList.size();
        this.arrayTypeName = symbol.printArrayTypeName();
        this.ptrVal = symbol.getAllocValue();
        setUse(symbol.getAllocValue());
        for (Value value : indexList) {
            setUse(value);
        }
    }
    
    private GEPInstr(int result, List<Value> indexList, Value allocValue, Symbol symbol) {
        super(symbol);
        type = 1;
        if (indexList == null) {
            throw new NullPointerException();
        }
        this.result = result;
        this.indexList = indexList;
        this.pointerLevel = symbol.getDim() + 1 - indexList.size();
        this.arrayTypeName = symbol.printArrayTypeName();
        this.ptrVal = allocValue;
        setUse(allocValue);
        for (Value value : indexList) {
            setUse(value);
        }
    }
    
    public GEPInstr(int result, Value base, Symbol symbol, String arrayTypeName) {
        super(symbol);
        type = 2;
        this.result = result;
        this.indexList = new ArrayList<>();
        indexList.add(new ConstInt(0));
        this.pointerLevel = base.getPointerLevel() - 1;
        this.arrayTypeName = arrayTypeName;
        this.ptrVal = base;
        setUse(base);
    }
    
    public GEPInstr(int result, Value base, List<Value> indexList, Symbol symbol, String arrayTypeName) {
        super(symbol);
        type = 3;
        this.result = result;
        this.indexList = indexList;
        this.pointerLevel = symbol.getDim() + 1 - indexList.size();
        
        if (arrayTypeName.charAt(arrayTypeName.length() - 1) == '*') {
            this.arrayTypeName = arrayTypeName.substring(0, arrayTypeName.length() - 1);
        } else {
            this.arrayTypeName = arrayTypeName;
        }
        
        this.ptrVal = base;
        setUse(base);
        for (Value value : indexList) {
            setUse(value);
        }
    }
    
    public GEPInstr(int result, GEPInstr base, GEPInstr toMerge, AddInstr link, int beginIndex) {
        super(base.symbol);
        type = 3;
        this.result = result;
        this.indexList = new ArrayList<>(base.indexList);
        this.indexList.set(this.indexList.size() - 1, link);

        for (int i = beginIndex; i < toMerge.indexList.size(); i++) {
            this.indexList.add(toMerge.indexList.get(i));
        }
        this.pointerLevel = toMerge.pointerLevel;
        this.arrayTypeName = base.arrayTypeName;
        this.ptrVal = base.ptrVal;
        this.setUse(this.ptrVal);
        for (Value value : this.indexList) {
            this.setUse(value);
        }
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        if (type == 1) {
            return new GEPInstr(procedure.getAndAddRegIndex(), new ArrayList<>(indexList), ptrVal, symbol);
        } else if (type == 2) {
            return new GEPInstr(procedure.getAndAddRegIndex(), ptrVal, symbol, arrayTypeName);
        } else if (type == 3) {
            return new GEPInstr(procedure.getAndAddRegIndex(), ptrVal, new ArrayList<>(indexList), symbol, arrayTypeName);
        } else {
            throw new RuntimeException("type 已经穷尽了");
        }
    }
    
    @Override
    public String type2string() {
        return this.printBaseType() + "*";
    }
    
    @Override
    public String printBaseType() {
        if (this.pointerLevel > 1) {
            List<Integer> limList = this.symbol.getLimitList();
            StringBuilder stringBuilder = new StringBuilder();
            int lim = limList.size();
            int start = lim + 1 - pointerLevel;
            for (int i = start; i < lim; i++) {
                stringBuilder.append("[").append(limList.get(i)).append(" x ");
            }
            stringBuilder.append(symbol.getType());
            for (int i = start; i < lim; i++) {
                stringBuilder.append("]");
            }
            return stringBuilder.toString();
        } else {
            return this.symbol.getType().toString();
        }
    }
    
    public List<Integer> getSizeList() {
        List<Integer> sizeList = new ArrayList<>();
        sizeList.add(1);
        int size = symbol.getLimitList().size();
        for (int i = size - 1; i >= 0; i--) {
            sizeList.add(0, sizeList.get(0) * symbol.getLimitList().get(i));
        }
        sizeList.remove(0);
        return sizeList;
    }
    
    public Value getPtrVal() {
        return ptrVal;
    }
    
    @Override
    public Number getNumber() {
        return result;
    }
    
    @Override
    public String print() {
        return "%reg_" + result + " = getelementptr " +
                printTypeAndIndex();
    }
    
    private String printTypeAndIndex() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append(arrayTypeName).append(", ");
        stringBuilder.append(arrayTypeName).append("* ");
        stringBuilder.append(ptrVal.value2string());
        if (!symbol.isArrayFParam()) {
            stringBuilder.append(", i32 0");
        }
        for (Value index : indexList) {
            stringBuilder.append(", i32 ").append(index.value2string());
        }
        return stringBuilder.toString();
    }
    
    @Override
    public void modifyValue(Value from, Value to) {
        if (this.ptrVal == from) {
            this.ptrVal = to;
        } else {
            for (int i = 0; i < indexList.size(); i++) {
                if (indexList.get(i) == from) {
                    indexList.set(i, to);
                    return;
                }
            }
            throw new RuntimeException("没有可以置换的 value");
        }
    }
    
    public List<Value> getWholeIndexList() {
        List<Value> wholeIndexList = new ArrayList<>(indexList);
        for (int i = 0; i < symbol.getLimitList().size() - indexList.size(); i++) {
            wholeIndexList.add(new ConstInt(0));
        }
        return wholeIndexList;
    }
    
    public boolean hasNonConstIndex() {
        for (Value index : indexList) {
            if (!(index instanceof ConstValue)) {
                return true;
            }
        }
        
        if (ptrVal instanceof GEPInstr) {
            return ((GEPInstr) ptrVal).hasNonConstIndex();
        }
        
        return false;
    }
    
    public boolean isUseless() {
        return indexList.isEmpty();
    }
    
    /**
     * 用来寻找一个指针指向对象的定义，从而确定两个指针有没有可能是指向同一个对象
     * root 只能是 AllocaInstr，GlobalObject，FParam 三种
     */
    public Value getRoot() {
        if (this.ptrVal instanceof GEPInstr) {
            return ((GEPInstr) this.ptrVal).getRoot();
        }
        return this.ptrVal;
    }
    
    public Group<AddInstr, GEPInstr> tryMergePtr(Function function) {
        if (!(this.ptrVal instanceof GEPInstr)) {
            return null;
        }
        List<Value> baseIndexList = ((GEPInstr) this.ptrVal).indexList;
        Value baseTail = baseIndexList.get(baseIndexList.size() - 1);

        AddInstr link;
        int beginIndex; // 如果是传参的话首位 index 有效，需要和 base 的末位加起来，否则加的是默认的 0
        if (this.symbol.isArrayFParam()) {
            link = new AddInstr(function.getAndAddRegIndex(), baseTail, this.indexList.get(0));
            beginIndex = 1;
        } else {
            link = new AddInstr(function.getAndAddRegIndex(), baseTail, ConstInt.Zero);
            beginIndex = 0;
        }
        
        GEPInstr merged = new GEPInstr(function.getAndAddRegIndex(), (GEPInstr) this.ptrVal, this, link, beginIndex);
        return new Group<>(link, merged);
    }
    
    @Override
    public String myHash() {
        return this.printTypeAndIndex();
    }
}
