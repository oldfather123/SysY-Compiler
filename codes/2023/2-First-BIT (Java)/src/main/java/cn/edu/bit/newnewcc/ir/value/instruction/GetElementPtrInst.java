package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.IndexOutOfBoundsException;
import cn.edu.bit.newnewcc.ir.type.ArrayType;
import cn.edu.bit.newnewcc.ir.type.PointerType;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.ArrayList;
import java.util.List;

/**
 * 取下标指令
 *
 * @see <a href="https://llvm.org/docs/LangRef.html#getelementptr-instruction">LLVM IR文档</a>
 */
public class GetElementPtrInst extends Instruction {
    /**
     * 基地址操作数
     */
    private final Operand rootOperand;

    /**
     * 下标操作数列表
     * <p>
     * 注意其与直观意义上的下标不同，例如 a[1] 会产生两个下标 [0,1]
     * <p>
     * 具体定义详见 LLVM IR 文档
     */
    private final List<Operand> indexOperands;

    /**
     * @param root    基地址操作数
     * @param indices 下标操作数列表。这与直观意义上的下标不同，请务必阅读 LLVM IR 文档。
     */
    public GetElementPtrInst(Value root, List<Value> indices) {
        super(inferDereferencedType(root.getType(), indices.size()));
        this.rootOperand = new Operand(this, root.getType(), root);
        this.indexOperands = new ArrayList<>();
        for (var index : indices) {
            indexOperands.add(new Operand(this, index.getType(), index));
        }
    }

    /**
     * @return 基地址操作数
     */
    public Value getRootOperand() {
        return rootOperand.getValue();
    }

    /**
     * @param value 基地址操作数
     */
    public void setRootOperand(Value value) {
        rootOperand.setValue(value);
    }

    /**
     * 获取下标操作数列表 <br>
     * 此方法仅供不关心第几个下标时使用 <br>
     *
     * @return 下标操作数列表
     */
    public List<Value> getIndexOperands() {
        var list = new ArrayList<Value>();
        for (Operand indexOperand : indexOperands) {
            list.add(indexOperand.getValue());
        }
        return list;
    }

    /**
     * 获取某个下标的值
     *
     * @param index 下标
     * @return 值
     */
    public Value getIndexAt(int index) {
        return indexOperands.get(index).getValue();
    }

    /**
     * 设置某个下标的值
     *
     * @param index 下标
     * @param value 值
     */
    public void setIndexAt(int index, Value value) {
        if (index < 0 || index >= indexOperands.size()) {
            throw new IndexOutOfBoundsException(index, 0, indexOperands.size());
        }
        indexOperands.get(index).setValue(value);
    }

    /**
     * 获取下标列表的大小
     *
     * @return 下标列表的大小
     */
    public int getIndicesSize() {
        return indexOperands.size();
    }

    @Override
    public List<Operand> getOperandList() {
        var list = new ArrayList<Operand>();
        list.add(rootOperand);
        list.addAll(indexOperands);
        return list;
    }

    @Override
    public void emitIr(StringBuilder builder) {
        builder.append(String.format(
                "%s = getelementptr %s, %s %s",
                getValueNameIR(),
                ((PointerType) rootOperand.getType()).getBaseType().getTypeName(),
                rootOperand.getType().getTypeName(),
                getRootOperand().getValueNameIR()
        ));
        for (var indexOperand : indexOperands) {
            var index = indexOperand.getValue();
            builder.append(String.format(
                    ", %s %s",
                    index.getTypeName(),
                    index.getValueNameIR()
            ));
        }
    }

    /**
     * 分析解引用后的类型
     *
     * @param rootType         根类型
     * @param dereferenceCount 解引用的次数
     * @return 解引用后的类型
     */
    public static Type inferDereferencedType(Type rootType, int dereferenceCount) {
        rootType = ((PointerType) rootType).getBaseType();
        for (var i = 1; i < dereferenceCount; i++) {
            rootType = ((ArrayType) rootType).getBaseType();
        }
        return PointerType.getInstance(rootType);
    }
}
