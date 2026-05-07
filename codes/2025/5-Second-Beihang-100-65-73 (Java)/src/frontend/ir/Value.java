package frontend.ir;

import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.Type;
import util.CustomList;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

public abstract class Value extends CustomList.Node {
    private Type type;
    private final List<Value> usedList; // to store those who used by this
    private final List<Value> userList; // to store those who uses this
    private static int vid = 0; // 给load/store优化准备的hash计数器
    private final int id; // 给load/store优化准备的hash

    protected Value(Type type) {
        this.type = type;
        this.usedList = new LinkedList<>();
        this.userList = new LinkedList<>();
        this.id = vid++;
    }

    public void setType(Type type) {
        this.type = type;
    }

    /**
     * add a used value for this
     */
    public void setUse(Value usedVal) {
        this.usedList.add(usedVal);
        usedVal.userList.add(this);
    }

    /**
     * 不再使用某value
     */
    public void delUse(Value usedVal) {
        this.usedList.remove(usedVal);
        usedVal.userList.remove(this);
    }

    /**
     * 把对 this 的使用换成对其它 Value 的
     */
    public void replaceUseWith(Value newVal) {
        for (Value user : new ArrayList<>(userList)) {
            user.modifyUse(this, newVal);
            newVal.userList.add(user);
        }
        userList.clear();
    }

    /**
     * 把 this 使用的一个值换成另一个
     */
    public void modifyUse(Value from, Value to) {
        if (!this.usedList.contains(from)) {
            throw new RuntimeException("no such value used");
        }
        this.usedList.remove(from);
        this.usedList.add(to);
    }

    public Type getType() {
        return type;
    }

    public List<Value> getUsedList() {
        return usedList;
    }

    public List<Value> getUserList() {
        return userList;
    }

    public abstract String value2string();

    /**
     * 逐个检查use是否被prev支配
     */
    public boolean isDominatedAllUsesBy(Instruction def) {
        for (Value user : userList) {
            if (!(user instanceof Instruction userInstr && def.dominates(userInstr))) return false;
        }
        return true;
    }

    /**
     * 给load/store优化准备的专用hash。注意：constInt很特殊
     */
    public int getId() {
        return id;
    }
}
