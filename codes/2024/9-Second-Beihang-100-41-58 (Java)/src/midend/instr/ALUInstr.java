package midend.instr;

import midend.BasicBlock;
import midend.Function;
import midend.LLVMType.FloatType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.Type;
import midend.Value;
import midend.Visitor;

import java.util.Arrays;
import java.util.List;

public class ALUInstr extends Instruction {
    private boolean is64 = false;

    public ALUInstr(Type type, List<Value> values, InstrType instrType, BasicBlock block) {
        super(type, "%reg" + (Value.num++), instrType);
        super.addValue(values.get(0));
        super.addValue(values.get(1));
        super.setBasicBlock(block);
    }

    public boolean canUse() {
        return true;
    }

    public String getOpStr() {
        return switch (getInstrType()) {
            case ADD -> "+";
            case FADD -> "+";
            case SUB -> "-";
            case FSUB -> "-";
            case MUL -> "*";
            case FMUL -> "*";
            case SDIV -> "/";
            case FDIV -> "/";
            case SHL -> "<<";
            case LSHR -> ">>>";
            case ASHR -> ">>";
            case ICMP -> ((CmpInstr) this).getOpStr();
            case FCMP -> ((CmpInstr) this).getOpStr();
            case AND -> null;
            case MOD -> "%";
            case FMOD -> "%";
            case OR -> null;
            case CALL -> null;
            case ALLOCA -> null;
            case LOAD -> null;
            case STORE -> null;
            case GETELEMENTPTR -> null;
            case PHI -> null;
            case ZEXT -> null;
            case TRUNC -> null;
            case FPTOSI -> null;
            case SITOFP -> null;
            case BR -> null;
            case RET -> null;
            case BITCAST -> null;
            case PCOPY -> null;
            case MOVE -> null;
        };
    }

    public Value getLeft() {
        return super.getValue(0);
    }

    public Value getRight() {
        return super.getValue(1);
    }

    @Override
    public String getInstr() {
        Value left = getLeft();
        Value right = getRight();
        String type = null;
        if (this.is64) {
            type = "i64 ";
        } else if (left.getType() == IntegerType.i32) {
            type = "i32 ";
        } else if (left.getType() == FloatType.f32) {
            type = "float ";
        } else if (left.getType() == IntegerType.i1) {
            type = "i1 ";
        }
        return this.getName() + " = " +
                getInstrType().toString() + " " +
                type + left.getName() + ", " + right.getName() + "\n";
    }

    @Override
    public ALUInstr clone(BasicBlock block) {
        Value left = getLeft();
        Value right = getRight();
        if (Function.cloneMap.containsKey(left)) {
            left = Function.cloneMap.get(left);
        }
        if (Function.cloneMap.containsKey(right)) {
            right = Function.cloneMap.get(right);
        }
        ALUInstr aluInstr = new ALUInstr(this.getType(), Arrays.asList(left, right), this.getInstrType(), block);
        aluInstr.is64 = this.is64;
        return aluInstr;
    }

    @Override
    public boolean canBeUsed() {
        return true;
    }

    public void modifyValue(Value before, Value after) {
        if (getLeft().equals(before)) {
            setValue(0, after);
        } else if (getRight().equals(before)) {
            setValue(1, after);
        }
    }

    public void modifyInstrType(InstrType integerType) {
        this.setInstrType(integerType);
    }

    public void setIs64() {
        this.is64 = true;
    }

    public boolean getIs64() {
        return this.is64;
    }
}
