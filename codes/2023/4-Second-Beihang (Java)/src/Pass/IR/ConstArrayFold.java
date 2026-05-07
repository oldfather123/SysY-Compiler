package Pass.IR;

import IR.IRBuildFactory;
import IR.IRModule;
import IR.Type.ArrayType;
import IR.Type.PointerType;
import IR.Use;
import IR.Value.*;
import IR.Value.Instructions.*;
import Pass.IR.Utils.AliasAnalysis;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;
import java.util.HashSet;

public class ConstArrayFold implements Pass.IRPass {

    private final IRBuildFactory f = IRBuildFactory.getInstance();
    HashSet<GlobalVar> globalArrs = new HashSet<>();

    @Override
    public void run(IRModule module) {
        initConstArray(module);
        foldConstArray(module);
    }

    private void initConstArray(IRModule module){
        globalArrs.addAll(module.getGlobalVars());
        //  除去所有被store过的全局数组
        for(Function function : module.getFunctions()) {
            for (IList.INode<BasicBlock, Function> bbNode : function.getBbs()) {
                BasicBlock bb = bbNode.getValue();
                for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                    Instruction inst = instNode.getValue();
                    if(inst instanceof StoreInst storeInst){
                        Value pointer = storeInst.getPointer();
                        Value root = AliasAnalysis.getRoot(pointer);
                        if(root instanceof GlobalVar globalVar && globalVar.isArray()){
                            globalArrs.remove(globalVar);
                        }
                    }
                    else if(inst instanceof CallInst callInst){

                        for(Value arg : callInst.getUseValues()){
                            if(arg instanceof GepInst gepInst){
                                Value root = AliasAnalysis.getRoot(gepInst.getTarget());
                                if(root instanceof GlobalVar globalVar && globalVar.isArray()){
                                    globalArrs.remove(globalVar);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    private void foldConstArray(IRModule module){
        for(Function function : module.getFunctions()) {
            for (IList.INode<BasicBlock, Function> bbNode : function.getBbs()) {
                BasicBlock bb = bbNode.getValue();
                for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                    Instruction inst = instNode.getValue();
                    if(inst instanceof GepInst gepInst){
                        Value ptr = gepInst.getTarget();
                        if(ptr instanceof GlobalVar globalVar && globalArrs.contains(globalVar)){
                            ArrayList<Value> indexs = gepInst.getIndexs();
                            boolean allConst = true;
                            for(Value value : indexs){
                                if(!(value instanceof ConstInteger)){
                                    allConst = false;
                                    break;
                                }
                            }
                            if(allConst){
                                ArrayList<Integer> intIndexs = new ArrayList<>();
                                for(int i = 1; i < indexs.size(); i++){
                                    intIndexs.add(((ConstInteger) indexs.get(i)).getValue());
                                }

                                Value retValue = getGlobalArrValue(globalVar, intIndexs);
                                if(retValue != null){
                                    for(Use use : gepInst.getUseList()){
                                        User user = use.getUser();
                                        if(user instanceof LoadInst loadInst){
                                            loadInst.replaceUsedWith(retValue);
                                        }
                                    }
                                }
                            }

                        }
                    }
                }
            }
        }
    }

    private Value getGlobalArrValue(GlobalVar globalVar, ArrayList<Integer> indexs){
        ArrayList<Value> initVal = globalVar.getValues();
        //  全局数组的init value应该都是常数
        ArrayType arrayType = (ArrayType) ((PointerType)globalVar.getType()).getEleType();

        if(globalVar.isAllZero()){
            if(arrayType.isIntegerArr()){
                return f.buildNumber(0);
            }
            else return f.buildNumber((float) 0.0);
        }

        ArrayList<Integer> numList = arrayType.getNumList();
        //  如果gep的是一个i32*，那么indexSize应该正好等于维度的size
        if(numList.size() != indexs.size()){
            return null;
        }

        ArrayList<Integer> mulFactors = new ArrayList<>();
        mulFactors.add(0, 1);
        for(int i = numList.size() - 1; i > 0; i--){
            int factor = mulFactors.get(0);
            mulFactors.add(0, factor * numList.get(i));
        }

        int totIndex = 0;
        for(int i = 0; i < indexs.size(); i++){
            totIndex += indexs.get(i) * mulFactors.get(i);
        }

        return initVal.get(totIndex);
    }

    @Override
    public String getName() {
        return "ConstArrayFold";
    }
}
