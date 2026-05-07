package midend.instr;

import midend.BasicBlock;
import midend.Function;
import midend.LLVMType.Type;
import midend.LLVMType.VoidType;
import midend.Value;

import java.util.Arrays;
import java.util.List;

public class BrInstr extends Instruction {
    private boolean isIf = false; //跳转类型
    private BasicBlock destBlock;
    private BasicBlock ifTrueBlock;
    private BasicBlock ifFalseBlock;
    public BrInstr(Value cond, List<BasicBlock> basicBlocks) {
        super(VoidType.voidType, "br", InstrType.BR);
        this.isIf = true;
        this.addValue(cond);
        this.ifTrueBlock = basicBlocks.get(0);
        this.ifFalseBlock = basicBlocks.get(1);
    }

    public BrInstr(BasicBlock destBlock) {
        super(VoidType.voidType, "br", InstrType.BR);
        this.isIf = false;
        this.destBlock = destBlock;
    }

    public boolean getIsIf() {
        return this.isIf;
    }

    public Value getValue() {
        return this.getValue(0);
    }

    public BasicBlock getDestBlock() {
        return this.destBlock;
    }

    public BasicBlock getIfTrueBlock() {
        return this.ifTrueBlock;
    }

    public void setIfTrueBlock(BasicBlock block) {
        this.ifTrueBlock = block;
    }

    public BasicBlock getIfFalseBlock() {
        return this.ifFalseBlock;
    }

    public void setIfFalseBlock(BasicBlock block) {
        this.ifFalseBlock = block;
    }

    @Override
    public String getInstr() {
        if (this.isIf) {
            Value cond = getValue();
            return "br i1 " + cond.getName() + ", label %" + this.ifTrueBlock.getName() +
                    ", label %" + this.ifFalseBlock.getName() + "\n";
        } else {
            return "br label %" + destBlock.getName() + "\n";
        }
    }

    @Override
    public BrInstr clone(BasicBlock block) {
        BrInstr brInstr = null;
        if (this.isIf) {
            Value cond = getValue();
            BasicBlock ifTrueBlock = this.ifTrueBlock;
            BasicBlock ifFalseBlock = this.ifFalseBlock;
            if (Function.cloneMap.containsKey(cond)) {
                cond = Function.cloneMap.get(cond);
            }
            if (Function.cloneMap.containsKey(ifTrueBlock)) {
                ifTrueBlock = (BasicBlock) Function.cloneMap.get(ifTrueBlock);
            }
            if (Function.cloneMap.containsKey(ifFalseBlock)) {
                ifFalseBlock = (BasicBlock) Function.cloneMap.get(ifFalseBlock);
            }
            brInstr = new BrInstr(cond, Arrays.asList(ifTrueBlock, ifFalseBlock));
        } else {
            BasicBlock destBlock = this.destBlock;
            if (Function.cloneMap.containsKey(destBlock)) {
                destBlock = (BasicBlock) Function.cloneMap.get(destBlock);
            }
            brInstr = new BrInstr(destBlock);
        }
        brInstr.setBasicBlock(block);
        return brInstr;
    }

    public void modifyBlock(BasicBlock before, BasicBlock after) {
        if (isIf) {
            if (ifTrueBlock.equals(before)) {
                ifTrueBlock = after;
            } else if (ifFalseBlock.equals(before)) {
                ifFalseBlock = after;
            }
        } else {
            if (destBlock.equals(before)) {
                destBlock = after;
            }
        }
    }
}
