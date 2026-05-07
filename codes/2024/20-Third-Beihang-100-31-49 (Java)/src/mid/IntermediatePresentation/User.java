package mid.IntermediatePresentation;

import java.util.ArrayList;

public class User extends Value {
    protected ArrayList<Value> operandList = new ArrayList<>();

    public User(String regName, ValueType VType) {
        super(regName, VType);
    }

    public void use(Value value) {
        operandList.add(value);
        value.usedBy(this);
    }

    public void removeOperand(Value value) {
        operandList.remove(value);
        value.removeUser(this);
    }

    public ArrayList<Value> getOperandList() {
        return operandList;
    }

    public BasicBlock getBlock() {
        return null;
    }

    public void setOperandList(ArrayList<Value> operandList) {
        this.operandList = operandList;
    }

    public void replaceOperand(Value oldValue, Value newValue) {
        for (int i = 0; i < operandList.size(); i++) {
            if (operandList.get(i).equals(oldValue)) {
                operandList.set(i, newValue);
            }
        }
        oldValue.removeUser(this);
        newValue.usedBy(this);
    }


    public void destroy() {
        ArrayList<User> users = new ArrayList<>(userList);
        for (User user : new ArrayList<>(users)) {
            user.removeOperand(this);
        }
        operandList.forEach(o -> o.removeUser(this));
        operandList.clear();
    }
}
