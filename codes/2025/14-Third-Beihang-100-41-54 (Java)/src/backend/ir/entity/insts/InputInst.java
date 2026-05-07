package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;

/**
 * &#064;Classname InputInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:22
 * &#064;Created MuJue
 */
public class InputInst extends Inst{
    public InputInst(String name, IRBasicType IRBasicType, BasicBlock basicBlock) {
        super(name, IRBasicType, basicBlock);
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        fout.write(name + " = call " + getTypeStr() + " @getint()");
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
        // 输入指令没有操作数
    }

    @Override
    public void replaceWithNewValue(Value oldValue, Value newValue){
        super.replaceWithNewValue(oldValue,newValue);
    }
}
