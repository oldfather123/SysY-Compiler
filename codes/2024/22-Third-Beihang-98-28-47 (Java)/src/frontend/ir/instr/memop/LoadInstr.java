package frontend.ir.instr.memop;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;
import frontend.ir.symbols.Symbol;

import java.util.ArrayList;

public class LoadInstr extends MemoryOperation {
    private final int result;
    private Value ptr;
    
    public LoadInstr(int result, Symbol symbol) {
        super(symbol);
        this.result = result;
        this.ptr = symbol.getAllocValue();
        setUse(symbol.getAllocValue());
        this.pointerLevel = symbol.getAllocValue().getPointerLevel() - 1;
    }
    
    public LoadInstr(int result, Symbol symbol, Value ptr) {
        super(symbol);
        this.result = result;
        this.ptr = ptr;
        setUse(ptr);
        this.pointerLevel = ptr.getPointerLevel() - 1;
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new LoadInstr(procedure.getAndAddRegIndex(), this.symbol, ptr);
    }
    
    @Override
    public Number getNumber() {
        return result;
    }
    
    @Override
    public String print() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("%reg_").append(result).append(" = load ");
        String ty;
        if (ptr != null && ptr.getPointerLevel() == 1) {
            ty = symbol.getType().toString();
        } else {
            ty = printBaseType();
        }
        stringBuilder.append(ty).append(", ").append(ty).append("* ");
        stringBuilder.append(ptr.value2string());
        return stringBuilder.toString();
    }
    
    @Override
    public String type2string() {
        if (this.pointerLevel > 0) {
            return this.printBaseType();
        } else {
            return this.getDataType().toString();
        }
    }

    @Override
    public void modifyValue(Value from, Value to) {
        if (ptr == from) {
            ptr = to;
        } else {
            throw new RuntimeException();
        }
    }
    
    public Value getPtr() {
        return ptr;
    }
    
    public boolean mayBeStored(ArrayList<BasicBlock> blks) {
        for (BasicBlock blk : blks) {
            Instruction ins = (Instruction) blk.getInstructions().getHead();
            while (ins != null) {
                if (ins instanceof StoreInstr) {
                    if (this.symbol.isArray()) {
                        if (!((StoreInstr) ins).getSymbol().isArray()) {
                            ins = (Instruction) ins.getNext();
                            continue;
                        }
                        Value yourPtr = ((StoreInstr) ins).getPtr();
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
        return this.value2string();
    }
}
