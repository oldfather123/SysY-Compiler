package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;

/**
 * 将PC指令转换为move指令，便于phi消除后能够转换为mips指令
 */
public class MoveInst extends Inst{

    private Value source;
    private Value target;

    public MoveInst(String name, IRBasicType irBasicType, BasicBlock basicBlock, Value source, Value target){
        super(name,irBasicType,basicBlock);
        this.source=source;
        this.target=target;
    }

    public Value getTarget() {
        return target;
    }

    public void setTarget(Value target) {
        this.target = target;
    }

    public Value getSource() {
        return source;
    }

    public void setSource(Value source) {
        this.source = source;
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        fout.write("move ");
        fout.write(target.getTypeStr()+" "+target.getName()+" "+source.getTypeStr()+" "+source.getName());
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {

    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }
}
