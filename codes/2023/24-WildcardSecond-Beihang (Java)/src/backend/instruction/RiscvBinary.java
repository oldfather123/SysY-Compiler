package backend.instruction;
import backend.component.*;
import backend.operand.RiscvOperand;
import backend.operand.RiscvReg;
import backend.operand.RiscvVirReg;
import ir.Value;
import ir.instr.InstType;
import ir.type.Type;

import java.util.ArrayList;

public class RiscvBinary extends RiscvInstr {
    private final RiscvBinaryType instType;

    public RiscvBinaryType getInstType() {
        return instType;
    }

    public RiscvBinary(ArrayList<RiscvOperand> uses,
                       RiscvReg reg, RiscvBinaryType instType) {
        super(reg, uses);
        this.instType = instType;
    }

    public enum RiscvBinaryType{
        addw,
        add,
        addiw,
        addi,
        subw,
        sub,
        slliw,
        sraiw,
        mulw,
        mul,
        divw,
        remw,
        fadd,
        fsub,
        fmul,
        fdiv,
        feq,
        flt,
        fle,
        sltiu,
        and,
        andi,
        or,
        ori,
        xori,
        slt,
        slti,
        sltu,
        srliw,
        slli
    }

    public String binaryTypeToString(){
        switch(instType){
            case addw -> {
                return "addw";
            }
            case addiw -> {
                return "addiw";
            }
            case subw -> {
                return "subw";
            }
            case slliw -> {
                return "slliw";
            }
            case sraiw -> {
                return "sraiw";
            }
            case mulw -> {
                return "mulw";
            }
            case divw -> {
                return "divw";
            }
            case remw -> {
                return "remw";
            }
            case fadd -> {
                return "fadd.s";
            }
            case fsub -> {
                return "fsub.s";
            }
            case fmul -> {
                return "fmul.s";
            }
            case fdiv -> {
                return "fdiv.s";
            }
            case feq -> {
                return "feq.s";
            }
            case fle -> {
                return "fle.s";
            }
            case flt -> {
                return "flt.s";
            }
            case sltiu -> {
                return "sltiu";
            }
            case and -> {
                return "and";
            }
            case or -> {
                return "or";
            }
            case andi -> {
                return "andi";
            }
            case ori -> {
                return "ori";
            }
            case xori -> {
                return "xori";
            }
            case slt -> {
                return "slt";
            }
            case slti -> {
                return "slti";
            }
            case sltu -> {
                return "sltu";
            }
            case addi -> {
                return "addi";
            }
            case add -> {
                return "add";
            }
            case sub -> {
                return "sub";
            }
            case mul -> {
                return "mul";
            }
            case srliw -> {
                return "srliw";
            }
            case slli -> {
                return "slli";
            }
        }
        return null;
    }

    public boolean is64Ins() {
        return instType == RiscvBinaryType.add || instType == RiscvBinaryType.sub ||
                instType == RiscvBinaryType.mul || instType == RiscvBinaryType.addi ||
                instType == RiscvBinaryType.slt || instType == RiscvBinaryType.slti ||
                instType == RiscvBinaryType.sltiu || instType == RiscvBinaryType.sltu;
    }
    @Override
    public String toString() {
        return binaryTypeToString() + " " + defReg + "," + operands.get(0) +", " + operands.get(1);
    }
}
