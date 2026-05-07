package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * &#064;Classname PCopyInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:22
 * &#064;Created MuJue
 */
/**
 * 这里的PC指令指的是Parallel copy, 仅用于消除phi
 * 实际上这个指令在LLVM IR中不存在，并且也没有对应的后端代码
 * 只是方便消除phi
 * 由于原来的phi可能作为操作数被别的量使用了
 * 所以这里PC也需要采用Instruction类
 * pc: a1 → b1, a2 → b2, ..., an → bn
 * 表示同时将 a1 赋值给 b1，a2 赋值给 b2，依此类推。
 */
public class PCopyInst extends Inst{
    /**
     * 记录源寄存器, 为了方便串行化move指令的构建
     */
    private List<Value> source = new ArrayList<>();
    /**
     * 记录目标寄存器, 为了方便串行化move指令的构建
     */
    private List<Value> target =  new ArrayList<>();

    public List<Value> getTarget() {
        return target;
    }

    public List<Value> getSource() {
        return source;
    }
    public void setSrcTgt(Value source, Value target) {
        this.source.add(source);
        this.target.add(target);
    }


    public PCopyInst(String name, IRBasicType basicType, BasicBlock basicBlock) {
        super(name, basicType, basicBlock);
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        fout.write(name+": ");
        for(int i = 0; i<source.size();i++){
            fout.write(source.get(i).getName() + " → ");
            fout.write(target.get(i).getName() + ", ");
        }
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {

    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }

    @Override
    public void replaceWithNewValue(Value oldValue, Value newValue){
        super.replaceWithNewValue(oldValue,newValue);
    }
}
