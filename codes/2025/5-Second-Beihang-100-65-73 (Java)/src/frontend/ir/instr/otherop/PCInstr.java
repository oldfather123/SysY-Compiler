package frontend.ir.instr.otherop;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.TVoid;
import frontend.ir.structure.BasicBlock;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

/**
 * parallel copy
 * 消 phi 用的中间指令
 */
public class PCInstr extends Instruction {
    private final List<Value> srcValList;
    private final List<Value> dstValList;

    public PCInstr(BasicBlock parentBB) {
        super(new TVoid(), -1, parentBB);
        srcValList = new ArrayList<>();
        dstValList = new ArrayList<>();
    }

    public void putPair(Value src, Value dst) {
        srcValList.add(src);
        dstValList.add(dst);
    }

    public List<Value> getSrcValList() {
        return srcValList;
    }

    public List<Value> getDstValList() {
        return dstValList;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        throw new RuntimeException("不该再改了");
    }

    @Override
    public String myHash() {
        return Integer.toString(this.hashCode());
    }

    @Override
    public Value simplify() {
        return null;
    }

    public HashSet<Value> getOperands() {
        return new HashSet<>(srcValList);
    }
}
