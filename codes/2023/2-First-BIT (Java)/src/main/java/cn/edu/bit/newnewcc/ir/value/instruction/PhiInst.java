package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.type.LabelType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.*;
import java.util.function.BiConsumer;

/**
 * Phi指令
 * <p>
 * 用于在存在分支的情况下实现SSA
 * <p>
 * 该指令必须放置于基本块的开头
 *
 * @see <a href="https://llvm.org/docs/LangRef.html#phi-instruction">LLVM IR文档</a>
 */
public class PhiInst extends Instruction {

    /**
     * 基本块操作数
     * <p>
     * 自动同步修改到 PhiInst::basicBlockOperandMap
     */
    private class BasicBlockOperand extends Operand {
        public BasicBlockOperand(Value value) {
            super(PhiInst.this, LabelType.getInstance(), value);
        }

        @Override
        public BasicBlock getValue() {
            return (BasicBlock) super.getValue();
        }

        @Override
        public void setValue(Value value) {
            if (this.value != null) {
                basicBlockOperandMap.remove((BasicBlock) this.value);
            }
            super.setValue(value);
            if (this.value != null) {
                basicBlockOperandMap.put((BasicBlock) this.value, this);
            }
        }

    }

    /**
     * @param type 指令返回值的类型
     */
    public PhiInst(Type type) {
        super(type);
    }

    private final Map<BasicBlock, BasicBlockOperand> basicBlockOperandMap = new HashMap<>();

    private final Map<BasicBlockOperand, Operand> entryMap = new HashMap<>();

    public void addEntry(BasicBlock basicBlock, Value value) {
        if (hasEntry(basicBlock)) {
            throw new IllegalArgumentException("Basic block already has a value in this phi instruction");
        }
        var basicBlockOperand = new BasicBlockOperand(basicBlock);
        var valueOperand = new Operand(this, this.getType(), value);
        entryMap.put(basicBlockOperand, valueOperand);
    }

    public boolean hasEntry(BasicBlock basicBlock) {
        return basicBlockOperandMap.containsKey(basicBlock);
    }

    public void removeEntry(BasicBlock basicBlock) {
        if (!hasEntry(basicBlock)) {
            throw new IllegalArgumentException("Basic block does not exist");
        }
        var basicBlockOperand = basicBlockOperandMap.get(basicBlock);
        var valueOperand = entryMap.get(basicBlockOperand);
        basicBlockOperand.setValue(null);
        valueOperand.setValue(null);
        entryMap.remove(basicBlockOperand);
    }

    public Set<BasicBlock> getEntrySet() {
        return basicBlockOperandMap.keySet();
    }

    public void forEach(BiConsumer<BasicBlock, Value> consumer) {
        for (var entry : entryMap.entrySet()) {
            consumer.accept(entry.getKey().getValue(), entry.getValue().getValue());
        }
    }

    public void forWithOperand(BiConsumer<BasicBlock, Operand> consumer) {
        for (var entry : entryMap.entrySet()) {
            consumer.accept(entry.getKey().getValue(), entry.getValue());
        }
    }

    public Value getValue(BasicBlock basicBlock) {
        if (!hasEntry(basicBlock)) {
            throw new IllegalArgumentException("Basic block given is not an entry of this phi.");
        }
        return entryMap.get(basicBlockOperandMap.get(basicBlock)).getValue();
    }

    public void setValue(BasicBlock basicBlock, Value value) {
        if (!hasEntry(basicBlock)) {
            throw new IllegalArgumentException("Basic block given is not an entry of this phi. (Maybe you want to use addEntry)");
        }
        entryMap.get(basicBlockOperandMap.get(basicBlock)).setValue(value);
    }

    @Override
    public List<Operand> getOperandList() {
        var list = new ArrayList<Operand>();
        entryMap.forEach((basicBlockOperand, valueOperand) -> {
            list.add(basicBlockOperand);
            list.add(valueOperand);
        });
        return list;
    }

    @Override
    public void emitIr(StringBuilder builder) {
        builder.append(String.format("%s = phi %s ", this.getValueNameIR(), this.getTypeName()));
        boolean isFirstPair = true;
        for (var entry : entryMap.entrySet()) {
            var basicBlock = entry.getKey().getValue();
            var value = entry.getValue().getValue();
            if (!isFirstPair) {
                builder.append(", ");
            }
            builder.append(String.format("[ %s, %s ]", value.getValueNameIR(), basicBlock.getValueNameIR()));
            isFirstPair = false;
        }
    }
}
