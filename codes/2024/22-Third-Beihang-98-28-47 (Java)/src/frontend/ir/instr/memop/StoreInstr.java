package frontend.ir.instr.memop;

import frontend.ir.Value;
import frontend.ir.constvalue.ConstValue;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;
import frontend.ir.symbols.Symbol;

import java.util.ArrayList;

public class StoreInstr extends MemoryOperation {
    private Value value;
    private Value ptr;
    
    public StoreInstr(Value value, Symbol symbol) {
        super(symbol);
        this.value = value;
        this.ptr = symbol.getAllocValue();
        setUse(value);
        setUse(ptr);
    }
    
    public StoreInstr(Value value, Symbol symbol, Value ptr) {
        super(symbol);
        this.value = value;
        this.ptr = ptr;
        setUse(value);
        setUse(ptr);
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new StoreInstr(this.value, this.symbol, this.ptr);
    }

    //store.getValue = value in storeInstr
    @Override
    public Number getNumber() {
        return value.getNumber();
    }

    public Value getValue() {
        return value;
    }
    
    @Override
    public String print() {
        String typeName;
        if (ptr != null && ptr.getPointerLevel() == 1) {
            typeName = symbol.getType().toString();
        } else {
            typeName = printBaseType();
        }
        return "store " + typeName + " " + value.value2string() + ", " + typeName + "* " + ptr.value2string();
    }
    
    @Override
    public String toString() {
        return "Store " + value.value2string() + " to " + ptr.value2string();
    }

    @Override
    public void modifyValue(Value from, Value to) {
        if (value == from) {
            value = to;
        } else if (ptr == from) {
            ptr = to;
        } else {
            throw new RuntimeException();
        }
    }
    
    public Value getPtr() {
        return ptr;
    }
    
    public boolean mayBeLoaded(ArrayList<BasicBlock> blks) {
        for (BasicBlock blk : blks) {
            Instruction ins = (Instruction) blk.getInstructions().getHead();
            while (ins != null) {
                if (ins instanceof LoadInstr) {
                    if (this.symbol.isArray()) {
                        if (!((LoadInstr) ins).getSymbol().isArray()) {
                            ins = (Instruction) ins.getNext();
                            continue;
                        }
                        Value yourPtr = ((LoadInstr) ins).getPtr();
                        Value myPtr   = this.ptr;
                        if (checkMayBeSameGEP(yourPtr, myPtr)) {
                            return true;
                        }
                    } else if (this.symbol.isGlobal()) {
                        return true;
                    } else {
                        throw new RuntimeException("这里应该不会对非数组、非全局的对象进行内存操作");
                    }
                } else {
                    if (ins instanceof CallInstr) {
                        if ((this.symbol.isGlobal() || this.symbol.isArray()) &&
                                !((CallInstr) ins).checkNoSideEffect()) {
                            return true;
                        }
                    }
                }
                ins = (Instruction) ins.getNext();
            }
        }
        
        return false;
    }
    
    @Override
    public String myHash() {
        return Integer.toString(this.hashCode());
    }
}
