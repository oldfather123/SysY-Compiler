package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;

/**
 * &#064;Classname UCJumpInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:23
 * &#064;Created MuJue
 */
public class UCJumpInst extends Inst{
    public UCJumpInst(BasicBlock basicBlock) {
        super("-1", IRBasicType.NONE, basicBlock);
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        fout.write("br label ");
        Value tarBlock = usees.get(0);
        tarBlock.printName(fout);
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {

    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }

    @Override
    public void replaceWithNewValue(Value oldValue,Value newValue){
        super.replaceWithNewValue(oldValue,newValue);
    }
}
