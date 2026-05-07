package backend.instruction;

import backend.component.RiscvBlock;
import backend.component.RiscvInstr;
import backend.operand.*;
import ir.instr.BinaryInstr;
import ir.instr.Br;
import ir.instr.InstType;
import ir.value.BasicBlock;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvBranch extends RiscvInstr {
    /**
     * branch仅有整数类型 浮点数就是比较值存在一个寄存器中
     */
    public enum RvCmpType {
        beq,
        bne,
        blt,
        bge
    }

    public String cmpType2Str() {
        switch(cmpType) {
            case beq -> {
                return "beq";
            }
            case bge -> {
                return "bge";
            }
            case blt -> {
                return "blt";
            }
            case bne -> {
                return "bne";
            }
        }
        return null;
    }

    private RvCmpType cmpType;

    public RiscvBranch(RiscvOperand left, RiscvOperand right, RiscvLabel falseTo,
                       RvCmpType cmpType) {
        super(null, new ArrayList<>(Arrays.asList(left, right, falseTo)));
        assert left instanceof RiscvReg && right instanceof RiscvReg;
        this.cmpType = cmpType;
    }

    public void setParent(RiscvBlock block) {
        assert operands.get(2) instanceof RiscvBlock;
        RiscvBlock block1 = (RiscvBlock) operands.get(2);
        block1.addPreds(block);
        block.addSuccs(block1);
    }

    /*
        eq,ne,
        gt,ge,
        lt,le,
        oeq,one,
        ogt,oge,
        olt,ole,
     */
    public static RiscvBranch buildBranch(RiscvOperand left, RiscvOperand right, InstType.CmpType type,
                                          RiscvLabel jLabel, boolean flag) {
        RiscvOperand trueLeft = null;
        RiscvOperand trueRight = null;
        RvCmpType trueCmpType = null;
        if(flag) {
            switch(type) {
                case eq:
                    trueLeft = left;
                    trueRight = right;
                    trueCmpType = RvCmpType.bne;
                    break;
                case ne:
                    trueLeft = left;
                    trueRight = right;
                    trueCmpType = RvCmpType.beq;
                    break;
                case gt:
                    trueLeft = right;
                    trueRight = left;
                    trueCmpType = RvCmpType.bge;
                    break;
                case ge:
                    trueLeft = left;
                    trueRight = right;
                    trueCmpType = RvCmpType.blt;
                    break;
                case lt:
                    trueLeft = left;
                    trueRight = right;
                    trueCmpType = RvCmpType.bge;
                    break;
                case le:
                    trueLeft = right;
                    trueRight = left;
                    trueCmpType = RvCmpType.blt;
            }
        } else {
            switch(type) {
                case eq:
                    trueLeft = left;
                    trueRight = right;
                    trueCmpType = RvCmpType.beq;
                    break;
                case ne:
                    trueLeft = left;
                    trueRight = right;
                    trueCmpType = RvCmpType.bne;
                    break;
                case gt:
                    trueLeft = right;
                    trueRight = left;
                    trueCmpType = RvCmpType.blt;
                    break;
                case ge:
                    trueLeft = left;
                    trueRight = right;
                    trueCmpType = RvCmpType.bge;
                    break;
                case lt:
                    trueLeft = left;
                    trueRight = right;
                    trueCmpType = RvCmpType.blt;
                    break;
                case le:
                    trueLeft = right;
                    trueRight = left;
                    trueCmpType = RvCmpType.bge;
            }
        }
        RiscvBranch branch = new RiscvBranch(trueLeft, trueRight, jLabel, trueCmpType);
        return branch;
    }

    //flag 为true 确保left是1就不跳转 是0跳转 false 则left是0就不跳转
    public static RiscvBranch buildBranch(RiscvOperand left, RiscvLabel jLabel, RvCmpType flag) {
        return new RiscvBranch(left, RiscvCPUReg.getRiscvCPUReg(0), jLabel, flag);
    }

    @Override
    public String toString() {
        return cmpType2Str() + " " + operands.get(0) + "," + operands.get(1) + "," + operands.get(2);
    }
}
