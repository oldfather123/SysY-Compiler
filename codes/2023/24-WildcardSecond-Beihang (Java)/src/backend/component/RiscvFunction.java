package backend.component;

import backend.instruction.RiscvFmv;
import backend.instruction.RiscvMv;
import backend.operand.*;
import ir.Value;
import ir.instr.Alloca;
import ir.instr.Instr;
import ir.value.Arg;
import ir.value.Variable;
import ir.type.*;
import ir.value.user.Function;
import tools.IrRegDispatcher;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;

public class RiscvFunction extends RiscvLabel {
    private ArrayList<RiscvBlock> blocks = new ArrayList<>();

    private int floatArgNum = 0;

    private int otherArgNum = 0;
    private int stackPos = 0;
    private final IrRegDispatcher irRegDispatcher;
    public final ArrayList<RiscvMv> moves = new ArrayList<>();
    private RiscvReg raReg;
    private  LinkedHashSet<RiscvOperand> allVirRegUsed = new LinkedHashSet<>();

    private final LinkedHashMap<Arg, Integer> argPositions = new LinkedHashMap<>();
    private final LinkedHashMap<Object, Integer> value2StackPos = new LinkedHashMap<>();

    public RiscvFunction(String name, IrRegDispatcher irRegDispatcher) {
        super(name);
        this.irRegDispatcher = irRegDispatcher;
    }

    public void addVirReg(RiscvVirReg reg) {
        allVirRegUsed.add(reg);
    }

    public void addBlock(RiscvBlock block) {
        blocks.add(block);
        block.setName(name+block.getName());
    }

    public int getFloatArgNum() {
        return floatArgNum;
    }

    public int getOtherArgNum() {
        return otherArgNum;
    }

    public LinkedHashSet<RiscvOperand> getAllVirRegUsed() {
        return new LinkedHashSet<>(this.allVirRegUsed);
    }

    public void genArgPos(Function function, LinkedHashMap<Value, RiscvReg> value2Reg){
        int floatPos = 0;
        int otherPos = 0;
        ArrayList<Variable> arguments = function.getArgv();
        for(int i = 0;i < arguments.size();i++) {
            Variable argument = arguments.get(i);
            Arg arg = function.getArgv2().get(i);
            if (argument.getType() instanceof FloatType)
            {
                argPositions.put(arg, floatPos);
                if (floatPos >= 8)
                {
                    stackPos+=8;
                    value2StackPos.put(arg, stackPos);
                } else {
                    RiscvVirReg reg = new RiscvVirReg(irRegDispatcher,
                            RiscvVirReg.RegType.floatType, this);
                    moves.add(new RiscvFmv(RiscvFPUReg.getRiscvFPUReg(10 + floatPos),reg));
                    value2Reg.put(arg, reg);
                }
                floatPos++;
            } else {
                argPositions.put(arg, otherPos);
                if (otherPos >= 8) {
                    stackPos+=8;
                    value2StackPos.put(arg, stackPos);
                } else {
                    RiscvVirReg reg = new RiscvVirReg(irRegDispatcher,
                            RiscvVirReg.RegType.intType, this);
                    moves.add(new RiscvMv(RiscvCPUReg.getRiscvCPUReg(10 + otherPos),reg));
                    value2Reg.put(arg, reg);
                }
                otherPos++;
            }
        }
        this.otherArgNum = otherPos;
        this.floatArgNum = floatPos;
    }

    public void alloc(Instr ins) {
        assert ins instanceof Alloca;
        stackPos += ((Alloca) ins).getAllocType().getOffset();
        value2StackPos.put(ins, stackPos);
    }

    public void alloc(RiscvVirReg reg) {
        stackPos += 4;
        value2StackPos.put(reg, stackPos);
    }

    public void alloc8(RiscvVirReg reg) {
        stackPos += 8;
        value2StackPos.put(reg, stackPos);
    }

    public int getOffset(Object obj) {
        assert obj instanceof Alloca || obj instanceof RiscvVirReg || obj instanceof Arg;
        return value2StackPos.get(obj);
    }

    public void reMap(RiscvVirReg dest, RiscvVirReg src) {
        value2StackPos.put(dest, value2StackPos.get(src));
        allVirRegUsed.remove(src);
        allVirRegUsed.add(dest);
    }

    public void updateAlluse(RiscvVirReg dest, RiscvVirReg src){
        allVirRegUsed.remove(src);
        allVirRegUsed.add(dest);
    }

    public int getStackPos() {
        return stackPos;
    }

    @Override
    public String toString() {
        return name;
    }

    public String toPrint() {
        if(blocks.size() == 0)
            return "";
        StringBuilder sb = new StringBuilder();
        sb.append("""
            .align\t1
            .globl""").append(" ").append(name).append('\n');
        sb.append(super.toPrint());
        for(RiscvBlock block: blocks) {
            sb.append(block.toPrint());
        }
        return sb.toString();
    }

    public ArrayList<RiscvBlock> getBlocks() {
        return blocks;
    }

    public IrRegDispatcher getIrRegDispatcher() {
        return irRegDispatcher;
    }

    public RiscvReg getRaReg() {
        return raReg;
    }

    public void setRaReg(RiscvReg raReg) {
        this.raReg = raReg;
    }
}
