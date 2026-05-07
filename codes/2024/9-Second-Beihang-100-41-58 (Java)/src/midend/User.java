package midend;

import midend.LLVMType.Type;

import java.util.ArrayList;

public class User extends Value {
    private ArrayList<Value> valueList;
    public User(Type type, String name) {
        super(type, name);
        valueList = new ArrayList<>();
    }

    public void addValue(Value value) {
        valueList.add(value);
        if (value != null) {
            value.addUse(new Use(value, this));
        }
    }

    public Value getValue(int count) {
        return valueList.get(count);
    }

    public ArrayList<Value> getValueList() {
        return this.valueList;
    }

    public void setValue(int pos, Value value) {
        if (pos >= valueList.size()) {
            return;
        }
        Value before = valueList.get(pos);
        if (before != null && before != value) {
            before.removeUserFromUse(this);
        }
        this.valueList.set(pos, value);
        if (value != null && before != value) {
            value.addUse(new Use(value, this));
        }
    }

    public void removeUseOfValue() {
        if (valueList.isEmpty()) {
            return;
        }
        for (Value value : valueList) {
            if (value != null) {
                value.removeUserFromUse(this);
            }
        }
    }
}
