package ir.instr;

import ir.IrInstr;
import ir.value.BasicBlock;
import ir.value.Value;
import utils.Panic;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

// 终结指令
public class TermInstr extends IrInstr {
    //RET, CALL, BR, BR_COND
    private Value value;
    private Value cond;
    private Value elseValue;
    public OpCode op;
    public boolean isVoid;
    private boolean isCond;
    private List<Value> arguments = List.of();

    @Override
    public IrInstr copy() {
        switch (op) {
            case RET: {
                if (isVoid) {
                    return new TermInstr(op);
                } else {
                    return new TermInstr(value, op);
                }
            }
            case CALL:
                return new TermInstr(value, new ArrayList<>(arguments), op, elseValue);
            case BR:
                if (isCond) {
                    return new TermInstr(cond, (BasicBlock) value, (BasicBlock) elseValue, op);
                } else {
                    return new TermInstr((BasicBlock) value, op);
                }
            default:
                Panic.panic("Unknown op code");
                return null;
        }
    }

    public TermInstr(OpCode op) {
        Panic.panicIfFalse(op == OpCode.RET);
        this.op = op;
        this.isVoid = true;
        this.value = null;
    }

    public void resetIntermediate(Value check, Value value) {
        if (this.value == null) {
            return ;
        }
        if (this.value.getName().equals(check.getName())) {
            this.value = value;
        }
    }

    // ret
    public TermInstr(Value value, OpCode op) {
        Panic.panicIfFalse(op == OpCode.RET);
        Panic.panicIfFalse(value != null);
        this.op = op;
        this.value = value;
        this.isVoid = false;
    }

    // call
    public TermInstr(Value value, List<Value> arguments, OpCode op, Value result) {
        // value: funcName | elseValue: return Type
        Panic.panicIfFalse(op == OpCode.CALL);
        this.value = value;
        this.elseValue = result;
        this.arguments = arguments;
        this.op = op;
    }

    // br
    public TermInstr(BasicBlock dest, OpCode op) {
        Panic.panicIfFalse(op == OpCode.BR);
        Panic.panicIfFalse(dest != null);
        this.value = dest;
        this.isCond = false;
        this.op = op;
    }

    // br_cond
    public TermInstr(Value cond, BasicBlock thenDest, BasicBlock elseDest, OpCode op) {
        Panic.panicIfFalse(op == OpCode.BR);
        this.cond = cond;
        this.value = thenDest;
        this.elseValue = elseDest;
        this.isCond = true;
        this.op = op;
    }

    public Value checkJumpAndGetTargetBlock() {
        if (op == OpCode.BR && !isCond) {
            return value;
        }
        return null;
    }

    public BasicBlock[] getJumpTargets() {
        if (op == OpCode.BR) {
            if (isCond) {
                return new BasicBlock[]{(BasicBlock) value, (BasicBlock) elseValue};
            } else {
                return new BasicBlock[]{(BasicBlock) value};
            }
        }
        return new BasicBlock[]{};
    }

    public IrInstr resetBrAfterResetBlock(HashSet<String> removeBlockNames) {
        if (op != OpCode.BR) {
            return null;
        }
        if (isCond) {
            if (toolCheckIfNameEqual(value.getName(),removeBlockNames) && toolCheckIfNameEqual(elseValue.getName(),removeBlockNames)) {
                return null;
            } else if (toolCheckIfNameEqual(value.getName(),removeBlockNames)) {
                return new TermInstr((BasicBlock) elseValue, OpCode.BR);
            } else if (toolCheckIfNameEqual(elseValue.getName(),removeBlockNames)) {
                return new TermInstr((BasicBlock) value, OpCode.BR);
            } else {
                return this;
            }
        } else {
            if (toolCheckIfNameEqual(value.getName(),removeBlockNames)) {
                return null;
            } else {
                return this;
            }
        }
    }

    private boolean toolCheckIfNameEqual(String b, HashSet<String> a) {
        for (String s : a) {
            if (s.equals(b)) {
                return true;
            }
        }
        return false;
    }

    public Value getValue() {
        return isVoid ? null : value;
    }

    private String getTypeString(Value value) {
        if (isVoid || value == null) {
            return "void";
        }
        return value.getType().toString();
    }

    public String getOpType() {
        return op.toString().toLowerCase();
    }

    public OpCode getOp() {
        return op;
    }

    public boolean checkIfCond() {
        return isCond;
    }

    public List<Value> getArguments() {
        return arguments;
    }

    @Override
    public OpCode getOpCode() {
        return op;
    }


    @Override
    public Value getInitialValue() {
        if (op == OpCode.CALL) {
            return elseValue;
        }
        return null;
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        switch (op) {
            case RET:
                return new ArrayList<Value>() {{
                    add(value);
                }};
            case CALL:
                return new ArrayList<Value>() {{
                    add(value);
                    addAll(arguments);
                }};
            case BR:
                if (this.isCond) {
                    return new ArrayList<Value>() {{
                        add(cond);
                    }};
                }
                else {
                    return new ArrayList<>() {
                        {
                            add(value);
                        }
                    };
                }
            default:
                return null;
        }
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        switch (op) {
            case RET:
                sb.append("ret ").append(getTypeString(value)).append(isVoid ? "" : (" " + value.getName()));
                break;
            case CALL:
                if (elseValue != null) {
                    sb.append(elseValue.getName()).append(" = ");
                }
                sb.append("call ").append(getTypeString(elseValue)).append(" ").append(value.getName()).append("(");
                for (int i = 0; i < arguments.size(); i++) {
                    sb.append(arguments.get(i).getType()).append(" ").append(arguments.get(i).getName());
                    if (i != arguments.size() - 1) {
                        sb.append(", ");
                    }
                }
                sb.append(")");
                break;
            case BR:
                if (isCond) {
                    sb.append("br i1 ").append(cond.getName()).append(", label %").append(value.getName()).append(", label %").append(elseValue.getName());
                }else {
                    sb.append("br label %").append(value.getName());
                }
                break;
            default:
                return "unknown";
        }
        return sb.toString();
    }

    public Value[] getOperands() {
        Value[] res = new Value[3 + arguments.size()];
        res[0] = elseValue;
        res[1] = value;
        res[2] = cond;
        for (int i = 0; i < arguments.size(); i++) {
            res[i + 3] = arguments.get(i);
        }
        return res;
    }

    public void replaceOperand(Value old, Value newv) {
        if (value != null && value.getName().equals(old.getName())) {
            value = newv;
        }
        if (cond != null && cond.getName().equals(old.getName())) {
            cond = newv;
        }
        if (elseValue != null && elseValue.getName().equals(old.getName())) {
            elseValue = newv;
        }
        for (int i = 0; i < arguments.size(); i++) {
            if (arguments.get(i).getName().equals(old.getName())) {
                arguments.set(i, newv);
            }
        }
    }
}
