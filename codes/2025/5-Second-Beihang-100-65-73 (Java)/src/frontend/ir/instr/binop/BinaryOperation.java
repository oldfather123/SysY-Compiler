package frontend.ir.instr.binop;

import frontend.ir.Value;
import frontend.ir.constant.ConstHandler;
import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.Type;
import frontend.ir.structure.BasicBlock;
import midend.ConstFold;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Objects;

public abstract class BinaryOperation extends Instruction {
    private Value operand1;
    private Value operand2;
    private final String operatorName;

    public BinaryOperation(Type type, int regIdx, Value op1, Value op2, String opName, BasicBlock parentBB) {
        super(type, regIdx, parentBB);
        this.operand1 = op1;
        this.operand2 = op2;
        this.operatorName = opName;
        if (parentBB.isNotClosed()) {
            this.setUse(op1);
            this.setUse(op2);
        }
    }

    public Value getOperand1() {
        return operand1;
    }

    public Value getOperand2() {
        return operand2;
    }

    protected void setOperand1(Value operand1) {
        this.operand1 = operand1;
    }

    protected void setOperand2(Value operand2) {
        this.operand2 = operand2;
    }

    protected abstract boolean isSwappable();
    
    public String getOperatorName() {
        return operatorName;
    }
    
    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(operand1);
        operands.add(operand2);
        return operands;
    }

    public void swapOperands() {
        Value temp = operand1;
        operand1 = operand2;
        operand2 = temp;
    }

    @Override
    public String myHash() {
        if (this.isSwappable()) {
            boolean bothIns = operand1 instanceof Instruction && operand2 instanceof Instruction;
            boolean outOfOrder;
            if (bothIns) {
                outOfOrder = operand1.value2string().compareTo(operand2.value2string()) > 0;
            } else {
                outOfOrder = !(operand1 instanceof Instruction) && operand2 instanceof Instruction;
            }
            if (outOfOrder) {
                return operand2.value2string() + operand1.value2string() + operatorName;
            }
        }
        return operand1.value2string() + operand2.value2string() + operatorName;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (this.operand1 == from) {
            operand1 = to;
        }
        if (this.operand2 == from) {
            operand2 = to;
        }
    }

    @Override
    public String toString() {
        return this.value2string() + " = " + operatorName + " " + this.getType().printIRType() +
                " " + operand1.value2string() + ", " + operand2.value2string();
    }


    protected <T, A extends BinaryOperation, S extends BinaryOperation> Value simplifyForAdd(
            ConstHandler<T> handler,
            Class<A> addClass,
            Class<S> subClass) {

        Value simplified = null;
        Value op1 = this.getOperand1();
        Value op2 = this.getOperand2();

        if (op1 instanceof IRConst con1 && op2 instanceof IRConst con2) {
            T v1 = handler.getValue(con1);
            T v2 = handler.getValue(con2);
            simplified = handler.makeConst(handler.add(v1, v2));
        } else if (op1 instanceof IRConst con1) {
            T v1 = handler.getValue(con1);
            if (handler.isZero(v1)) {
                simplified = op2;
            } else {
                simplified = mergeConstForAdd(handler, con1, op2, addClass, subClass);
            }
        } else if (op2 instanceof IRConst con2) {
            T v2 = handler.getValue(con2);
            if (handler.isZero(v2)) {
                simplified = op1;
            } else {
                simplified = mergeConstForAdd(handler, con2, op1, addClass, subClass);
            }
        } else if (op1 instanceof Instruction i1 && op2 instanceof Instruction i2) {
            if (subClass.isInstance(i1)) {
                S sub = subClass.cast(i1);
                if (sub.getOperand2() == i2) {
                    simplified = sub.getOperand1();
                } else if (sub.getOperand1() instanceof IRConst con1) {
                    T v1 = handler.getValue(con1);
                    if (handler.isZero(v1)) {
                        SubInstr newSub = new SubInstr(
                                this.getRegIndex(), i2, sub.getOperand2(), sub.getParentBB());
                        newSub.setUse(i2);
                        newSub.setUse(sub.getOperand2());
                        newSub.insertAfter(this);
                        simplified = newSub;
                    }
                }
            } else if (subClass.isInstance(i2)) {
                S sub = subClass.cast(i2);
                if (sub.getOperand2() == i1) {
                    simplified = sub.getOperand1();
                } else if (sub.getOperand1() instanceof IRConst con1) {
                    T v1 = handler.getValue(con1);
                    if (handler.isZero(v1)) {
                        SubInstr newSub = new SubInstr(
                                this.getRegIndex(), i1, sub.getOperand2(), sub.getParentBB());
                        newSub.setUse(i1);
                        newSub.setUse(sub.getOperand2());
                        newSub.insertAfter(this);
                        simplified = newSub;
                    }
                }
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


    private <T, A extends BinaryOperation, S extends BinaryOperation> Value mergeConstForAdd(
            ConstHandler<T> handler,
            IRConst con,
            Value nonCon,
            Class<A> addClass,
            Class<S> subClass) {

        if (addClass.isInstance(nonCon)) {
            A add = addClass.cast(nonCon);
            if (add.getOperand1() instanceof IRConst con1) {
                T res = handler.add(handler.getValue(con1), handler.getValue(con));
                IRConst resConst = handler.makeConst(res);
                this.modifyUseV2(con, resConst);
                this.modifyUseV2(nonCon, add.getOperand2());
                if (add.getUserList().isEmpty()) {
                    add.removeFromList();
                }
                this.simplify();
            } else if (add.getOperand2() instanceof IRConst con2) {
                T res = handler.add(handler.getValue(con2), handler.getValue(con));
                IRConst resConst = handler.makeConst(res);
                this.modifyUseV2(con, resConst);
                this.modifyUseV2(nonCon, add.getOperand1());
                if (add.getUserList().isEmpty()) {
                    add.removeFromList();
                }
                this.simplify();
            }
        } else if (subClass.isInstance(nonCon)) {
            S sub = subClass.cast(nonCon);
            if (sub.getOperand1() instanceof IRConst con1) {
                T res = handler.add(handler.getValue(con1), handler.getValue(con));
                IRConst resConst = handler.makeConst(res);
                Instruction newSub = new SubInstr(
                        this.getRegIndex(), resConst, sub.getOperand2(), sub.getParentBB());
                newSub.setUse(resConst);
                newSub.setUse(sub.getOperand2());
                newSub.insertAfter(this);
                return newSub.simplify();
            } else if (sub.getOperand2() instanceof IRConst con2) {
                T res = handler.sub(handler.getValue(con), handler.getValue(con2));
                IRConst resConst = handler.makeConst(res);
                this.modifyUseV2(con, resConst);
                this.modifyUseV2(nonCon, sub.getOperand1());
                if (sub.getUserList().isEmpty()) {
                    sub.removeFromList();
                }
                this.simplify();
            }
        }

        return null;
    }

    protected <T, A extends BinaryOperation, S extends BinaryOperation> Value simplifyForSub(
            ConstHandler<T> handler,
            Class<A> addClass,
            Class<S> subClass) {

        Value simplified = null;
        Value op1 = this.getOperand1();
        Value op2 = this.getOperand2();

        if (op1 instanceof IRConst con1 && op2 instanceof IRConst con2) {
            T result = handler.sub(handler.getValue(con1), handler.getValue(con2));
            simplified = handler.makeConst(result);
        } else if (op2 instanceof IRConst con2) {
            T val = handler.getValue(con2);
            if (handler.isZero(val)) {
                simplified = op1;
            } else if (addClass.isInstance(op1)) {
                A add = addClass.cast(op1);
                if (add.getOperand1() instanceof IRConst con) {
                    T res = handler.sub(val, handler.getValue(con));
                    IRConst resConst = handler.makeConst(res);
                    this.modifyUseV2(con2, resConst);
                    this.modifyUseV2(add, add.getOperand2());
                    if (add.getUserList().isEmpty()) {
                        add.removeFromList();
                    }
                    this.simplify();
                } else if (add.getOperand2() instanceof IRConst con) {
                    T res = handler.sub(val, handler.getValue(con));
                    IRConst resConst = handler.makeConst(res);
                    this.modifyUseV2(con2, resConst);
                    this.modifyUseV2(add, add.getOperand1());
                    if (add.getUserList().isEmpty()) {
                        add.removeFromList();
                    }
                    this.simplify();
                }

            } else if (subClass.isInstance(op1)) {
                S sub = subClass.cast(op1);
                if (sub.getOperand1() instanceof IRConst con) {
                    T res = handler.sub(handler.getValue(con), val);
                    IRConst resConst = handler.makeConst(res);
                    this.modifyUseV2(con2, resConst);
                    this.modifyUseV2(sub, sub.getOperand2());
                    this.swapOperands();
                    if (sub.getUserList().isEmpty()) {
                        sub.removeFromList();
                    }
                    this.simplify();
                } else if (sub.getOperand2() instanceof IRConst con) {
                    T res = handler.add(handler.getValue(con), val);
                    IRConst resConst = handler.makeConst(res);
                    this.modifyUseV2(con2, resConst);
                    this.modifyUseV2(sub, sub.getOperand1());
                    if (sub.getUserList().isEmpty()) {
                        sub.removeFromList();
                    }
                    this.simplify();
                }
            } else {
                T neg = handler.neg(handler.getValue(con2));
                Value negConst = handler.makeConst(neg);
                try {
                    A newAdd = addClass.getConstructor(int.class, Value.class, Value.class, BasicBlock.class)
                            .newInstance(this.getRegIndex(), op1, negConst, this.getParentBB());
                    newAdd.setUse(op1);
                    newAdd.setUse(negConst);
                    newAdd.insertAfter(this);
                    simplified = newAdd;
                } catch (Exception e) {
                    throw new RuntimeException("Failed to create Add instruction for simplifyForSub", e);
                }
            }

        } else if (op1 instanceof IRConst con1) {
            if (addClass.isInstance(op2)) {
                A add = addClass.cast(op2);
                if (add.getOperand1() instanceof IRConst con) {
                    T res = handler.sub(handler.getValue(con1), handler.getValue(con));
                    IRConst resConst = handler.makeConst(res);
                    this.modifyUseV2(con1, resConst);
                    this.modifyUseV2(add, add.getOperand2());
                    if (add.getUserList().isEmpty()) {
                        add.removeFromList();
                    }
                    this.simplify();
                } else if (add.getOperand2() instanceof IRConst con) {
                    T res = handler.sub(handler.getValue(con1), handler.getValue(con));
                    IRConst resConst = handler.makeConst(res);
                    this.modifyUseV2(con1, resConst);
                    this.modifyUseV2(add, add.getOperand1());
                    if (add.getUserList().isEmpty()) {
                        add.removeFromList();
                    }
                    this.simplify();
                }
            } else if (subClass.isInstance(op2)) {
                // %2 = 2 - %1
                S sub = subClass.cast(op2);
                if (sub.getOperand1() instanceof IRConst con) {
                    // %1 = 1 - %0
                    // %2 = 2 - %1
                    // %2 = 1 + %0
                    T res = handler.sub(handler.getValue(con1), handler.getValue(con));
                    IRConst resConst = handler.makeConst(res);
                    try {
                        A newAdd = addClass.getConstructor(int.class, Value.class, Value.class, BasicBlock.class)
                                .newInstance(this.getRegIndex(), sub.getOperand2(), resConst, this.getParentBB());
                        newAdd.setUse(sub.getOperand2());
                        newAdd.setUse(resConst);
                        newAdd.insertAfter(this);
                        simplified = newAdd.simplify();
                    } catch (Exception e) {
                        throw new RuntimeException("Failed to create Add instruction in simplifyForSub", e);
                    }
                } else if (sub.getOperand2() instanceof IRConst con) {
                    // %1 = %0 - 1
                    // %2 = 2 - %1
                    // -> %2 = 3 - %0
                    T res = handler.add(handler.getValue(con1), handler.getValue(con));
                    IRConst resConst = handler.makeConst(res);
                    this.modifyUseV2(con1, resConst);
                    this.modifyUseV2(sub, sub.getOperand1());
                    if (sub.getUserList().isEmpty()) {
                        sub.removeFromList();
                    }
                    this.simplify();
                }

            }

        } else if (op1 instanceof Instruction i1 && op2 instanceof Instruction i2) {
            if (Objects.equals(i1.getRegIndex(), i2.getRegIndex())) {
                simplified = handler.makeZero();
            } else if (addClass.isInstance(i1)) {
                A add = addClass.cast(i1);
                if (add.getOperand1() == i2) {
                    simplified = add.getOperand2();
                } else if (add.getOperand2() == i2) {
                    simplified = add.getOperand1();
                }
            } else if (addClass.isInstance(i2)) {
                A add = addClass.cast(i2);
                if (add.getOperand1() == i1) {
                    this.modifyUseV2(i1, IntConst.Zero);
                    this.modifyUseV2(i2, add.getOperand2());
                } else if (add.getOperand2() == i1) {
                    this.modifyUseV2(i1, IntConst.Zero);
                    this.modifyUseV2(i2, add.getOperand1());
                }
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
