package frontend.ir.instr.memop;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.symbols.Symbol;

public abstract class MemoryOperation extends Instruction {
    protected final Symbol symbol;
    
    public MemoryOperation(Symbol symbol) {
        if (symbol == null) {
            throw new NullPointerException();
        }
        this.symbol = symbol;
    }
    
    @Override
    public DataType getDataType() {
        return symbol.getType();
    }
    
    public Symbol getSymbol() {
        return symbol;
    }
    
    public String printBaseType() {
        if (symbol.isArray()) {
            return symbol.printArrayTypeName();
        } else {
            return symbol.getType().toString();
        }
    }
    
    protected boolean checkMayBeSameGEP(Value yourPtr, Value myPtr) {
        if (!(yourPtr instanceof GEPInstr) || !(myPtr instanceof GEPInstr)) {
            throw new RuntimeException("对数组的存取应该都需要通过GEP");
        }
        if (((GEPInstr) yourPtr).myHash().equals(((GEPInstr) myPtr).myHash())) {
            return true;
        }
        if (((GEPInstr) yourPtr).hasNonConstIndex() || ((GEPInstr) myPtr).hasNonConstIndex()) {
            // todo: 这里比较保守，一旦二者之间有一个的目的地址不确定，我们就认为它们可能在存取相同地址
            return true;
        }
        return false;
    }
    
    @Override
    public Value operationSimplify() {
        return null;
    }
}
