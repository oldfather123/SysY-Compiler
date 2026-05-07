package frontend.ir.instr.convop;

import frontend.ir.Value;
import frontend.ir.constant.*;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.arithmetic.TBool;
import frontend.ir.objecttype.arithmetic.TChar;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.structure.BasicBlock;
import midend.ConstFold;

import java.util.ArrayList;
import java.util.HashSet;

public abstract class ConversionOperation extends Instruction {
    private final Type from;
    private final Type target;
    private Value value;
    private final String operatorName;

    protected ConversionOperation(int regIdx, Type target, Value value, String operatorName, BasicBlock parentBB) {
        super(target, regIdx, parentBB);
        this.from = value.getType();
        this.target = target;
        this.value = value;
        this.operatorName = operatorName;

        if (parentBB.isNotClosed()) {
            this.setUse(value);
        }
    }

    public Value getValue() {
        return value;
    }

    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(value);
        return operands;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (this.value == from) {
            this.value = to;
        }
    }

    @Override
    public String toString() {
        return this.value2string() + " = " + operatorName + " " + from.printIRType() + " " +
                value.value2string() + " to " + target.printIRType();
    }

    @Override
    public String myHash() {
        return this.value.value2string() + " " + this.from.printIRType() + " " + this.target.printIRType();
    }

    @Override
    public Value simplify() {
        if (isRemoved) {
            return this;
        }
        IRConst simplified = null;
        if (this.value instanceof IRConst constVal) {
            if (this.target instanceof TInt) {
                if (this.from instanceof TFloat) {
                    simplified = new IntConst((int) constVal.getConstVal().floatValue());
                } else {
                    simplified = new IntConst(constVal.getConstVal().intValue());
                }
            } else if (this.target instanceof TChar) {
                simplified = new CharConst(constVal.getConstVal().intValue());
            } else if (this.target instanceof TBool) {
                simplified = new BoolConst(constVal.getConstVal().intValue());
            } else if (this.target instanceof TFloat) {
                simplified = new FloatConst(constVal.getConstVal().intValue());
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
