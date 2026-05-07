package mid.IntermediatePresentation;

import mid.IntermediatePresentation.Array.ArrayInitializer;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Instruction.Fptosi;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.Sitofp;
import mid.IntermediatePresentation.Instruction.ZextTo;

import java.util.ArrayList;
import java.util.HashMap;

public class Value {
    protected ArrayList<User> userList = new ArrayList<>();
    protected ValueType vType;

    protected String reg;

    public Value(String reg, ValueType vType) {
        this.reg = reg;
        this.vType = vType;
    }

    public void usedBy(User value) {
        if (!userList.contains(value)) {
            userList.add(value);
        }
    }

    public String getReg() {
        return reg;
    }

    public String toString() {
        return reg;
    }

    public ValueType getType() {
        return vType;
    }

    public ValueType getRefType() {
        return vType.getRefType();
    }

    public boolean isPointer() {
        return vType.isPointer();
    }

    public String getTypeString() {
        return vType.toString();
    }

    public String getName() {
        return reg.substring(1);
    }

    public ArrayList<User> getUserList() {
        return new ArrayList<>(userList);
    }

    public void removeUser(User user) {
        userList.remove(user);
    }

    public void beReplacedBy(Value v) {
        if (this.equals(v)) {
            return;
        }
        for (User user : userList) {
            ArrayList<Value> newOperandList = new ArrayList<>(user.operandList);
            for (int i = 0; i < user.operandList.size(); i++) {
                if (user.operandList.get(i).equals(this)) {
                    if (v instanceof ConstNumber n) {
                        v = n.withType(!user.operandList.get(i).isFloat());
                    }
                    newOperandList.set(i, v);
                }
            }
            user.setOperandList(newOperandList);
            v.usedBy(user);
        }
    }

    public boolean isUseless() {
        return userList.size() == 0;
    }

    public Value withType(boolean isInt) {
        Value ret = this;
        if (vType == ValueType.NULL) {
            return this;
        }
        if (ret instanceof ConstNumber constNumber) {
            String val = constNumber.getReg();
            boolean isIntString = !val.startsWith("0x");
            if (isInt && !isIntString) {
                val = Integer.toString(constNumber.getVal().intValue());
            } else if (!isInt && isIntString) {
                val = String.format("0x%x", Double.doubleToRawLongBits(Float.parseFloat(reg + ".0")));
            }
            Value nValue;
            if (isInt) {
                nValue = new ConstNumber(Integer.parseInt(val));
            } else {
                nValue = new ConstNumber(Double.longBitsToDouble(
                        Long.parseUnsignedLong(val.substring(2), 16)));
            }
            return nValue;
        }

        if (ret instanceof ArrayInitializer ainit) {
            ret = new ArrayInitializer(new ArrayList<>(), ainit.getLen());
            HashMap<Integer, Value> initVals = ainit.getVals();
            for (Integer i : initVals.keySet()) {
                ((ArrayInitializer) ret).add(i, initVals.get(i).withType(isInt));
            }
            return ret;
        }

        if (ret.isPointer()) {
            ret = new Load(IRManager.getInstance().declareTempVar(), ret);
        }

        if (isInt && ret.vType == ValueType.I32 || !isInt && ret.vType == ValueType.FLT) {
            return ret;
        } else if (isInt) {
            if (ret.vType == ValueType.I1) {
                return new ZextTo(ret, ValueType.I32);
            } else {
                return new Fptosi(ret);
            }
        } else {
            if (vType == ValueType.I1) {
                return new Sitofp(new ZextTo(ret, ValueType.I32));
            } else {
                return new Sitofp(ret);
            }
        }
    }

    public Value toI1() {
        Value ret = this;
        if (ret.isPointer()) {
            ret = new Load(IRManager.getInstance().declareTempVar(), ret);
        }

        if (ret.vType == ValueType.I1) {
            return ret;
        } else {
            return new Cmp("!=", (new ConstNumber(0)).withType(ret.vType == ValueType.I32),
                    ret);
        }
    }

    public boolean isFloat() {
        return vType == ValueType.FLT || vType == ValueType.PFLT ||
                vType.equals(ValueType.ARRAY) && vType.getRefType() == ValueType.FLT;
    }

    public int hashCode() {
        return reg.hashCode();
    }
}
