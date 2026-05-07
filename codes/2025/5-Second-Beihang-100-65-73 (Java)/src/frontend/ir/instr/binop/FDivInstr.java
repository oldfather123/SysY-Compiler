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
import java.util.Objects;

public class FDivInstr extends BinaryOperation {
    public FDivInstr(int regIdx, Value op1, Value op2, BasicBlock parentBB) {
        super(new TFloat(), regIdx, op1, op2, "fdiv", parentBB);
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
            simplified = new FloatConst(con1.getConstVal().floatValue() / con2.getConstVal().floatValue());
        } else if (op1 instanceof IRConst con1) {
            if (con1.getConstVal().floatValue() == 0.0f) {
                simplified = new FloatConst(0);
            }
        } else if (op2 instanceof IRConst con2) {
            if (con2.getConstVal().floatValue() == 1) {
                simplified = op1;
//            } else if (op1 instanceof FMulInstr mul) {
//                List<Value> userList = mul.getUserList();
//                if (userList.size() == 1 && userList.getFirst() == this) {
//                    if (mul.getOperand1() instanceof IRConst con) {
//                        float res = con.getConstVal().floatValue() / con2.getConstVal().floatValue();
//                        IRConst newCon = new FloatConst(res);
////                        mul.setOperand1(newCon);
////                        con.replaceUseWith(newCon);
//                        mul.modifyUse(con, newCon);
//                        con.getUserList().remove(mul);
//                        newCon.getUserList().add(mul);
//                        simplified = mul.simplify();
//                    } else if (mul.getOperand2() instanceof IRConst con) {
//                        float res = con.getConstVal().floatValue() / con2.getConstVal().floatValue();
//                        IRConst newCon = new FloatConst(res);
////                        mul.setOperand2(newCon);
////                        con.replaceUseWith(newCon);
//                        mul.modifyUse(con, newCon);
//                        con.getUserList().remove(mul);
//                        newCon.getUserList().add(mul);
//                        simplified = mul.simplify();
//                    }
//                }
            } else if (con2.getConstVal().floatValue() == -1) {
                FloatConst zero = FloatConst.Zero;
                simplified = new FSubInstr(this.getRegIndex(), zero, op1, this.getParentBB());
                simplified.setUse(zero);
                simplified.setUse(op1);
                simplified.insertAfter(this);
            } else if (op1 instanceof FDivInstr div) {
                if (Arg.ARG.getOptLevel() > 0) {
                    List<Value> userList = div.getUserList();
                    if (userList.size() == 1 && userList.getFirst() == this) {
                        if (div.getOperand1() instanceof IRConst con) {
                            float res = con.getConstVal().floatValue() / con2.getConstVal().floatValue();
                            IRConst newCon = new FloatConst(res);
                            div.modifyUseV2(con, newCon);
                            simplified = div.simplify();
                        } else if (div.getOperand2() instanceof IRConst con) {
                            float res = con.getConstVal().floatValue() * con2.getConstVal().floatValue();
                            IRConst newCon = new FloatConst(res);
                            div.modifyUseV2(con, newCon);
                            simplified = div.simplify();
                        }
                    }
                }
            }

        } else if (op1 instanceof Instruction instr1 && op2 instanceof Instruction instr2) {
            if (Objects.equals(instr1.getRegIndex(), instr2.getRegIndex())) {
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

    @Override
    protected boolean isSwappable() {
        return false;
    }
}
