package cn.edu.bit.newnewcc.ir.value;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.exception.UsageRelationshipCheckFailedException;
import cn.edu.bit.newnewcc.ir.type.LabelType;
import cn.edu.bit.newnewcc.ir.util.InstructionList;
import cn.edu.bit.newnewcc.ir.value.instruction.AllocateInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;
import cn.edu.bit.newnewcc.ir.value.instruction.TerminateInst;

import java.util.*;

/**
 * 基本块
 */
public class BasicBlock extends Value {

    public enum Tag {
        NO_LOOP_UNROLL
    }

    public final Set<Tag> tags = new HashSet<>();

    public BasicBlock() {
        super(LabelType.getInstance());
    }

    @Override
    public LabelType getType() {
        return (LabelType) super.getType();
    }

    /// 名称

    private String valueName = null;

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

    public void emitIr(StringBuilder builder) {
        builder.append(this.getValueName()).append(":\n");
        for (Instruction instruction : this.getInstructions()) {
            builder.append("    ");
            instruction.emitIr(builder);
            if (instruction.getComment() != null) {
                builder.append(" ;").append(instruction.getComment());
            }
            builder.append('\n');
        }
    }

    private final InstructionList instructionList = new InstructionList(this);

    /**
     * 向基本块中加入一条指令
     * <p>
     * 对于一般的指令，其会被添加到主指令体的末尾；
     * 对于前导指令，其会被添加到基本块开头；
     * 对于结束指令，其会替换现有的结束指令
     *
     * @param instruction 待插入的指令
     */
    public void addInstruction(Instruction instruction) {
        if (isLeadingInstruction(instruction)) {
            instructionList.addLeadingInst(instruction);
            return;
        }
        if (isTerminateInstruction(instruction)) {
            setTerminateInstruction((TerminateInst) instruction);
            return;
        }
        instructionList.appendMainInstruction(instruction);
    }

    public void addMainInstructionAtBeginning(Instruction instruction) {
        assert !isLeadingInstruction(instruction) && !isTerminateInstruction(instruction);
        instructionList.appendMainInstructionAtBeginning(instruction);
    }

    public void setTerminateInstruction(TerminateInst terminateInstruction) {
        if (getTerminateInstruction() != null) {
            getTerminateInstruction().getExits().forEach(exit -> {
                if (exit != null) {
                    exit.__removeEntryBlock__(this);
                }
            });
        }
        instructionList.setTerminateInstruction(terminateInstruction);
        if (getTerminateInstruction() != null) {
            getTerminateInstruction().getExits().forEach(exit -> {
                if (exit != null) {
                    exit.__addEntryBlock__(this);
                }
            });
        }
    }

    /**
     * 获取所有前导指令
     *
     * @return 前导指令集合
     */
    public List<Instruction> getLeadingInstructions() {
        var list = new ArrayList<Instruction>();
        instructionList.getLeadingInstructions().forEachRemaining(list::add);
        return Collections.unmodifiableList(list);
    }

    /**
     * 获取所有主体指令
     *
     * @return 主体指令集合
     */
    public List<Instruction> getMainInstructions() {
        var list = new ArrayList<Instruction>();
        instructionList.getMainInstructions().forEachRemaining(list::add);
        return Collections.unmodifiableList(list);
    }

    /**
     * 获取结束指令
     * <p>
     * 没有设置结束指令时，返回 null
     *
     * @return 结束指令或 null
     */
    public TerminateInst getTerminateInstruction() {
        return instructionList.getTerminateInstruction();
    }

    /**
     * 获取所有指令
     *
     * @return 所有指令的迭代器
     */
    public List<Instruction> getInstructions() {
        var list = new ArrayList<Instruction>();
        instructionList.iterator().forEachRemaining(list::add);
        return Collections.unmodifiableList(list);
    }

    public Collection<BasicBlock> getExitBlocks() {
        return getTerminateInstruction().getExits();
    }

    /// 基本块入口维护

    private final Set<BasicBlock> entryBlockSet = new HashSet<>();

    public void __addEntryBlock__(BasicBlock basicBlock) {
        if (!entryBlockSet.add(basicBlock)) {
            throw new IllegalArgumentException("Specified basic block is already an entry of this block.");
        }
    }

    public void __removeEntryBlock__(BasicBlock basicBlock) {
        if (!entryBlockSet.contains(basicBlock)) {
            throw new IllegalArgumentException("Specified basic block is not an entry of this block.");
        }
        entryBlockSet.remove(basicBlock);
    }

    /**
     * 获取该基本块的所有入口块
     *
     * @return 基本块的所有入口块的只读副本
     */
    public Set<BasicBlock> getEntryBlocks() {
        return Collections.unmodifiableSet(entryBlockSet);
    }

    /**
     * 将入口从该基本块的所有PHI指令中移除
     *
     * @param entry 入口
     */
    public void removeEntryFromPhi(BasicBlock entry) {
        for (Instruction leadingInstruction : getLeadingInstructions()) {
            if (leadingInstruction instanceof PhiInst phiInst && phiInst.hasEntry(entry)) {
                phiInst.removeEntry(entry);
            }
        }
    }

    /// 基本块与函数的关系

    /**
     * 该基本块所在的函数
     */
    private Function function = null;

    private boolean isFunctionFixed = false;

    public Function getFunction() {
        return function;
    }

    public void removeFromFunction() {
        if (function != null) {
            function.removeBasicBlock(this);
        }
    }

    /**
     * 设置该基本块所在的函数
     * <p>
     * 不要在Function类以外的任何地方调用该函数！
     *
     * @param function 基本块所在的函数
     */
    public void __setFunction__(Function function, boolean shouldFixFunction) {
        // 检查所属函数是否被锁定，锁定则不能被修改
        if (this.isFunctionFixed) {
            throw new UsageRelationshipCheckFailedException("Belonging of this basic block has been fixed");
        }
        // 检查是否直接修改所属函数，必须先从其他函数中移除该基本块
        if (this.function != null && function != null) {
            throw new UsageRelationshipCheckFailedException("A basic block is being added to another function without removing from the original function");
        }
        // 设置函数锁定
        if (shouldFixFunction) {
            // 不可以锁定到null上
            if (function == null) {
                throw new IllegalArgumentException("Cannot fix null as the belonging of this basic block");
            }
            this.isFunctionFixed = true;
        }
        this.function = function;
    }

    /**
     * 设置该基本块不在任何函数内
     * <p>
     * 不要在Function类以外的任何地方调用该函数！
     */
    public void __clearFunction__() {
        __setFunction__(null, false);
    }

    public static boolean isLeadingInstruction(Instruction instruction) {
        return instruction instanceof AllocateInst || instruction instanceof PhiInst;
    }

    public static boolean isTerminateInstruction(Instruction instruction) {
        return instruction instanceof TerminateInst;
    }

}
