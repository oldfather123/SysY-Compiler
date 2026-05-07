package backend.ir.entity;

import frontend.semantic.symbol.IRBasicType;
import frontend.semantic.symbol.VarInfo;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * &#064;Classname Value
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:18
 * &#064;Created MuJue
 */
public abstract class Value {
    protected String name;
    protected final VarInfo varInfo;
    protected List<User> users =  new ArrayList<>();
    public abstract void printAssembly(BufferedWriter fout) throws IOException;
    public abstract void printName(BufferedWriter fout) throws IOException;
    public abstract void printUse(BufferedWriter fout) throws IOException;
    public Value(String name, IRBasicType IRBasicType){
        this.name = name;
        varInfo = new VarInfo(IRBasicType);
    }
    public Value(String name, IRBasicType IRBasicType, boolean isConst, boolean isArray, List<Integer> dim){
        this.name = name;
        varInfo = new VarInfo(IRBasicType, isConst, isArray, dim);
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }
    public IRBasicType getBasicType() {
        return varInfo.getBasicType();
    }
    public void setBasicType(IRBasicType IRBasicType) {varInfo.setBasicType(IRBasicType);}
    public String getTypeStr(){return varInfo.getTypeStr();}
    public List<Integer> getFullDim(){
        return varInfo.getFullDim();
    }
    public Integer getBasicByte(){
        return varInfo.getBasicBytes();
    }
    public Integer getTotalBytes(){
        return varInfo.getTotalBytes();
    }
    public boolean isInt() {return varInfo.isInt();}
    public boolean isFloat() {return varInfo.isFloat();}
    public boolean isChar(){return varInfo.isChar();}
    public boolean isVoid(){
        return varInfo.isVoid();
    }
    public boolean isString(){return varInfo.isString();}
    public boolean isBoolean() {return varInfo.isBoolean();}
    public boolean isArray(){
        return varInfo.isArray();
    }
    public boolean isConst(){
        return varInfo.isConst();
    }
    public boolean isPointer() {
        return varInfo.isPointer();
    }
    public Integer getDimSize(){ return varInfo.getDimSize(); }
    public VarInfo getVarInfo(){
        return varInfo;
    }

    public List<User> getUsers() {
        return users;
    }

    public void setUsers(List<User> users) {
        this.users = users;
    }

    public void addUser(User user){users.add(user);}
    public void removeUser(User user){users.remove(user);}

    /**
     * 将自己替换成新的value
     * @param value
     */
    public void replaceAllUser(Value value){
        List<User> newUsers = new ArrayList<>(users);
        for(User user : newUsers){
            if(user instanceof BasicBlock) continue;
            user.replaceValue(this,value);
        }
    }

    public void replaceUser(Value oldValue, Value newValue, User user){
        if(oldValue!=null &&  newValue!=null){
            oldValue.removeUser(user);
            newValue.addUser(user);
        }
    }
}
