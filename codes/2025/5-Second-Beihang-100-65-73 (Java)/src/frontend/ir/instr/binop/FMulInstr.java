package frontend.ir.instr.binop;

import IO.Arg;
import frontend.ir.Value;
import frontend.ir.constant.FloatConst;
import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.structure.BasicBlock;
import midend.ConstFold;

import java.util.ArrayList;
import java.util.List;

public class FMulInstr extends BinaryOperation {
    public FMulInstr(int regIdx, Value op1, Value op2, BasicBlock parentBB) {
        super(new TFloat(), regIdx, op1, op2, "fmul", parentBB);
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
            simplified = new FloatConst(con1.getConstVal().floatValue() * con2.getConstVal().floatValue());
        } else if (op1 instanceof IRConst con1) {
            if (con1.getConstVal().floatValue() == 0) {
                simplified = new FloatConst(0);
            } else if (con1.getConstVal().floatValue() == 1) {
                simplified = op2;
            } else if (con1.getConstVal().floatValue() == -1) {
                FloatConst zero = FloatConst.Zero;
                simplified = new FSubInstr(this.getRegIndex(), zero, op2, this.getParentBB());
                simplified.setUse(zero);
                simplified.setUse(op2);
                simplified.insertAfter(this);
            } else {
                simplified = mergeConst(con1, op2);
            }
        } else if (op2 instanceof IRConst con2) {
            if (con2.getConstVal().floatValue() == 0) {
                simplified = new IntConst(0);
            } else if (con2.getConstVal().floatValue() == 1) {
                simplified = op1;
            } else if (con2.getConstVal().floatValue() == -1) {
                FloatConst zero = FloatConst.Zero;
                simplified = new FSubInstr(this.getRegIndex(), zero, op1, this.getParentBB());
                simplified.setUse(zero);
                simplified.setUse(op1);
                simplified.insertAfter(this);
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
        if (nonCon instanceof FMulInstr mul) {
            if (mul.getOperand1() instanceof FloatConst con1) {
                float res = con1.getConstVal().floatValue() * con.getConstVal().floatValue();
                IRConst newCon = new FloatConst(res);
                this.modifyUseV2(con, newCon);
                this.modifyUseV2(nonCon, mul.getOperand2());
                if (mul.getUserList().isEmpty()) {
                    mul.removeFromList();
                }
                this.simplify();
            } else if (mul.getOperand2() instanceof FloatConst con2) {
                float res = con2.getConstVal().floatValue() * con.getConstVal().floatValue();
                IRConst newCon = new FloatConst(res);
                this.modifyUseV2(con, newCon);
                this.modifyUseV2(nonCon, mul.getOperand1());
                if (mul.getUserList().isEmpty()) {
                    mul.removeFromList();
                }
                this.simplify();
            }
        } else if (nonCon instanceof FDivInstr div) {
            if (Arg.ARG.getOptLevel() > 0) {
                List<Value> userList = div.getUserList();
                if (userList.size() == 1 && userList.getFirst() == this) {
                    // %1 = fdiv 1, %0
                    // %2 = fmul %1, 0.5
                    if (div.getOperand1() instanceof FloatConst con1) {
                        float res = con1.getConstVal().floatValue() * con.getConstVal().floatValue();
                        IRConst newCon = new FloatConst(res);
                        div.modifyUseV2(con1, newCon);
                        return div.simplify();
                    }
                }
            }
        }

        return null;
    }

    @Override
    protected boolean isSwappable() {
        return true;
    }
}
