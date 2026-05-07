package Pass.IR;

import IR.IRModule;
import IR.Value.BasicBlock;
import IR.Value.Function;
import IR.Value.GlobalVar;
import IR.Value.Instructions.CallInst;
import IR.Value.Instructions.Instruction;
import IR.Value.Instructions.LoadInst;
import IR.Value.Instructions.StoreInst;
import IR.Value.Value;
import Pass.IR.Utils.AliasAnalysis;
import Pass.IR.Utils.InterproceduralAnalysis;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.HashMap;

public class LoadLVN implements Pass.IRPass {

    @Override
    public void run(IRModule module) {
        InterproceduralAnalysis.run(module);
        for(Function function : module.getFunctions()){
            for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
                BasicBlock bb = bbNode.getValue();
                HashMap<Value, Value> LVNMap = new HashMap<>();
                IList.INode<Instruction, BasicBlock> itInstNode = bb.getInsts().getHead();
                while (itInstNode != null){
                    Instruction inst = itInstNode.getValue();
                    itInstNode = itInstNode.getNext();
                    if(inst instanceof LoadInst loadInst){
                        Value root = AliasAnalysis.getRoot(loadInst.getPointer());
                        if(AliasAnalysis.isGlobal(root)){
                            continue;
                        }
                        //  只做局部数组的load
                        if(LVNMap.containsKey(loadInst.getPointer())){
                            loadInst.replaceUsedWith(LVNMap.get(loadInst.getPointer()));
                            loadInst.removeSelf();
                        }
                        else{
                            LVNMap.put(loadInst.getPointer(), loadInst);
                        }
                    }
                    else if(inst instanceof StoreInst storeInst){
                        LVNMap.remove(storeInst.getPointer());
                    }
                    else if(inst instanceof CallInst callInst){
                        for(Value value : callInst.getUseValues()){
                            if(value.getType().isPointerType()){
                                LVNMap.clear();
                            }
                        }
                    }

                }
            }
        }
    }

    @Override
    public String getName() {
        return "LoadLVN";
    }
}
