package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.List;

/**
 * &#064;Classname GetElemPtrInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:21
 * &#064;Created MuJue
 */
public class GetElemPtrInst extends Inst{
    public GetElemPtrInst(String name, IRBasicType IRBasicType, List<Integer> dim, BasicBlock basicBlock){
       super(name, IRBasicType, false, true, dim, basicBlock);
    }
    public List<Value> getIndexList(){
        return usees.subList(1, usees.size());
    }
    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        if (usees.isEmpty()) return;
        
        Value basePointer = usees.get(0);
        // 构建数组类型字符串
        String arrayType = basePointer.getTypeStr();
        arrayType = arrayType.replace("*", "");

        fout.write(name + " = getelementptr " + arrayType + ", " + arrayType + "* ");
        basePointer.printName(fout);

        int idxSize = usees.size();
        for(int i = 1; i < idxSize; i++){
            fout.write(", i32 ");
            usees.get(i).printName(fout);
        }
    }
    @Override
    public void printName(BufferedWriter fout) throws IOException {
        try {
            fout.write(name);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {
        if (!usees.isEmpty()) {
            try {
                usees.getFirst().printName(fout);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        }
    }

    @Override
    public void replaceWithNewValue(Value oldValue,Value newValue){
        super.replaceWithNewValue(oldValue,newValue);
    }
}
