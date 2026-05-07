package IR.IRInstruction;

import java.util.ArrayList;
import java.util.List;

import IR.IRType.IRArrayType;
import IR.IRType.IRPointerType;
import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;
import IR.IRType.IRType;

import static IR.IRType.IRInt32Type.IRInt32Type;

public class GetElementPointerInstruction extends IRInstruction {
    /*
     * 用于计算数组元素的地址
     */
    private IRValueRef base;//数组的基本地址
    private ArrayList<IRValueRef> index;

    public GetElementPointerInstruction(List<IRValueRef> operands, IRBaseBlockRef basicBlock) {
        super(operands, basicBlock);
        this.base = operands.get(1);
        this.index = new ArrayList<>();
        for(int i = 2; i < operands.size();i++){
            this.index.add(operands.get(i));
        }
    }

    public IRValueRef getBase() {
        return base;
    }

    public void setBase(IRValueRef irValueRef){this.base = irValueRef;}

    public IRValueRef getPointer() {
        return getOperands().get(1);
    }

    public ArrayList<IRValueRef> getIndex() {
        return index;
    }

    public void setIndex(int i, IRValueRef newValue) {
        index.set(i, newValue);
    }

    public IRType getPointedType(IRValueRef pointer, int depth) {
        IRType pointedType = null;
        if (pointer.getType() instanceof IRPointerType) {
            pointedType = ((IRPointerType) pointer.getType()).getBaseType();
            for (int i = 1; i < depth; i++) {
                if (pointedType instanceof IRArrayType) {
                    pointedType = ((IRArrayType) pointedType).getBaseType();
                }
            }
        } else if (pointer.getType() instanceof IRArrayType) {
            pointedType = ((IRArrayType) pointer.getType()).getBaseType();
            for (int i = 1; i < depth; i++) {
                if (pointedType instanceof IRArrayType) {
                    pointedType = ((IRArrayType) pointedType).getBaseType();
                }
            }
        }
        return pointedType;
    }

    @Override
    public String toString() {
        IRValueRef resRegister = getOperands().get(0);
        IRType baseType = ((IRPointerType) base.getType()).getBaseType();
        IRType resType;
        if (baseType instanceof IRArrayType) {
            resType = ((IRArrayType) baseType).getBaseType();
        } else {
            resType = baseType;
        }
        StringBuilder indexStrBuilder = new StringBuilder();
        for (IRValueRef index : index) {
            indexStrBuilder.append(", ").append(IRInt32Type().getText())
                    .append(" ").append(index.getText());
        }
        return resRegister.getText() + " = getelementptr " + baseType.getText() + ", "
                + base.getType().getText() + " " + base.getText() + indexStrBuilder.toString();
    }
}
