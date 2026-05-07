package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;

/**
 * &#064;Classname StoreInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:22
 * &#064;Created MuJue
 */
public class StoreInst extends Inst{
    public StoreInst(IRBasicType IRBasicType, BasicBlock basicBlock) {
        super("store", IRBasicType, basicBlock);
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        if (usees.size() < 2) return;
        
        Value value = usees.get(0);
        Value pointer = usees.get(1);
        String typeStr = getTypeStr();
        
        fout.write("store " + typeStr);
        fout.write(" ");
        try {
            value.printName(fout);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
        fout.write(", " + typeStr + "*");
        fout.write(" ");
        try {
            pointer.printName(fout);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
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
        try {
            if (usees.size() >= 2) {
                usees.get(0).printName(fout);
                fout.write(", ");
                usees.get(1).printName(fout);
            }
        } catch (IOException e) {
            // 处理异常
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void replaceWithNewValue(Value oldValue,Value newValue){
        super.replaceWithNewValue(oldValue,newValue);
    }
}
