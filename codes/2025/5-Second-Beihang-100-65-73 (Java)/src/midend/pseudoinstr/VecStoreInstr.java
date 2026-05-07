package midend.pseudoinstr;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class VecStoreInstr extends Instruction {
    private Value valueToStore;
    private GEPInstr basicPointer;

    public VecStoreInstr(Value valueToStore, GEPInstr basicPointer, BasicBlock parentBB) {
        super(null, null, parentBB);

        this.valueToStore = valueToStore;
        this.basicPointer = basicPointer;

        this.setUse(valueToStore);
        this.setUse(basicPointer);
    }

    public GEPInstr getBasicPointer() {
        return basicPointer;
    }

    public Value getValueToStore() {
        return valueToStore;
    }

    @Override
    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(valueToStore);
        operands.add(basicPointer);
        return operands;
    }

    @Override
    public String myHash() {
        return String.valueOf(this.hashCode());
    }

    @Override
    public Value simplify() {
        return null;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (this.valueToStore == from) {
            this.valueToStore = to;
        }
        if (this.basicPointer == from) {
            this.basicPointer = (GEPInstr) to;
        }
    }

    @Override
    public String toString() {
        return "VEC_STORE " + ((TPointer) this.basicPointer.getType()).getReferencedType().printIRType() + " "
                + this.valueToStore.value2string() + ", "
                + this.basicPointer.getType().printIRType() + " " + this.basicPointer.value2string();
    }
}
