package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.VoidType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * 返回语句
 */
public class ReturnInst extends TerminateInst {
    private final Operand returnValueOperand;

    /**
     * 构造一个未填入返回值的返回语句
     * <p>
     * 若要构造返回void的语句，请使用new ReturnInst(VoidValue.getInstance())
     *
     * @param returnType 返回值类型
     */
    public ReturnInst(Type returnType) {
        this.returnValueOperand = new Operand(this, returnType, null);
    }

    /**
     * 构造一个返回语句
     * <p>
     * 若要构造返回void的语句，请传入VoidValue.getInstance()
     *
     * @param returnValue 返回值
     */
    public ReturnInst(Value returnValue) {
        this.returnValueOperand = new Operand(this, returnValue.getType(), returnValue);
    }

    public Type getReturnValueType() {
        return returnValueOperand.getType();
    }

    public Value getReturnValue() {
        return returnValueOperand.getValue();
    }

    public void setReturnValue(Value returnValue) {
        this.returnValueOperand.setValue(returnValue);
    }

    @Override
    public void emitIr(StringBuilder builder) {
        var returnValue = returnValueOperand.getValue();
        if (returnValue.getType() instanceof VoidType) {
            builder.append("ret void");
        } else {
            builder.append(String.format(
                    "ret %s %s",
                    returnValue.getTypeName(),
                    returnValue.getValueNameIR()
            ));
        }
    }

    @Override
    public List<Operand> getOperandList() {
        var list = new ArrayList<Operand>();
        list.add(returnValueOperand);
        return list;
    }

    @Override
    public Collection<BasicBlock> getExits() {
        return new ArrayList<>();
    }
}
