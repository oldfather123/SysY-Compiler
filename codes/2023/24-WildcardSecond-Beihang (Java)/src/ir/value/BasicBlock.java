package ir.value;
import ir.Value;
import ir.instr.Br;
import ir.instr.Instr;
import ir.pass.analyze.Loop;
import ir.value.user.Function;
import tools.ErrOutput;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;

//BasicBlock承担容器作用
public class BasicBlock extends Value {
    public boolean isLoopHead;
    private ArrayList<Value> instrs = new ArrayList<>();
    private BasicBlock parent = null;
    private final ArrayList<BasicBlock> preds = new ArrayList<>();
    private final ArrayList<BasicBlock> succs = new ArrayList<>();

    private ArrayList<BasicBlock> dominate = new ArrayList<>();
    private ArrayList<BasicBlock> dominatedBy = new ArrayList<>();
    private ArrayList<BasicBlock> idominate = new ArrayList<>();
    public LinkedHashSet<BasicBlock> accessible = new LinkedHashSet<>();

    private BasicBlock idominatedBy;

    private final LinkedHashSet<BasicBlock> domFrontier = new LinkedHashSet<>();

    private int loopDepth=0;
    public Loop loop= null;

    private int domDepth;
    public Function function;

    //remove phi 使用
    public ArrayList<Value> fromcopy = new ArrayList<>();
    public ArrayList<Value> tocopy = new ArrayList<>();

    //end

    public int getLoopDepth() {
        return loopDepth;
    }

    public void setLoopDepth(int loopDepth) {
        this.loopDepth = loopDepth;
    }

    public int getDomDepth() {
        return domDepth;
    }

    public void setDomDepth(int domDepth) {
        this.domDepth = domDepth;
    }

    public LinkedHashSet<BasicBlock> getDomFrontier() {
        return domFrontier;
    }

    public void setDominate(ArrayList<BasicBlock> dominate) {
        this.dominate = dominate;
    }

    public void setDominatedBy(ArrayList<BasicBlock> dominatedBy) {
        this.dominatedBy = dominatedBy;
    }

    public void setIdominate(ArrayList<BasicBlock> idominate) {
        this.idominate = idominate;
    }

    public void setIdominatedBy(BasicBlock idominatedBy) {
        this.idominatedBy = idominatedBy;
    }

    public ArrayList<BasicBlock> getDominate() {
        return dominate;
    }

    public ArrayList<BasicBlock> getDominatedBy() {
        return dominatedBy;
    }

    public ArrayList<BasicBlock> getIdominate() {
        return idominate;
    }

    public BasicBlock getIdominatedBy() {
        return idominatedBy;
    }

    public void printIdominate(){
        System.out.println("idominate");
        System.out.println(name+":");
        for(var idm:idominate){
            System.out.println(idm.getName());
        }
    }
    public void printIdominatedBy(){
        System.out.println("idominatedBy");
        System.out.println(name+":");
        if(idominatedBy!=null) System.out.println(idominatedBy.getName());
    }
    public void domClear(){
        dominate.clear();
        dominatedBy.clear();
        idominate.clear();
        idominatedBy = null;
    }
    private final boolean haveLabel;//对于设置了haveLabel的Block 其标签就是其名字
    private BasicBlock continueTo = null;
    private BasicBlock breakTo = null;
    public boolean isEnd;

    //直接设置name为Label
    //name既为占用的寄存器标识符
    public BasicBlock(String name, User parent, boolean haveLabel) {
        super(parent, name);
        this.haveLabel = haveLabel;
        isEnd = false;
    }

    public void addValue(Value value) {
        instrs.add(value);
        if(value instanceof BasicBlock) {
            ((BasicBlock) value).setParent(this);
        } else if(value instanceof Instr) {
            ((Instr) value).setParent(this);
            if(value instanceof Br){
                for(var v : ((Br) value).getOperands()){
                    if(v instanceof BasicBlock){
                        ((BasicBlock) v).addPreds(this);
                    }
                }
                this.isEnd = true;
            }
        }
    }

    public void addValueFirst(Value value) {
        instrs.add(0, value);
        if(value instanceof BasicBlock) {
            ((BasicBlock) value).setParent(this);
        } else if(value instanceof Instr) {
            ((Instr) value).setParent(this);
            if(value instanceof Br){
                for(var v : ((Br) value).getOperands()){
                    if(v instanceof BasicBlock){
                        ((BasicBlock) v).addPreds(this);
                    }
                }
                this.isEnd = true;
            }
        }
    }

    public ArrayList<Value> getInsts() {
        return instrs;
    }

    public void setInsts(ArrayList<Value> instrs){
        this.instrs = instrs;
    }

    public void merge(BasicBlock block) {
        for(int i = 0;i < block.getInsts().size();i++) {
            Value value = block.getInsts().get(i);
            if(value instanceof BasicBlock) {
                ((BasicBlock) value).setParent(this);
            } else if(value instanceof Instr) {
                ((Instr) value).setParent(this);
            }
            instrs.add(block.getInsts().get(i));
        }
        if(block.isEnd){
            this.isEnd = true;
        }
    }

    public BasicBlock getParent() {
        return parent;
    }

    public void setParent(BasicBlock parent) {
        this.parent = parent;
    }

    public void addPreds(BasicBlock block) {
        //TODO：维护preds关系
        preds.add(block);
    }

    public void addSuccs(BasicBlock block) {
        //TODO：维护preds关系
        succs.add(block);
    }

    public ArrayList<BasicBlock> getPreds() {
        return preds;
    }

    public ArrayList<BasicBlock> getSuccs() {
        return succs;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(name);
        sb.append(":\n");
        if(ErrOutput.DEBUG && isLoopHead){
            sb.append("//loop head\n");
        }
        if(ErrOutput.DEBUG){
            sb.append(String.format("loopDepth : %d\n", getLoopDepth()));
        }
        for (Value instr : instrs) {
            if (instr instanceof BasicBlock) {
                sb.append(instr).append("\n");
            } else {
                sb.append("\t").append(instr).append("\n");
            }
        }
        return sb.toString();
    }

    public String toStringForGlobals(){
        StringBuilder sb = new StringBuilder();
        for (Value instr : instrs) {
            sb.append(instr).append("\n");
        }
        return sb.toString();
    }

    public BasicBlock getContinueTo() {
        return continueTo;
    }

    public void setContinueTo(BasicBlock continueTo) {
        this.continueTo = continueTo;
    }

    public BasicBlock getBreakTo() {
        return breakTo;
    }

    public void setBreakTo(BasicBlock breakTo) {
        this.breakTo = breakTo;
    }

    @Override
    public String getNameWithType() {
        return getFullName();
    }

    public void insertBefore(Value instr, Value newinstr) {
        int pos = instrs.indexOf(instr);
        if(pos == -1){
            assert false;
        }else{
            instrs.add(pos, newinstr);
        }
    }

    public void insertAfter(Value instr, Value newinstr) {
        int pos = instrs.indexOf(instr);
        if(pos == -1){
            assert false;
        }else{
            instrs.add(pos+1, newinstr);
        }
    }

    public void deletInstr(Value instr) {
        instrs.remove(instr);
    }

    public BasicBlock clone(User parent, LinkedHashMap<Value, Value> valueHashMap) throws CloneNotSupportedException {
        BasicBlock clone = new BasicBlock(this.name,  parent,this.haveLabel);
        for(Value v : this.instrs){
            if(!(v instanceof Instr)){
                ErrOutput.outputErr("error: try to clone un-instr in bb");
                return null;
            }
            clone.addValue(((Instr)v).clone(clone, valueHashMap));
        }
        valueHashMap.put(this, clone);
        return clone;
    }

    public void reClone(LinkedHashMap<Value, Value> valueHashMap) {
        for(Value v:this.instrs) {
            if(!(v instanceof Instr)){
                ErrOutput.outputErr("error: try to clone un-instr in bb");
            }
            ((Instr)v).reClone(valueHashMap);
        }
    }
}
