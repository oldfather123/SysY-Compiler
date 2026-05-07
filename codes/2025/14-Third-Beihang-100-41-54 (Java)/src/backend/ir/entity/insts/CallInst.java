package backend.ir.entity.insts;

import backend.ir.entity.*;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;

/**
 * &#064;Classname CallInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:20
 * &#064;Created MuJue
 */
public class CallInst extends Inst{
    public CallInst(String name, IRBasicType basicType, BasicBlock basicBlock) {
        super(name, basicType, basicBlock);
    }
    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        if (usees.isEmpty()) return;
        
        Value func = usees.get(0);
        
        if (!isVoid()) {
            fout.write(name + " = ");
        }
        fout.write("call " + getTypeStr() + " ");

        if(func.getName().equals("putf")){
            fout.write("(i8*, ...) ");
        }
        func.printName(fout);
        fout.write("(");

        boolean isParam = false;
        int argSize = usees.size();
        for (int i = 1; i < argSize; i++) {
            if(isParam){
                fout.write(", ");
            }
            isParam = true;
            Value arg = usees.get(i);
            String paramStr = arg.getTypeStr();
            fout.write(paramStr + " ");
            arg.printName(fout);
        }
        
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
