package cn.edu.bit.newnewcc.ir.value;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.exception.ValueBeingUsedException;
import cn.edu.bit.newnewcc.ir.util.InstructionList;

import java.util.List;

public abstract class Instruction extends Value {

    /**
     * @param type 语句返回值的类型
     */
    protected Instruction(Type type) {
        super(type);
        this.node = new InstructionList.Node(this);
    }

    /**
     * 废弃该语句
     * <p>
     * 此操作会将该语句从基本块中移除，清除所有操作数绑定
     */
    public void waste() {
        if (getUsages().size() > 0) {
            throw new ValueBeingUsedException();
        }
        if (getBasicBlock() != null) {
            removeFromBasicBlock();
        }
        clearOperands();
    }

    /// Name

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

    private String comment = null;

    public void setComment(String comment) {
        this.comment = comment;
    }

    public String getComment() {
        return comment;
    }

    public abstract void emitIr(StringBuilder builder);


    /// Operands

    /**
     * @return 当前语句用到的操作数的列表
     */
    public abstract List<Operand> getOperandList();

    /**
     * 清除所有操作数
     */
    public void clearOperands() {
        for (Operand operand : getOperandList()) {
            operand.removeValue();
        }
    }

    /// BasicBlock & InstructionList

    private final InstructionList.Node node;

    public BasicBlock getBasicBlock() {
        return node.getBasicBlock();
    }

    /**
     * 将当前节点插入到乙节点后方
     * <p>
     * 在插入前，需保证当前节点不属于任何链表
     *
     * @param beta 乙节点
     */
    public void insertAfter(Instruction beta) {
        if (BasicBlock.isLeadingInstruction(beta) || BasicBlock.isTerminateInstruction(beta)) {
            throw new CompilationProcessCheckFailedException("Cannot insert instruction after special instructions.");
        }
        InstructionList.insertAlphaAfterBeta(this.node, beta.node);
    }

    /**
     * 将当前节点插入到乙节点前方
     * <p>
     * 在插入前，需保证当前节点不属于任何链表
     *
     * @param beta 乙节点
     */
    public void insertBefore(Instruction beta) {
        if (BasicBlock.isLeadingInstruction(beta) || BasicBlock.isTerminateInstruction(beta)) {
            throw new CompilationProcessCheckFailedException("Cannot insert instruction before special instructions.");
        }
        InstructionList.insertAlphaBeforeBeta(this.node, beta.node);
    }

    /**
     * 将该语句从当前基本块中移除
     */
    public void removeFromBasicBlock() {
        InstructionList.removeNodeFromList(node);
    }

    /**
     * 将当前指令替换为另一个指令 <br>
     * 当前指令立即被废弃 <br>
     *
     * @param newInstruction 新指令
     */
    public void replaceInstructionTo(Instruction newInstruction) {
        replaceAllUsageTo(newInstruction);
        newInstruction.insertAfter(this);
        this.waste();
    }

    /**
     * 获取当前指令所处的链表节点
     * <p>
     * 不要在InstructionList类以外的任何地方调用该函数！
     *
     * @return 当前指令所处的链表节点
     */
    public InstructionList.Node __getInstructionListNode__() {
        return node;
    }

    // 禁止重写
    // Instruction对象在部分Pass中会被放入Set，需要以内存地址鉴别等价性
    @Override
    public final boolean equals(Object obj) {
        return super.equals(obj);
    }

    // 禁止重写
    // Instruction对象在部分Pass中会被放入Set，需要以内存地址鉴别等价性
    @Override
    public final int hashCode() {
        return super.hashCode();
    }
}
