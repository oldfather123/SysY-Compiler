package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

public class GetElementPtr extends Instruction {

    //全部视作一维数组
    public GetElementPtr(Value ptr, Value elemIndex) {
        super(IRManager.getInstance().declareTempVar(), ptr.getRefType().getPointerType());
        use(ptr);
        use(elemIndex);
    }

    public GetElementPtr(Value ptr, int elemIndex) {
        super(IRManager.getInstance().declareTempVar(), ptr.getRefType().getPointerType());
        use(ptr);
        use(new ConstNumber(elemIndex));
    }

    public String toString() {
        Value ptr = operandList.get(0);
        Value elemIndex = operandList.get(1);
            /*
                想要从数组中取出一个数，则需要进行两次offset；而指针只偏移一次
                拿到的数据类型是type*，即要取到的数的指针
             */
        if (ptr.getType().equals(ValueType.ARRAY)) {
            return reg + " = getelementptr " + ptr.getTypeString() + ", " + ptr.getTypeString() + "* " +
                    ptr.getReg() + ",i32 0, i32 " + elemIndex.getReg() + "\n";
        } else {
            return reg + " = getelementptr " + ptr.getRefType() + ", " + ptr.getTypeString() + " " +
                    ptr.getReg() + ",i32 " + elemIndex.getReg() + "\n";
        }
    }

    public Value getPtr() {
        return operandList.get(0);
    }

    public Value getElemIndex() {
        return operandList.get(1);
    }

    public Number getStorageVal() {
        return ((GlobalDecl) getPtr()).getConstValAtIndex(((ConstNumber) getElemIndex()).getVal().intValue());
    }

    public boolean canGetConstNumber() {
        Value ptr = getPtr();
        return ptr instanceof GlobalDecl globalDecl && globalDecl.isConst() && getElemIndex() instanceof ConstNumber;
    }


}
