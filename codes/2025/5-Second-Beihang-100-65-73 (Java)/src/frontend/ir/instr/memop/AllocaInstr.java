package frontend.ir.instr.memop;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class AllocaInstr extends MemoryOperation {
    private final Type referencedType;

    public AllocaInstr(Type referencedType, int regIdx, BasicBlock parentBB) {
        super(new TPointer(referencedType), regIdx, parentBB);
        this.referencedType = referencedType;
    }

    public Type getReferencedType() {
        return referencedType;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        throw new RuntimeException("no value to modify");
    }

    @Override
    public String toString() {
        return this.value2string() + " = alloca " + referencedType.printIRType();
    }

    @Override
    public String myHash() {
        return Integer.toString(this.hashCode());
    }

    @Override
    public Value simplify() {
        return this;
    }

    public HashSet<Value> getOperands() {
        return new HashSet<>();
    }

    public static class ArrayAlloca extends AllocaInstr {
        private final Value arrayInitVal;
        private Instruction initBeginIns;
        private Instruction initEndIns;

        public ArrayAlloca(Type referencedType, int regIdx, BasicBlock parentBB, Value arrayInitVal) {
            super(referencedType, regIdx, parentBB);
            this.arrayInitVal = arrayInitVal;
        }

        public Value getArrayInitVal() {
            return arrayInitVal;
        }

        public Instruction getInitBeginIns() {
            return initBeginIns;
        }

        public Instruction getInitEndIns() {
            return initEndIns;
        }

        public void setInitBeginIns(Instruction initBeginIns) {
            this.initBeginIns = initBeginIns;
        }

        public void setInitEndIns(Instruction initEndIns) {
            this.initEndIns = initEndIns;
        }
    }
}
