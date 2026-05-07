package backend.ir.entity.insts;

import backend.ir.entity.*;
import backend.ir.optimize.tool.MemToReg;
import frontend.semantic.symbol.IRBasicType;

import java.util.List;
import java.util.Set;

/**
 * &#064;Classname Inst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:23
 * &#064;Created MuJue
 */
public abstract class Inst extends User {
    protected BasicBlock basicBlock;

    public Inst(String name, IRBasicType IRBasicType, BasicBlock basicBlock) {
        super(name, IRBasicType);
        this.basicBlock = basicBlock;
    }
    public Inst(String name, IRBasicType IRBasicType, boolean isConst, boolean isArray, List<Integer> dim, BasicBlock basicBlock){
        super(name, IRBasicType, isConst, isArray, dim);
        this.basicBlock = basicBlock;
    }

    public BasicBlock getBasicBlock() {
        return basicBlock;
    }
    public void setBasicBlock(BasicBlock basicBlock) {
        this.basicBlock = basicBlock;
    }

    public void replaceWithNewValue(Value oldValue,Value newValue){
        for(int index = 0;index < this.usees.size();index ++ ){
            Value value = this.usees.get(index);
            if(value.getName().equals(oldValue.getName())){
                this.usees.set(index,newValue);
                newValue.addUser(this);
            }
        }
    }

    public void insertPhiInst(){
        MemToReg.resetByInst(this);
        MemToReg.insertPhiInst();
    }

    /**
     * 生成活跃变量分析的use和def
     */
    public void generateUseDef(){
        Set<Value> use = basicBlock.getUseSet();
        Set<Value> def = basicBlock.getDefSet();
        for(Value usee:usees){
            if(!def.contains(usee) && (
                    usee instanceof Inst || usee instanceof GlobalVar ||  usee instanceof Argument
                    )){
                //说明usee既不在定义链，同时也属于指令/全局变量/参数，应当加入使用链
                use.add(usee);
            }
        }
        //如果使用链没有自己，那么自己就是属于定义链
        if(!use.contains(this) && isUseful()){
            def.add(this);
        }
    }

    /**
     * 判断指令是否有用
     */
    public boolean isUseful(){
         return this instanceof AllocateInst
                 || this instanceof BinOpInst
                 || this instanceof CallInst
                 || this instanceof CmpInst
                 || this instanceof GetElemPtrInst
                 || this instanceof InputInst
                 || this instanceof LoadInst
                 || this instanceof PHIInst
                 || this instanceof TransInst;
    }

    /**
     * 指令级别的活跃变量分析获取
     */
    public Set<Value> getUseSet(){
        return basicBlock.getActiveVarAnalysisInst().getUseSet(this);
    }

    public Set<Value> getDefSet(){
        return basicBlock.getActiveVarAnalysisInst().getDefSet(this);
    }

    public Set<Value> getInSet(){
        return basicBlock.getActiveVarAnalysisInst().getInSet(this);
    }

    public Set<Value> getOutSet(){
        return basicBlock.getActiveVarAnalysisInst().getOutSet(this);
    }
}
