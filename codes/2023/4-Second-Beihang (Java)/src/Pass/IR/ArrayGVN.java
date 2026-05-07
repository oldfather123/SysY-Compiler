package Pass.IR;

import IR.IRModule;
import IR.Value.BasicBlock;
import IR.Value.Function;
import IR.Value.Instructions.*;
import IR.Value.User;
import IR.Value.Value;
import Pass.IR.Utils.DomAnalysis;
import Pass.IR.Utils.InterproceduralAnalysis;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

//  将数组的load进行GVN（其实不是GVN,实际作用LVN< this < SVN < GVN）
//  可以起个名叫做AVN(Adjacent Value Numbering)
public class ArrayGVN implements Pass.IRPass {

    //  记录指针与其loadInst的映射关系
    HashMap<Value, Value> GVNMap = new HashMap<>();
    HashSet<GepInst> canGVNPointer = new HashSet<>();

    @Override
    public void run(IRModule module) {
        for(Function function : module.getFunctions()){
            DomAnalysis.run(function);
            //  针对指针没有store的情况
            runSimpleGVN(function);

            runArrayGVN(function);
        }
    }

    //  当一个指针没有store时，全可以替换
    private void runSimpleGVN(Function function){
        GVNMap.clear();
        canGVNPointer.clear();
        initCanGVNPointers(function);
        RPOSearch(function.getBbEntry());
    }

    private void runArrayGVN(Function function){
        for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
            BasicBlock bb = bbNode.getValue();
            GVNMap.clear();
            addLoadsToGVN(bb);
            for(BasicBlock sonBb : bb.getIdoms()){
                if(bb.getNxtBlocks().contains(sonBb)){
                    replaceLoadsInBb(sonBb);
                    for(BasicBlock sonSonBb : sonBb.getIdoms()){
                        if(sonBb.getNxtBlocks().contains(sonSonBb)){
                            replaceLoadsInBb(sonSonBb);
                        }
                    }
                }
            }
        }
    }

    private void replaceLoadsInBb(BasicBlock bb){
        IList.INode<Instruction, BasicBlock> itInstNode = bb.getInsts().getHead();
        HashMap<Value, Value> tmpRemoves = new HashMap<>();
        while(itInstNode != null){
            Instruction inst = itInstNode.getValue();
            itInstNode = itInstNode.getNext();
            if(inst instanceof LoadInst loadInst && GVNMap.containsKey(loadInst.getPointer())){
                loadInst.replaceUsedWith(GVNMap.get(loadInst.getPointer()));
                loadInst.removeSelf();
            }
            else if(inst instanceof StoreInst storeInst){
                Value key = storeInst.getPointer();
                if(GVNMap.containsKey(key)){
                    Value value = GVNMap.get(key);
                    tmpRemoves.put(key, value);
                    GVNMap.remove(key);
                }
            }
        }

        for(Value key : tmpRemoves.keySet()){
            GVNMap.put(key, tmpRemoves.get(key));
        }
    }

    private void addLoadsToGVN(BasicBlock bb){
        for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
            Instruction inst = instNode.getValue();
            if(inst instanceof LoadInst loadInst){
                if(GVNMap.containsKey(loadInst.getPointer())){
                    loadInst.replaceUsedWith(GVNMap.get(loadInst.getPointer()));
                }
                else {
                    GVNMap.put(loadInst.getPointer(), loadInst);
                }
            }
            else if(inst instanceof StoreInst storeInst){
                GVNMap.remove(storeInst.getPointer());
            }
        }
    }

    private void initCanGVNPointers(Function function){
        ArrayList<GepInst> gepInsts = new ArrayList<>();
        for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
            BasicBlock bb = bbNode.getValue();
            for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()) {
                Instruction inst = instNode.getValue();
                if(inst instanceof GepInst gepInst){
                    gepInsts.add(gepInst);
                }
            }
        }
        for(GepInst gepInst : gepInsts){
            boolean hasStore = false;
            for(User user : gepInst.getUserList()){
                Instruction userInst = (Instruction) user;
                if(userInst instanceof StoreInst){
                    hasStore = true;
                    break;
                }
            }
            if(!hasStore){
                canGVNPointer.add(gepInst);
            }
        }
    }

    private void RPOSearch(BasicBlock bb){
        IList.INode<Instruction, BasicBlock> itInstNode = bb.getInsts().getHead();
        HashSet<LoadInst> numberedInsts = new HashSet<>();
        while (itInstNode != null){
            Instruction inst = itInstNode.getValue();
            itInstNode = itInstNode.getNext();
            if (inst instanceof LoadInst loadInst && loadInst.getPointer() instanceof GepInst gepInst
                    && canGVNPointer.contains(gepInst)){
                boolean isNumbered = runGVNOnLoad(loadInst);
                if(isNumbered){
                    numberedInsts.add(loadInst);
                }
            }
        }

        for(BasicBlock idomBb : bb.getIdoms()){
            RPOSearch(idomBb);
        }

        for(LoadInst loadInst : numberedInsts){
            GVNMap.remove(loadInst.getPointer());
        }
    }

    private boolean runGVNOnLoad(LoadInst loadInst){
        if(GVNMap.containsKey(loadInst.getPointer())){
            loadInst.replaceUsedWith(GVNMap.get(loadInst.getPointer()));
            loadInst.removeSelf();
            return false;
        }
        GVNMap.put(loadInst.getPointer(), loadInst);
        return true;
    }

    @Override
    public String getName() {
        return "ArrayGVN";
    }
}
