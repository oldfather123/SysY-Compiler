package midend.pseudoinstr;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.objecttype.derived.TVector;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class VecLoadInstr extends Instruction {
    private GEPInstr basicPointer;

    public VecLoadInstr(Integer regIndex, GEPInstr basicPointer, BasicBlock parentBB) {
        super(new TVector(((TPointer) basicPointer.getType()).getReferencedType()), regIndex, parentBB);

        this.basicPointer = basicPointer;

        setUse(basicPointer);
    }

    public GEPInstr getBasicPointer() {
        return basicPointer;
    }

    @Override
    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
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
        if (this.basicPointer == from) {
            this.basicPointer = (GEPInstr) to;
        }
    }

    @Override
    public String toString() {
        return "VEC_LOAD " + this.getType().printIRType() + " " + this.value2string() + ", "
                + this.basicPointer.getType().printIRType() + " " + this.basicPointer.value2string();
    }
}
