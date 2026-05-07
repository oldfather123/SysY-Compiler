package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;

/**
 * &#064;Classname BranchInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:20
 * &#064;Created MuJue
 */
public class BranchInst extends Inst {
    public BranchInst(String name, IRBasicType IRBasicType, BasicBlock basicBlock) {
        super(name, IRBasicType, basicBlock);
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        Value condition = usees.get(0);
        Value trueTarget = usees.get(1);
        Value falseTarget = usees.get(2);

        fout.write("br i1 ");
        condition.printName(fout);
        fout.write(", label ");
        trueTarget.printName(fout);
        fout.write(", label ");
        falseTarget.printName(fout);
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
            for (int i = 0; i < usees.size(); i++) {
                if (i > 0) fout.write(", ");
                usees.get(i).printName(fout);
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
