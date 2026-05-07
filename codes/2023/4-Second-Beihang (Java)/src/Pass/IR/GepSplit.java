package Pass.IR;

import IR.IRBuildFactory;
import IR.IRModule;
import IR.Value.BasicBlock;
import IR.Value.ConstInteger;
import IR.Value.Function;
import IR.Value.Instructions.GepInst;
import IR.Value.Instructions.Instruction;
import IR.Value.Value;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;

public class GepSplit implements Pass.IRPass{
    private final IRBuildFactory f = IRBuildFactory.getInstance();

    @Override
    public void run(IRModule module) {
        runGepSpill(module);
        removeUselessGep(module);
    }

    private void runGepSpill(IRModule module){
        for(Function function : module.getFunctions()){
            for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
                BasicBlock bb = bbNode.getValue();
                IList.INode<Instruction, BasicBlock> itInstNode = bb.getInsts().getHead();
                while (itInstNode != null){
                    Instruction inst = itInstNode.getValue();
                    if(inst instanceof GepInst gepInst && gepInst.getIndexs().size() >= 2) {
                        split(gepInst);
                    }
                    itInstNode = itInstNode.getNext();
                }
            }
        }
    }

    private void split(GepInst gepInst){
        ArrayList<Value> indexs = new ArrayList<>(gepInst.getIndexs().subList(1, gepInst.getIndexs().size()));
        Value ptr = gepInst.getTarget();
        Instruction pos = gepInst;
        Value pre = gepInst.getIndexs().get(0);
        ArrayList<Value> preIndexs = new ArrayList<>();
        preIndexs.add(pre);
        if (gepInst.getIndexs().size() == 2) {
            if (pre instanceof ConstInteger constInteger && constInteger.getValue() == 0) {
                return;
            }
        }
        Instruction preOffset = f.getGepInst(ptr, preIndexs);
        preOffset.insertBefore(pos);
        pos = preOffset;
        ptr = preOffset;
        for (Value index: indexs) {
            ArrayList<Value> tempIndexs = new ArrayList<>();
            tempIndexs.add(f.buildNumber(0));
            tempIndexs.add(index);
            GepInst temp = f.getGepInst(ptr, tempIndexs);
            temp.insertAfter(pos);
            pos = temp;
            ptr = temp;
        }
        gepInst.replaceUsedWith(pos);
    }

    private void removeUselessGep(IRModule module){
        for (Function function: module.getFunctions()) {
            for (IList.INode<BasicBlock, Function> bbNode : function.getBbs()) {
                BasicBlock bb = bbNode.getValue();
                IList.INode<Instruction, BasicBlock> instNode = bb.getInsts().getHead();
                while (instNode != null){
                    Instruction inst = instNode.getValue();
                    instNode = instNode.getNext();
                    if (inst instanceof GepInst && isGepOneZero((GepInst) inst)) {
                        inst.replaceUsedWith(((GepInst) inst).getTarget());
                        inst.removeSelf();
                    }
                }
            }
        }
    }

    private boolean isGepOneZero(GepInst gepInst){
        if(gepInst.getIndexs().size() > 1){
            return false;
        }
        Value value = gepInst.getIndexs().get(0);
        return value instanceof ConstInteger constInteger && constInteger.getValue() == 0;
    }

    @Override
    public String getName() {
        return "GepSpill";
    }
}
