package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;

/**
 * &#064;Classname OutputInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:22
 * &#064;Created MuJue
 */
public class OutputInst extends Inst{
    public OutputInst(String name, IRBasicType IRBasicType, BasicBlock basicBlock) {
        super(name, IRBasicType, basicBlock);
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        if (usees.isEmpty()) return;
        
        Value value = usees.get(0);
        String typeStr = getTypeStr();
        
        fout.write("call void @putint(" + typeStr + " ");
        value.printAssembly(fout);
        fout.write(")");
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        try {
            fout.write(name);
        } catch (IOException e) {
            // 处理异常
        }
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {
        if (!usees.isEmpty()) {
            try {
                usees.get(0).printName(fout);
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
