package frontend.ir.instr.otherop;

import frontend.ir.Value;
import frontend.ir.constant.BoolConst;
import frontend.ir.constant.IRConst;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.arithmetic.TBool;
import frontend.ir.structure.BasicBlock;
import midend.ConstFold;

import java.util.*;

/**
 * 比较指令，包括用于（有符号）整数的 icmp 和用于浮点数的 fcmp
 */
public class CmpInstr extends Instruction {
    private CmpCond cond;
    private final Type operandType;
    private Value operand1;
    private Value operand2;

    public CmpInstr(int regIdx, CmpCond cond, Value op1, Value op2, BasicBlock parentBB) {
        super(new TBool(), regIdx, parentBB);
        this.cond = cond;
        this.operand1 = op1;
        this.operand2 = op2;

        if (op1 == null || op2 == null) {
            throw new NullPointerException();
        }
        this.operandType = op1.getType();
        if (!operandType.equals(op2.getType())) {
            throw new RuntimeException("cmp 两个操作数类型必须一致");
        }

        if (parentBB.isNotClosed()) {
            this.setUse(op1);
            this.setUse(op2);
        }
    }

    public boolean isI32() {
        return !this.cond.isForFloat();
    }

    public CmpCond getCond() {
        return cond;
    }

    public void invertCond() {
        this.cond = this.cond.invert();
    }

    /**
     * 标准化顺序：常数必在右
     */
    public void swapOrder() {
        Value oldOperand1 = this.operand1;
        this.operand1 = this.operand2;
        this.operand2 = oldOperand1;
        this.cond = cond.swap();
    }

    public Value getOperand1() {
        return operand1;
    }

    public Value getOperand2() {
        return operand2;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (this.operand1 == from) {
            this.operand1 = to;
        }
        if (this.operand2 == from) {
            this.operand2 = to;
        }
    }

    @Override
    public String toString() {
        return this.value2string() + " = " + (this.cond.isForFloat() ? "fcmp " : "icmp ") + this.cond + " " +
                operandType.printIRType() + " " + operand1.value2string() + ", " + operand2.value2string();
    }

    @Override
    public String myHash() {
        if (this.cond == CmpCond.IEQ || this.cond == CmpCond.INE) {
            boolean bothIns = operand1 instanceof Instruction && operand2 instanceof Instruction;
            boolean outOfOrder;
            if (bothIns) {
                outOfOrder = operand1.value2string().compareTo(operand2.value2string()) > 0;
            } else {
                outOfOrder = !(operand1 instanceof Instruction) && operand2 instanceof Instruction;
            }
            if (outOfOrder) {
                return operand2.value2string() + " " + operand1.value2string() + " " + this.cond;
            }
        } else if (this.cond == CmpCond.IGE) {
            return operand2.value2string() + " " + operand1.value2string() + " " + CmpCond.ILE;
        } else if (this.cond == CmpCond.IGT) {
            return operand2.value2string() + " " + operand1.value2string() + " " + CmpCond.ILT;
        }

        return operand1.value2string() + " " + operand2.value2string() + " " + this.cond;
    }

    @Override
    public Value simplify() {
        if (isRemoved) {
            return this;
        }

        Value simplified = null;

        if (this.cond == CmpCond.IEQ || this.cond == CmpCond.FEQ) {
            if (Objects.equals(this.operand1, this.operand2)) {
                simplified = new BoolConst(1);
            }
        }

        if (simplified == null && this.operand1 instanceof IRConst con1 && this.operand2 instanceof IRConst con2) {
            simplified = new BoolConst(switch (this.cond) {
                case IEQ -> con1.getConstVal().intValue() == con2.getConstVal().intValue();
                case INE -> con1.getConstVal().intValue() != con2.getConstVal().intValue();
                case IGT -> con1.getConstVal().intValue() > con2.getConstVal().intValue();
                case IGE -> con1.getConstVal().intValue() >= con2.getConstVal().intValue();
                case ILT -> con1.getConstVal().intValue() < con2.getConstVal().intValue();
                case ILE -> con1.getConstVal().intValue() <= con2.getConstVal().intValue();
                case FEQ -> con1.getConstVal().floatValue() == con2.getConstVal().floatValue();
                case FNE -> con1.getConstVal().floatValue() != con2.getConstVal().floatValue();
                case FGT -> con1.getConstVal().floatValue() > con2.getConstVal().floatValue();
                case FGE -> con1.getConstVal().floatValue() >= con2.getConstVal().floatValue();
                case FLT -> con1.getConstVal().floatValue() < con2.getConstVal().floatValue();
                case FLE -> con1.getConstVal().floatValue() <= con2.getConstVal().floatValue();
            } ? 1 : 0);
        }

        if (simplified != null) {
            ArrayList<Value> users = new ArrayList<>(this.getUserList());
            this.replaceUseWith(simplified);
            this.removeFromList();
            if (ConstFold.depthCounter < ConstFold.MAX_DEPTH) {
                for (Value user : users) {
                    if (user instanceof Instruction ins) {
                        ConstFold.depthCounter++;
                        ins.simplify();
                    } else {
                        throw new RuntimeException("CmpInstr simplify error: user not instruction");
                    }
                }
            } else {
                ConstFold.depthCounter = 0;
            }
            return simplified;
        } else {
            return this;
        }
    }

    public enum CmpCond {
        IEQ("eq", false),
        INE("ne", false),
        IGT("sgt", false),
        IGE("sge", false),
        ILT("slt", false),
        ILE("sle", false),

        FEQ("oeq", true),
        FNE("one", true),
        FGT("ogt", true),
        FGE("oge", true),
        FLT("olt", true),
        FLE("ole", true),
        ;

        private final String nameInIR;
        private final boolean forFloat;
        private static final Map<CmpCond, CmpCond> invertMap = new HashMap<>();

        static {
            invertMap.put(IEQ, INE);
            invertMap.put(INE, IEQ);
            invertMap.put(IGT, ILE);
            invertMap.put(IGE, ILT);
            invertMap.put(ILT, IGE);
            invertMap.put(ILE, IGT);

            invertMap.put(FEQ, FNE);
            invertMap.put(FNE, FEQ);
            invertMap.put(FGT, FLE);
            invertMap.put(FGE, FLT);
            invertMap.put(FLT, FGE);
            invertMap.put(FLE, FGT);
        }

        CmpCond(String nameInIR, boolean forFloat) {
            this.nameInIR = nameInIR;
            this.forFloat = forFloat;
        }

        public boolean isForFloat() {
            return forFloat;
        }

        public CmpCond invert() {
            return invertMap.get(this);
        }

        public CmpCond swap() {
            return switch (this) {
                case IEQ -> IEQ;
                case INE -> INE;
                case IGT -> ILT;
                case ILT -> IGT;
                case IGE -> ILE;
                case ILE -> IGE;

                case FEQ -> FEQ;
                case FNE -> FNE;
                case FGT -> FLT;
                case FLT -> FGT;
                case FGE -> FLE;
                case FLE -> FGE;
            };
        }

        @Override
        public String toString() {
            return nameInIR;
        }
    }

    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(operand1);
        operands.add(operand2);
        return operands;
    }
}
