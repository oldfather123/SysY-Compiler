package midend.value.instrs;

import midend.Type;
import midend.Use;
import midend.value.BasicBlock;
import midend.value.Constant;
import midend.value.User;
import midend.value.Value;

public class Instr extends User {
    public int number = -1; // 寄存器分配时用

    public BasicBlock getParent() {
        return parent;
    }

    public String getHash() {
        String old = toString();
        return old.substring(old.indexOf(" = ") + 3);
    }

    private BasicBlock parent;

    public Value I1toI32(Value opValue, BasicBlock parent) {
        if (opValue instanceof Constant.ConstantInteger) {
            return new Constant.ConstantInteger(Type.IntegerType.I32, opValue.getName());
        }
        return new Zext(opValue, Type.IntegerType.I32, parent);
    }

    public Value I32toF(Value opValue, BasicBlock parent) {
        if (((Type.IntegerType) opValue.getType()).isI1()) {
            opValue = I1toI32(opValue, parent);
        }
        if (opValue instanceof Constant.ConstantInteger) {
            return new Constant.ConstantFloat(new Type.FloatType(), (float) (((Constant.ConstantInteger) opValue).getConstantIntegerValue()));
        }
        return new ItoF(opValue, parent);
    }

    public Value FtoI32(Value opValue, BasicBlock parent) {
        if (opValue instanceof Constant.ConstantFloat) {
            return new Constant.ConstantInteger(Type.IntegerType.I32, String.valueOf((int)((Constant.ConstantFloat) opValue).getConstantFloatValue()));
        }
        return new FtoI(opValue, parent);
    }

    public void remove() {
        removeFromBlock();
        removeUse();
    }

    public void removeFromBlock() {
        this.parent.getInstrList().removeIf(it -> it == this);
    }

    public void removeUse() {
        // 将 usedOpList 清空
        this.getUsedOpList().clear();
        // 将被使用者记录的 use 删去
        for (Value value : getOperandList()) {
            value.removeUseFromUsedList(this);
        }
    }

    public void changeUseIn(Value now, int index) {
        this.getOperandList().get(index).removeUseFromUsedList(this);
        this.getOperandList().set(index, now);
        if (this instanceof Phi) {
            if (now.getType() instanceof Type.FloatType) {
                setType(new Type.FloatType());
            } else if (now.getType() instanceof Type.IntegerType && !(getType() instanceof Type.FloatType)){
                setType(now.getType());
            }
        }
        Use use = new Use(this, index);
        now.addUsedOp(use);
    }

    public Instr() {
        super(null, null);
    }

    public Instr(BasicBlock parent) {
        super(parent.getType(), "");
        this.parent = parent;
        if (this instanceof Alloc) {
            //如果是alloc或者phi则将其插入当前函数的第一个基本块的头部
            this.parent.getParent().getFirstBlock().getInstrList().addFirst(this);
            this.parent = this.parent.getParent().getFirstBlock();
        } else if (this instanceof Phi) {
            this.parent.getInstrList().addFirst(this);
        } else if (!(this instanceof Store || this instanceof BinaryOp || this instanceof Return
        || this instanceof Call || this instanceof CallVoid)){
            this.parent.addInstr(this);
        }
    }
}

