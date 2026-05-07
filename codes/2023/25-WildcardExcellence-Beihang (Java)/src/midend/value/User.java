package midend.value;

import midend.Type;
import midend.Use;
import util.nodelist.NodeList;

import java.util.ArrayList;

public class User extends Value {

    private ArrayList<Value> operandList; // 记录该User使用的Value
//    private NodeList<Use> useOpList; // 记录该User使用的Value，use形式，和used value中存储的指向同一对象

    public void addUseOp(Value usedValue, int idx) {
        Use use = new Use(this, idx);
//        this.useOpList.addLast(use);
        usedValue.addUsedOp(use);
    }

    public ArrayList<Value> getOperandList() {
        return operandList;
    }

    public void setOperandList(ArrayList<Value> operandList) {
        this.operandList = operandList;
    }

    public User(Type type, String name) {
        super(type, name);
        operandList = new ArrayList<>();
    }
}
