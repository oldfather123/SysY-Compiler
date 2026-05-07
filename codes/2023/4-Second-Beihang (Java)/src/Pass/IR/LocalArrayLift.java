package Pass.IR;

import IR.IRBuildFactory;
import IR.IRModule;
import IR.Type.ArrayType;
import IR.Type.PointerType;
import IR.Type.Type;
import IR.Use;
import IR.Value.*;
import IR.Value.Instructions.*;
import Pass.IR.Utils.LoopAnalysis;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;

public class LocalArrayLift implements Pass.IRPass {
    private final IRBuildFactory f = IRBuildFactory.getInstance();
    private IRModule module;

    @Override
    public void run(IRModule module){
        this.module = module;
        for(Function function : module.getFunctions()){
            if(function.getName().equals("@main") || function.getCallerList().size() == 1){
                runLocalArrayLift(function);
            }
        }
    }


    private void runLocalArrayLift(Function function){
        LoopAnalysis.runLoopInfo(function);
        for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
            BasicBlock bb = bbNode.getValue();
            if(bb.getLoopDepth() != 0){
                continue;
            }
            //  不在循环内的localArray可以提升
            //  TODO: 如何将初始值为常数的情况下填充初始值
            IList.INode<Instruction, BasicBlock> itInstNode = bb.getInsts().getHead();
            while (itInstNode != null){
                Instruction inst = itInstNode.getValue();
                itInstNode = itInstNode.getNext();
                if(inst instanceof AllocInst allocInst){
                    PointerType pointerType = (PointerType) allocInst.getType();
                    Type eleType = pointerType.getEleType();
                    if(eleType instanceof ArrayType arrayType){
                        if(!arrayType.isIntegerArr()){
                            continue;
                        }
                        int size = arrayType.getTotalSize();
                        //  初始值清零
                        ArrayList<Value> initValues = new ArrayList<>();
                        for (int i = 0; i < size; i++) {
                            initValues.add(f.buildNumber(0));
                        }
                        //  这里要删除存储初始值的相关指令
                        //  据visitor中的写法，大致是这样的顺序：
                        //  1. gep tar 0 ... 0
                        //  2. optional: bitcast
                        //  3. call memset
                        //  4. valueSize * (gep + store)

                        //  1. 处理gepInst
                        Instruction gepInst = itInstNode.getValue();
                        itInstNode = itInstNode.getNext();
                        gepInst.removeSelf();
                        //  2. 处理bitCast+call memset
                        Instruction memset = itInstNode.getValue();
                        itInstNode = itInstNode.getNext();
                        memset.removeSelf();
                        //  3. 成对处理gep+store
                        ArrayList<Value> oriInitValue = allocInst.getInitValues();
                        for(int i = 0; i < oriInitValue.size(); i++){
                            //  Visitor中填充的值不会构建gep+store
                            if(oriInitValue.get(i) == null){
                                continue;
                            }
                            //  Gep
                            GepInst gepInst1 = (GepInst) itInstNode.getValue();
                            itInstNode = itInstNode.getNext();
                            StoreInst storeInst = (StoreInst) itInstNode.getValue();
                            itInstNode = itInstNode.getNext();
                            if(storeInst.getValue() instanceof Const){
                                initValues.set(i, storeInst.getValue());
                                gepInst1.removeSelf();
                                storeInst.removeSelf();
                            }
                        }

                        String name = "lift_" + allocInst.getName().replace("%", "");
                        GlobalVar globalVar = f.getGlobalArray(name, arrayType, initValues);
                        module.addGlobalVar(globalVar);

                        allocInst.replaceUsedWith(globalVar);
                        allocInst.removeSelf();
                    }
                }
            }
        }
    }

    @Override
    public String getName(){
        return "LocalArrayLift";
    }
}
