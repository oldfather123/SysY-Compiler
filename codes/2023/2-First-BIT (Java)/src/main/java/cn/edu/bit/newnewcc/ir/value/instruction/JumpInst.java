package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.type.LabelType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * 无条件跳转语句
 */
public class JumpInst extends TerminateInst{
    private final ExitOperand exitOperand;

    public JumpInst(){
        this(null);
    }
    /**
     * @param exit 跳转的目标基本块
     */
    public JumpInst(BasicBlock exit) {
        this.exitOperand = new ExitOperand(this, LabelType.getInstance(), exit);
    }

    public BasicBlock getExit() {
        return (BasicBlock) exitOperand.getValue();
    }

    public void setExit(BasicBlock exit) {
        exitOperand.setValue(exit);
    }

    @Override
    public void emitIr(StringBuilder builder) {
        builder.append(String.format(
                "br label %s",
                getExit().getValueNameIR()
        ));
    }

    @Override
    public List<Operand> getOperandList() {
        var list = new ArrayList<Operand>();
        list.add(exitOperand);
        return list;
    }

    @Override
    public Collection<BasicBlock> getExits() {
        var list = new ArrayList<BasicBlock>();
        list.add(getExit());
        return list;
    }

}
