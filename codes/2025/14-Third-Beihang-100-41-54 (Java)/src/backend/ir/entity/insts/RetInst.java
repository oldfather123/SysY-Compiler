package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;

/**
 * &#064;Classname RetInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:22
 * &#064;Created MuJue
 */
public class RetInst extends Inst{
    public RetInst(String name, IRBasicType IRBasicType, BasicBlock basicBlock) {
        super(name, IRBasicType, basicBlock);
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        if (usees.isEmpty()) {
            // void返回
            fout.write("ret void");
        } else {
            // 带值的返回
            Value returnValue = usees.get(0);
            String typeStr = getTypeStr();
            fout.write("ret " + typeStr + " ");
            returnValue.printName(fout);
        }
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        fout.write(name);
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {
        if (!usees.isEmpty()) {
            usees.get(0).printName(fout);
        }
    }

    @Override
    public void replaceWithNewValue(Value oldValue,Value newValue){
        super.replaceWithNewValue(oldValue,newValue);
    }
}
