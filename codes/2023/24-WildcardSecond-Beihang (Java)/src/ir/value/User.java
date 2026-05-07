package ir.value;

import ir.Value;
import ir.instr.GetElementPtrInst;
import ir.type.Type;

import java.util.LinkedHashMap;
import java.util.LinkedList;

public class User extends Value {
    // 联系每一个User对应的Value List,我的理解是主要用途应在Instruction继承以后对操作数的控制
    // 联系 User 与 Value 的 Use list,是乱序的，顺序通过 Use 的成员变量 operandRank 来体现
    // private int numOP = 0;//操作数的数量
    private LinkedList<Value> operands;

    public User(LinkedList<User> users,  Type type) {
        super(users, type);
        //this.numOP = numOp;
    }

    public User(Type type) {
        super(new LinkedList<>(), type);
    }

    public User(User user, Type type, String name) {
        super(user, type,name);
    }

    public User(LinkedList<Value> uses, Type type, String name) {
        super(type, name);
        operands = uses;
        for(var sub : uses){
            if(sub!=null){
                sub.addUser(this);
            }
        }
    }
    //对value出边进行遍历，返回序号为i的value
    public Value getOP(int i) {
        return operands.get(i);
    }

    //加边操作，新生成一个Use
    public void addOperand(Value v) {
        operands.add(v);
        v.addUser(this);
    }

    public void addOperand(int index, Value v){
        operands.add(index, v);
        v.addUser(this);
    }

    //将序号为i的边的终点value进行替换，如果不存在则创建新的对象
    public void setOperand(int i, Value v) {
        operands.set(i, v);
    }

//    public int getNumOP() {
//        return numOP;
//    }

    public void addValue(Value v) {
        this.operands.add(v);
    }

    public LinkedList<Value> getOperands() {
        return operands;
    }

    public void clearOperands() {
        operands.clear();
    }

    public void delete(){
        for(Value v : operands){
            v.getUsers().remove(this);
        }
    }
    public void replaceOp(Value old, Value newop){
        for(int i = 0;i<operands.size();i++){
            if(operands.get(i).equals(old)){
                old.getUsers().remove(this);
                operands.set(i, newop);
                newop.addUser(this);
            }
        }
    }

    @Override
    public void cloneTo(Value target) throws CloneNotSupportedException {
        super.cloneTo(target);
        User userTarget = (User)target;
        userTarget.operands = new LinkedList<>(this.operands);
    }

    @Override
    public void afterclone(LinkedHashMap<Value, Value> valueMap) {
        super.afterclone(valueMap);
        LinkedList<Value> newOperands = new LinkedList<>();
        for(var oper : this.operands){
            newOperands.add(valueMap.get(oper));
        }
        this.operands = newOperands;
    }
}