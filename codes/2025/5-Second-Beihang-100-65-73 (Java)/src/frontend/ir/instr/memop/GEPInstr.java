package frontend.ir.instr.memop;

import frontend.ir.Value;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.derived.TArray;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

public class GEPInstr extends MemoryOperation {
    private final List<Value> indexList;
    private Value ptrVal;    // 指针基质

    public GEPInstr(int regIdx, List<Value> indexList, Value ptrVal, BasicBlock parentBB) {
        super(getMyType(indexList.size(), ptrVal.getType()), regIdx, parentBB);
        this.indexList = new ArrayList<>(indexList);
        this.ptrVal = ptrVal;
        if (parentBB.isNotClosed()) {
            this.setUse(ptrVal);
            for (Value index : indexList) {
                this.setUse(index);
            }
        }
    }

    private static Type getMyType(int indexNum, Type baseType) {
        if (!(baseType instanceof TPointer basePtrType)) {
            throw new RuntimeException("指针基址必须是指针类型");
        }
        if (indexNum == 1) {
            return basePtrType;
        } else {
            Type referenced = basePtrType.getReferencedType();
            for (int i = 1; i < indexNum; i++) {
                if (!(referenced instanceof TArray arrayType)) {
                    throw new RuntimeException("GEP 目标应该是数组");
                }
                referenced = arrayType.getElementType();
            }
            return new TPointer(referenced);
        }
    }

    public List<Value> getIndexList() {
        return indexList;
    }

    public Value getIndex() {
        return indexList.getLast();
    }

    public Value getPtrVal() {
        return ptrVal;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (ptrVal == from) {
            ptrVal = to;
        }
        for (int i = 0; i < indexList.size(); i++) {
            if (indexList.get(i) == from) {
                indexList.set(i, to);
            }
        }
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(this.value2string()).append(" = getelementptr ");

        Type ptrValType = ptrVal.getType();
        if (!(ptrValType instanceof TPointer ptr)) {
            throw new RuntimeException("指针的类型必须是指针类型");
        }
        Type referencedType = ptr.getReferencedType();
        sb.append(referencedType.printIRType()).append(", ");
        sb.append(referencedType.printIRType()).append("* ").append(ptrVal.value2string());

        for (Value index : indexList) {
            sb.append(", ").append(index.getType().printIRType()).append(" ").append(index.value2string());
        }

        return sb.toString();
    }

    @Override
    public String myHash() {
        StringBuilder sb = new StringBuilder("GEP ");
        sb.append(this.ptrVal.value2string());
        for (Value index : indexList) {
            sb.append(" ").append(index.value2string());
        }
        return sb.toString();
    }

    @Override
    public Value simplify() {
        if (isRemoved) {
            return this;
        }

        if (this.ptrVal instanceof GEPInstr gep) {
            AddInstr newPos = null;
            List<Value> listIndexNew = new ArrayList<>(gep.getIndexList());
            if (ptrVal.getType() instanceof TPointer) {
                listIndexNew.removeLast();
                newPos = new AddInstr(this.getParentBB().getParentFunc().getAndAddRegIdx(),
                        gep.getIndexList().getLast(), this.indexList.getFirst(), this.getParentBB());
                newPos.setUse(gep.getIndexList().getLast());
                newPos.setUse(this.indexList.getFirst());
                newPos.insertBefore(gep);
                listIndexNew.add(newPos);
                listIndexNew.addAll(this.indexList.subList(1, this.indexList.size()));
            }
            GEPInstr newGep = new GEPInstr(this.getRegIndex(), listIndexNew, gep.getPtrVal(), this.getParentBB());
            newGep.setUse(gep.getPtrVal());
            for (Value index : listIndexNew) {
                newGep.setUse(index);
            }
            newGep.insertAfter(this);
            this.replaceUseWith(newGep);
            this.removeFromList();
            if (newPos != null) {
                newPos.simplify();
            }
            return newGep;
        }
        return this;
    }

    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(ptrVal);
        operands.addAll(indexList);
        return operands;
    }
}
