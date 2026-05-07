package ir;

import ir.instr.*;
import ir.type.IntType;
import ir.type.Type;
import ir.value.ConstNumber;
import ir.value.User;
import ir.value.Variable;
import ir.value.user.Function;
import tools.ErrOutput;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedList;

public class Value {
    //保存都有什么东西用到了这个Value
    public LinkedList<User> users;
    public Type type;
    public String name;

    private boolean settedBound = false;
    public  int upperBound;
    public  int lowerBound;
    public Value(LinkedList<User> users, Type type) {
        this(users, type, "");
    }

    public Value(LinkedList<User> users, Type type, String name) {
        this.type = type;
        this.users = users;
        this.name = name;
    }

    public Value(Type type, String name){
        this.type = type;
        this.name = name;
        this.users = new LinkedList<User>();
    }

    public Value(User user, Type type, String name){
        this(new LinkedList<>(), type, name);
        this.users.add(user);
    }

    public Value(User user, String name) {
        this(new LinkedList<>(), null, name);
        this.users.add(user);
    }

    public boolean needName = true;

    //将所有对this的使用换为对v的使用
//    public void replaceAllUseWith(Value v) {
//        for (User use : users) {
//            use.setV(v);
//            v.uses.add(use);
//        }
//    }

    public void addUser(User u) {
        if(this.users.contains(u)){
            return;
        }
        this.users.add(u);
    }

    public Value(){
        this.users = new LinkedList<>();
    }

    public String getName() {
        return name;
    }
    public String getNameWithType(){
        return type.toString()+' '+getFullName();
    }

    public String getFullName() {
        return "%"+name;
    }

    public boolean hasName(){
        return true;
    }

    public LinkedList<User> getUsers() {
        return users;
    }


    public boolean checkUsers(){
        for (User user : users) {
            if (user instanceof Load) return false;
            if (user instanceof Call) return false;
            if (user instanceof GetElementPtrInst && !user.checkUsers()) return false;
            if( user instanceof Bitcast && !user.checkUsers()) return false;
        }
        return true;
    }
    public Function getFatherFunction(){
        for(var user : users){
            if(user instanceof Function){
                return (Function) user;
            }
        }
        return null;
    }

    public Type getType() {
        return type;
    }

    public void replaceAllUsers(Value replaceOp) {
        ArrayList<User> allusers = new ArrayList<>(users);
        for(var user : allusers){
            if (!(user instanceof Function)) {
                user.replaceOp(this, replaceOp);
            }
        }
    }

//    public void bindTo(Value value, HashMap<Value, Value> valueHashMap){
//        valueHashMap.put(this, value);
//    }

    public void cloneTo(Value target) throws CloneNotSupportedException {
        target.name = name;
        target.type = type;
        target.users = new LinkedList<>();
        target.users.addAll(users);
    }

    public void afterclone(LinkedHashMap<Value, Value> valueMap){
        LinkedList<User> newUsers = new LinkedList<>();
        for(User user :users){
            newUsers.add((User) valueMap.get(user));
        }
        this.users = newUsers;
    }

    public void oriInitBound(){
        this.settedBound = false;
        if(this instanceof BinaryInstr){
            ((BinaryInstr) this).combineAddcount = 1;
            ((BinaryInstr) this).combineAddori = null;
        }
    }

    private void initBound(){
        if(!this.settedBound && this.type instanceof IntType){
            if(this instanceof ConstNumber){
                lowerBound = (Integer) ((ConstNumber) this).value;
                upperBound = lowerBound;
            }else{
                this.upperBound = Integer.MAX_VALUE;
                this.lowerBound = Integer.MIN_VALUE;
            }
            this.settedBound = true;
        }else if(!(this.type instanceof IntType)){
            ErrOutput.outputErr("error: try to predict a float");
        }
    }

    public void setBound(int upperBound, int lowerBound){
        this.upperBound = upperBound;
        this.lowerBound = lowerBound;
        this.settedBound = true;
    }

    public int getUpperBound(){
        initBound();
        return this.upperBound;
    }

    public int getLowerBound(){
        initBound();
        return this.lowerBound;
    }

    public boolean isFixBound(){
        initBound();
        return this.lowerBound == this.upperBound;
    }
}
