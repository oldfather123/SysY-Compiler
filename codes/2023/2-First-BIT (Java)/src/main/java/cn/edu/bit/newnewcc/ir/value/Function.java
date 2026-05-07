package cn.edu.bit.newnewcc.ir.value;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.type.FunctionType;

import java.util.*;

/**
 * 函数
 */
public class Function extends BaseFunction {

    public static class FormalParameter extends Value {
        private FormalParameter(Type type, Function function) {
            super(type);
            this.function = function;
        }

        private String valueName;
        private final Function function;

        public Function getFunction() {
            return function;
        }

        @Override
        public String getValueName() {
            return valueName;
        }

        @Override
        public String getValueNameIR() {
            return '%' + getValueName();
        }

        @Override
        public void setValueName(String valueName) {
            this.valueName = valueName;
        }
    }

    private final List<FormalParameter> formalParameters;
    // LinkedHashSet能够保证迭代顺序和插入顺序一致
    private final LinkedHashSet<BasicBlock> basicBlocks;
    private final BasicBlock entryBasicBlock;

    /**
     * 创建一个函数
     * <p>
     * 新建的函数默认携带一个入口块，且该入口块是与函数绑定的，不可解绑。
     *
     * @param functionType 函数类型
     */
    public Function(FunctionType functionType) {
        super(functionType);
        this.formalParameters = new ArrayList<>();
        for (Type parameterType : functionType.getParameterTypes()) {
            this.formalParameters.add(new FormalParameter(parameterType, this));
        }
        this.basicBlocks = new LinkedHashSet<>();
        this.entryBasicBlock = new BasicBlock();
        this.addBasicBlock_(this.entryBasicBlock, true);
    }

    private String functionName = null;

    @Override
    public String getValueName() {
        return functionName;
    }

    @Override
    public String getValueNameIR() {
        return '@' + functionName;
    }

    @Override
    public void setValueName(String valueName) {
        functionName = valueName;
    }

    /**
     * 将基本块添加到函数中
     *
     * @param basicBlock 待添加的基本块
     */
    public void addBasicBlock(BasicBlock basicBlock) {
        this.addBasicBlock_(basicBlock, false);
    }

    /**
     * 添加基本块
     * <p>
     * 锁定功能只允许Function类使用，用于锁定函数入口块
     *
     * @param basicBlock        待添加的基本块
     * @param shouldFixFunction 是否锁定该基本块
     */
    private void addBasicBlock_(BasicBlock basicBlock, boolean shouldFixFunction) {
        basicBlock.__setFunction__(this, shouldFixFunction);
        this.basicBlocks.add(basicBlock);
    }


    /**
     * 将基本块从函数中移除
     *
     * @param basicBlock 待移除的基本块
     */
    public void removeBasicBlock(BasicBlock basicBlock) {
        if (basicBlock.getFunction() != this) {
            throw new IllegalArgumentException("Specified basic block does not belong to this function");
        }
        basicBlock.__clearFunction__();
        this.basicBlocks.remove(basicBlock);
    }

    public BasicBlock getEntryBasicBlock() {
        return entryBasicBlock;
    }

    /**
     * 获取函数内的所有基本块
     * <p>
     * 保证基本块按插入顺序排列
     *
     * @return 函数基本块列表（只读）
     */
    public List<BasicBlock> getBasicBlocks() {
        return List.copyOf(basicBlocks);
    }

    /**
     * @return 形参列表（只读）
     */
    public List<FormalParameter> getFormalParameters() {
        return Collections.unmodifiableList(formalParameters);
    }

    public void emitIr(StringBuilder builder) {
        builder.append(String.format(
                "define dso_local %s %s",
                this.getReturnType().getTypeName(),
                this.getValueNameIR()
        ));
        builder.append('(');
        StringJoiner joiner = new StringJoiner(", ");
        for (Value formalParameter : this.getFormalParameters()) {
            joiner.add(formalParameter.getTypeName() + " " + formalParameter.getValueNameIR());
        }
        builder.append(joiner);
        builder.append(") {\n");
        for (BasicBlock basicBlock : this.getBasicBlocks()) {
            basicBlock.emitIr(builder);
        }
        builder.append("}\n\n");
    }

}
