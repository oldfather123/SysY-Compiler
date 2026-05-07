package midend.value.instrs;

import midend.MyModule;
import midend.Type;
import midend.value.BasicBlock;
import midend.value.Constant;
import midend.value.Function;
import midend.value.Value;

import java.util.ArrayList;

public class CallVoid extends Instr {
    public CallVoid(BasicBlock parent, Value func, ArrayList<Value> args) {
        super(parent);
        setName("");
        setType(new Type.VoidType());
        this.getOperandList().add(func);
        ArrayList<Value> realArgs = ((Function) func).getArgument();
        for (int i = 0; i < args.size(); i++) {
            Type nowType = args.get(i).getType();
            Value now = args.get(i);
            if (!MyModule.myModule.getInFunc().contains(func.getName())) {
                Type tarType = realArgs.get(i).getType();
                if (nowType.isBasicType() && tarType.isBasicType()) {
                    boolean isNowI1 = nowType.isIntegerType() && ((Type.IntegerType) nowType).isI1();
                    boolean isNowI32 = nowType.isIntegerType() && ((Type.IntegerType) nowType).isI32();
                    boolean isNowFloat = nowType.isFloatType();
                    boolean isTarI1 = tarType.isIntegerType() && ((Type.IntegerType) tarType).isI1();
                    boolean isTarI32 = tarType.isIntegerType() && ((Type.IntegerType) tarType).isI32();
                    boolean isTarFloat = tarType.isFloatType();
                    if (isNowI1) {
                        if (isTarI32) {
                            now = I1toI32(now, parent);
                        } else if (isTarFloat) {
                            now = I32toF(now, parent);
                        }
                    } else if (isNowI32) {
                        if (isTarI1) {
                            now = new BinaryOp(true, now, Op.Ne, Constant.ConstantInteger.Constant0, parent);
                        } else if (isTarFloat) {
                            now = I32toF(now, parent);
                        }
                    } else if (isNowFloat) {
                        if (isTarI1) {
                            now = FtoI32(now, parent);
                            now = new BinaryOp(true, now, Op.Ne, Constant.ConstantInteger.Constant0, parent);
                        } else if (isTarI32) {
                            now = FtoI32(now, parent);
                        }
                    }
                }
            } else if (func.getName().equals("putint") || func.getName().equals("putch")) {
                if (nowType.isFloatType()) {
                    now = FtoI32(now, parent);
                } else if (nowType.isIntegerType()) {
                    if (((Type.IntegerType) nowType).isI1()) {
                        now = I1toI32(now, parent);
                    }
                }
            } else if (func.getName().equals("putfloat")) {
                if (nowType.isIntegerType()) {
                    now = I32toF(now, parent);
                }
            }
            this.getOperandList().add(now);
        }
        for (int i = 0; i < this.getOperandList().size(); i++) {
            this.addUseOp(this.getOperandList().get(i), i);
        }

        this.getParent().addInstr(this);
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        Value func = this.getOperandList().get(0);
        sb.append("call ");
        sb.append(func.getType()).append(" ");
        sb.append("@").append(func.getName());
        sb.append("(");
        int size = this.getOperandList().size();
        for (int i = 1; i < size; i++) {
            if (i > 1) {
                sb.append(", ");
            }
            sb.append(this.getOperandList().get(i).getType()).append(" ");
            sb.append(this.getOperandList().get(i).getName());
        }
        sb.append(")");
        return sb.toString();
    }
}
