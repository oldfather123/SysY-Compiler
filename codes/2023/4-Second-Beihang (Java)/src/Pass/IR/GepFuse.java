package Pass.IR;

import IR.IRBuildFactory;
import IR.IRModule;
import IR.Type.ArrayType;
import IR.Type.PointerType;
import IR.Value.*;
import IR.Value.Instructions.*;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;

//  GepFuse后的每个gepInst都应该是针对数组进行gep
public class GepFuse implements Pass.IRPass {
    private ArrayList<GepInst> GepInsts = new ArrayList<>();
    private final IRBuildFactory f = IRBuildFactory.getInstance();

    @Override
    public void run(IRModule module) {
        for(Function function : module.getFunctions()){
            runGepFuse(function);
        }
    }

    private void runGepFuse(Function function){
        //  删除%x = gep target 0的情况
        for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
            BasicBlock bb = bbNode.getValue();
            IList.INode<Instruction, BasicBlock> itInstNode = bb.getInsts().getHead();
            while (itInstNode != null){
                Instruction inst = itInstNode.getValue();
                itInstNode = itInstNode.getNext();
                if(inst instanceof GepInst gepInst && isGepOneZero(gepInst)){
                    gepInst.replaceUsedWith(gepInst.getTarget());
                    gepInst.removeSelf();
                }
            }
        }

        for (IList.INode<BasicBlock, Function> bbNode : function.getBbs()) {
            BasicBlock bb = bbNode.getValue();
            for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                Instruction inst = instNode.getValue();
                if (inst instanceof GepInst gepInst && gepInst.getTarget() instanceof GepInst preGepInst) {
                    if(canFuse(preGepInst, gepInst)) {
                        fuse(preGepInst, gepInst);
                    }
                }
            }
        }
    }

    /*  将preGepInst合并到当前gepInst中
    *   pre: gep 0 1 0
    *   now: gep 0 1
    *   fuse: gep 0 1 0 1
    *
    *   pre: gep 0 1 0
    *   now: gep 1 1
    *   fuse: gep 0 1 1 1
    * */
    private void fuse(GepInst preGepInst, GepInst gepInst){
        gepInst.modifyTarget(preGepInst.getTarget());

        ArrayList<Value> preIndexs = preGepInst.getIndexs();
        ArrayList<Value> nowIndexs = gepInst.getIndexs();

        ArrayList<Value> newIndexs = new ArrayList<>();
        for(int i = 0; i < preIndexs.size() - 1; i++){
            newIndexs.add(preIndexs.get(i));
        }
        int preConst = ((ConstInteger) preIndexs.get(preIndexs.size() - 1)).getValue();
        int nowConst = ((ConstInteger) nowIndexs.get(0)).getValue();
        newIndexs.add(f.buildNumber(preConst + nowConst));
        for(int i = 1; i < nowIndexs.size(); i++){
            newIndexs.add(nowIndexs.get(i));
        }

        gepInst.modifyIndexs(newIndexs);
    }

    private boolean canFuse(GepInst preGepInst, GepInst gepInst){
        Value preLastValue = preGepInst.getIndexs().get(preGepInst.getIndexs().size() - 1);
        Value nowFirstValue = gepInst.getIndexs().get(0);
        return preLastValue instanceof Const && nowFirstValue instanceof Const;
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
        return null;
    }
}
