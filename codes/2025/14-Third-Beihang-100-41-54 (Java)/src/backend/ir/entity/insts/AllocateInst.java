package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.List;

/**
 * &#064;Classname AllocateInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:20
 * &#064;Created MuJue
 */
public class AllocateInst extends Inst{
    public AllocateInst(String name, IRBasicType IRBasicType, boolean isConst, boolean isArray, List<Integer> dim, BasicBlock bb) {
        super(name, IRBasicType, isConst, isArray, dim, bb);
    }
    @Override
    public String getTypeStr(){
        return varInfo.getTypeStr() + "*";
    }
    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        fout.write(name + " = alloca " + varInfo.getTypeStr());
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        try {
            fout.write(name);
        } catch (IOException e) {
            // 处理异常
            e.printStackTrace();
        }
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }

    @Override
    public void replaceWithNewValue(Value oldValue,Value newValue){
        super.replaceWithNewValue(oldValue,newValue);
    }
}
