package backend.ir.entity;

import frontend.semantic.symbol.IRBasicType;

import java.util.ArrayList;
import java.util.List;

/**
 * &#064;Classname User
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:18
 * &#064;Created MuJue
 */
public abstract class User extends Value {
    protected List<Value> usees = new ArrayList<>();

    public User(String name, IRBasicType IRBasicType) {
        super(name, IRBasicType);
    }
    public User(String name, IRBasicType IRBasicType, Boolean isConst, Boolean isArray, List<Integer> dim){
        super(name, IRBasicType, isConst, isArray, dim);
    }

    /**
     * 添加使用的操作数
     * @param usee 使用的操作数
     * @param cnt 添加的个数
     */
    public void addUsee(Value usee, int... cnt) {
        int size = cnt.length==0? 1:cnt[0];
        for(int i=0; i<size; i++){
            usees.add(usee);
            if(usee!=null){
                usee.addUser(this);
            }
        }
    }
    public void addUseeAt(Value usee, int idx){
        usees.add(idx, usee);
        if(usee!=null){
            usee.addUser(this);
        }
    }

    /**
     * 移除使用的操作数
     */
    public void removeUsee(Value usee, int... cnt) {
        int size = cnt.length==0? 1:cnt[0];
        for(int i=0; i<size; i++){
            usees.remove(usee);
            if(usee!=null){
                usee.removeUser(this);
            }
        }
    }

    /**
     * 删除所有该User中所有使用的操作数
     */
    public void deleteAllUsees(){
        for(Value usee:usees){
            if(usee!=null){
                usee.getUsers().remove(this);
            }
        }
        usees.clear();
    }

    public List<Value> getUsees() {
        return usees;
    }

    public Value getUsee(int index) {
        return usees.get(index);
    }

    /**
     * 替换value
     * @param oldValue 老的value
     * @param newValue 新的value
     */
    public void replaceValue(Value oldValue, Value newValue){
        if(oldValue!=null){
            if(newValue!=null && usees.contains(oldValue))
                usees.set(usees.indexOf(oldValue),newValue);
            else usees.remove(oldValue);
        }
        replaceUser(oldValue,newValue,this);
    }

    /**
     * 将自己使用的数据全部增加到newUser下面
     * 仅用于删除只有跳转指令的基本块
     */
    public void replaceAllUsees(User newUser){
        List<Value> newUsees = new ArrayList<>(usees);
        for(Value usee:newUsees){
            if(usee!=newUser){
                newUser.addUsee(usee);
                this.removeUsee(usee);
            }
        }
    }
}
