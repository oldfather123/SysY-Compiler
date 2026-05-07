package frontend.ir.instr.binop;

import frontend.ir.Value;
import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.structure.BasicBlock;
import midend.ConstFold;

import java.util.ArrayList;

public class MulInstr extends BinaryOperation {
    public MulInstr(int regIdx, Value op1, Value op2, BasicBlock parentBB) {
        super(new TInt(), regIdx, op1, op2, "mul", parentBB);
    }

    @Override
    protected boolean isSwappable() {
        return true;
    }

    @Override
    public Value simplify() {
        if (isRemoved) {
            return this;
        }
        Value simplified = null;

        Value op1 = this.getOperand1();
        Value op2 = this.getOperand2();
        if (op1 instanceof IRConst con1 && op2 instanceof IRConst con2) {
            simplified = new IntConst(con1.getConstVal().intValue() * con2.getConstVal().intValue());
        } else if (op1 instanceof IRConst con1) {
            if (con1.getConstVal().intValue() == 0) {
                simplified = new IntConst(0);
            } else if (con1.getConstVal().intValue() == -1) {
                IntConst zero = IntConst.Zero;
                simplified = new SubInstr(this.getRegIndex(), zero, op2, getParentBB());
                simplified.setUse(zero);
                simplified.setUse(op2);
                simplified.insertAfter(this);
            } else if (con1.getConstVal().intValue() == 1) {
                simplified = op2;
            } else {
                simplified = mergeConst(con1, op2);
            }
        } else if (op2 instanceof IRConst con2) {
            if (con2.getConstVal().intValue() == 0) {
                simplified = new IntConst(0);
            } else if (con2.getConstVal().intValue() == -1) {
                IntConst zero = IntConst.Zero;
                simplified = new SubInstr(this.getRegIndex(), zero, op1, getParentBB());
                simplified.setUse(zero);
                simplified.setUse(op1);
                simplified.insertAfter(this);
            } else if (con2.getConstVal().intValue() == 1) {
                simplified = op1;
            } else {
                simplified = mergeConst(con2, op1);
            }
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
                        throw new RuntimeException("ConversionOperation simplify error: user not instruction");
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

    private Value mergeConst(IRConst con, Value nonCon) {
        if (nonCon instanceof MulInstr mul) {
            if (mul.getOperand1() instanceof IntConst con1) {
                int res = con1.getConstInt().intValue() * con.getConstVal().intValue();
                IRConst newCon = new IntConst(res);
                this.modifyUseV2(con, newCon);
                this.modifyUseV2(nonCon, mul.getOperand2());
                if (mul.getUserList().isEmpty()) {
                    mul.removeFromList();
                }
                this.simplify();
            } else if (mul.getOperand2() instanceof IntConst con2) {
                int res = con2.getConstInt().intValue() * con.getConstVal().intValue();
                IRConst newCon = new IntConst(res);
                this.modifyUseV2(con, newCon);
                this.modifyUseV2(nonCon, mul.getOperand1());
                if (mul.getUserList().isEmpty()) {
                    mul.removeFromList();
                }
                this.simplify();
            }
        }
        return null;
    }
}
