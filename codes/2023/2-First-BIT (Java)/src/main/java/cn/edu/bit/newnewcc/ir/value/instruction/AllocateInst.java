package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.type.PointerType;

import java.util.ArrayList;
import java.util.List;

/**
 * 栈空间分配指令
 */
public class AllocateInst extends MemoryInst {
    /**
     * @param allocatedType 待分配内存的变量的类型
     */
    public AllocateInst(Type allocatedType) {
        super(PointerType.getInstance(allocatedType));
    }

    /**
     * @return 待分配内存的变量的类型
     */
    public Type getAllocatedType() {
        return getType().getBaseType();
    }

    /**
     * 获取此语句返回值的类型
     * <p>
     * 注意：该方法返回的是语句返回值的类型，而不是被分配变量的类型
     * <p>
     * 要得到被分配变量的类型，请使用 getAllocatedType
     *
     * @return 此语句返回值的类型
     */
    @Override
    public PointerType getType() {
        return (PointerType) super.getType();
    }

    @Override
    public List<Operand> getOperandList() {
        return new ArrayList<>();
    }

    @Override
    public void emitIr(StringBuilder builder) {
        builder.append(String.format(
                "%s = alloca %s",
                this.getValueNameIR(),
                this.getType().getBaseType().getTypeName()
        ));
    }
}
