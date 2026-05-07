package frontend.ir.instr.binop;

import frontend.ir.Value;
import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.structure.BasicBlock;
import midend.ConstFold;

import java.util.ArrayList;
import java.util.Objects;

public class SDivInstr extends BinaryOperation {
    public SDivInstr(int regIdx, Value op1, Value op2, BasicBlock parentBB) {
        super(new TInt(), regIdx, op1, op2, "sdiv", parentBB);
    }

    @Override
    protected boolean isSwappable() {
        return false;
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
            simplified = new IntConst(con1.getConstVal().intValue() / con2.getConstVal().intValue());
        } else if (op1 instanceof IRConst con1) {
            if (con1.getConstVal().intValue() == 0) {
                simplified = new IntConst(0);
            }
        } else if (op2 instanceof IRConst con2) {
            if (con2.getConstVal().intValue() == 1) {
                simplified = op1;
            } else if (con2.getConstVal().intValue() == -1) {
                IntConst zero = IntConst.Zero;
                simplified = new SubInstr(this.getRegIndex(), zero, op1, this.getParentBB());
                simplified.setUse(zero);
                simplified.setUse(op1);
                simplified.insertAfter(this);
            } else if (op1 instanceof SDivInstr sdiv) {
                if (sdiv.getOperand2() instanceof IRConst con) {
                    int res = con.getConstVal().intValue() * con2.getConstVal().intValue();
                    IRConst newCon = new IntConst(res);
                    this.modifyUseV2(con2, newCon);
                    this.modifyUseV2(op1, sdiv.getOperand1());
                    if (sdiv.getUserList().isEmpty()) {
                        sdiv.removeFromList();
                    }
                    this.simplify();
                }
            }
        } else if (op1 instanceof Instruction instr1 && op2 instanceof Instruction instr2) {
            if (Objects.equals(instr1, instr2)) {
                simplified = new IntConst(1);
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
}
