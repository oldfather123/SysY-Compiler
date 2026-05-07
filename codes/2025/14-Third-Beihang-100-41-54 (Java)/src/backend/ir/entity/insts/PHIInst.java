package backend.ir.entity.insts;

import backend.ir.entity.*;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.List;
import java.util.Set;

/**
 * &#064;Classname PHIInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:22
 * &#064;Created MuJue
 */
public class PHIInst extends Inst{

    private final List<BasicBlock> preBlocks;//数据流分析中的前序基本块集合

    public PHIInst(String name, IRBasicType IRBasicType, BasicBlock basicBlock,
                   List<BasicBlock> preBlocks, int... cnt) {
        super(name, IRBasicType, basicBlock);
        this.preBlocks = preBlocks;
        int size = cnt.length==0? preBlocks.size():cnt[0];
        int num = size - preBlocks.size();
        addUsee(null, size);
        for(int i=0;i<num;i++){
            preBlocks.add(null);
        }
    }

    public List<BasicBlock> getPreBlocks(){return preBlocks;}

    /**
     * 在构造函数阶段，我们向定义使用链插入的是空指针，这肯定不行
     * 所以需要根据传入的Value，修改使用的数据
     */
    public void changeValue(Value value, BasicBlock basicBlock){
        int index = preBlocks.indexOf(basicBlock);
        if (index == -1) {
            throw new IllegalArgumentException("没有这样的基本块");
        }
        if(getBasicType()==IRBasicType.FLOAT &&
                value.getBasicType()!= IRBasicType.FLOAT){
            //说明出现了如下错误情况
            //%var_phi_85 = phi float [%var_12, %Block_8], [0, %Block_16]
            //需要将0变成0.0
            if(value instanceof LiteralConst literalConst &&
            literalConst.getIntValue()==0){
                value = new LiteralConst(0.0, IRBasicType.FLOAT, false);
            }
        }
        usees.set(index, value);
        //存在其他属性的可能性，所以添加值的时候就需要重新确定值的属性
        //如果自己属性是i32时修改
        if(getBasicType()== IRBasicType.I32 && value.getBasicType()!= IRBasicType.I32){
            setBasicType(value.getBasicType());
            //如果自己属性不是i32时，且index不是0，则遍历，将所有value设置为当前属性
            if(getBasicType()== IRBasicType.FLOAT && index!=0){
                for(int i = 0; i< usees.size();i++){
                    if(usees.get(i) instanceof LiteralConst literalConst){
                        literalConst.removeUser(this);
                        LiteralConst newConst = new LiteralConst(0.0, IRBasicType.FLOAT, false);
                        usees.set(i, newConst);
                        newConst.addUser(this);
                    }
                }
            }
        }
        //由于初始化时，添加了null，现在需要将这个value和phi绑定
        value.addUser(this);
    }

    public void changePhiToPC(List<PCopyInst> pCopyInsts){
        for(int i = 0;i<usees.size();i++){
            Value value = usees.get(i);
            //如果value不是常量，就需要记录源寄存器和目标寄存器，为了之后的转move指令
            if(value instanceof LiteralConst literalConst && !literalConst.isDefined()){continue;}
            pCopyInsts.get(i).setSrcTgt(value,this);
        }
    }

    /**
     * 用于活跃变量分析
     */
    public void addIntoUse(){
        Set<Value> use = basicBlock.getUseSet();
        for(Value usee : usees){
            if(usee instanceof Inst || usee instanceof GlobalVar || usee instanceof Argument){
                use.add(usee);
            }
        }
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        String typeStr = getTypeStr();
        fout.write(name+" = phi " + typeStr +" ");
        for(int i = 0; i<preBlocks.size();i++){
            if(usees.get(i)!=null && preBlocks.get(i) != null){
                fout.write("["+usees.get(i).getName() + ", %" +  preBlocks.get(i).getName() +"]");
            }
            if(i<preBlocks.size()-1){
                fout.write(", ");
            }
        }
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        fout.write(name);
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }

    @Override
    public void replaceWithNewValue(Value oldValue, Value newValue){
        super.replaceWithNewValue(oldValue,newValue);
    }
}
