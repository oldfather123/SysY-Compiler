package Pass.IR;

import IR.IRBuildFactory;
import IR.IRModule;
import IR.Type.IntegerType;
import IR.Use;
import IR.Value.*;
import IR.Value.Instructions.*;
import Pass.IR.Utils.AliasAnalysis;
import Pass.IR.Utils.UtilFunc;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;
import java.util.HashSet;

//  PeepHole中都是一些对特定模式的优化
public class PeepHole implements Pass.IRPass {

    private final IRBuildFactory f = IRBuildFactory.getInstance();

    @Override
    public void run(IRModule module) {
        // A = icmp %0, %1; B = icmp ne A, 0
        // A.replaceUsedWith(B)
        PeepHole3(module);
        // store A ptr1; ...; B = load ptr1;
        PeepHole4(module);
        //  全局数组初始化为0
        PeepHole5(module);
        //  < a || > -a
        PeepHole6(module);
        //  10条指令范围内进行store/load替换
//        PeepHole8(module);
    }

    private void PeepHole8(IRModule module){
        for(Function function : module.getFunctions()){
            for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
                BasicBlock bb = bbNode.getValue();
                IList.INode<Instruction, BasicBlock> instNode = bb.getInsts().getHead();
                ArrayList<StoreInst> storeInsts = new ArrayList<>();
                while (instNode != null){
                    Instruction inst = instNode.getValue();
                    instNode = instNode.getNext();
                    if(inst instanceof StoreInst storeInst){
                        storeInsts.add(storeInst);
                    }
                }

                for(StoreInst storeInst : storeInsts){
                    ArrayList<Instruction> mayLoadInsts = new ArrayList<>();
                    IList.INode<Instruction, BasicBlock> itInstNode = storeInst.getNode().getNext();
                    int cnt = 0;
                    while (itInstNode != null && cnt < 10){
                        mayLoadInsts.add(itInstNode.getValue());
                        itInstNode = itInstNode.getNext();
                        cnt = cnt + 1;
                    }

                    for(Instruction itInst : mayLoadInsts){
                        if(itInst instanceof CallInst){
                            break;
                        }
                        else if(itInst instanceof LoadInst loadInst){
                            if(storeInst.getPointer().equals(loadInst.getPointer())){
                                loadInst.replaceUsedWith(storeInst.getValue());
                                loadInst.removeSelf();
                            }
                        }
                        else if(itInst instanceof StoreInst storeInst2){
                            if(AliasAnalysis.isMaySame(storeInst.getPointer(), storeInst2.getPointer())){
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    private void PeepHole6(IRModule module){
        for(Function function : module.getFunctions()){
            if(function.getName().equals("@main")){
                for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
                    BasicBlock bb = bbNode.getValue();
                    for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                        Instruction inst = instNode.getValue();
                        if(inst instanceof BinaryInst binInst
                                && binInst.getOp().equals(OP.FLe)
                                && binInst.getRightVal() instanceof ConstFloat constFloat){
                            BrInst brInst = null;
                            for(User user : binInst.getUserList()){
                                if(user instanceof BrInst){
                                    brInst = (BrInst) user;
                                }
                            }
                            if(brInst == null || brInst.isJump()){
                                continue;
                            }

                            BasicBlock trueBlock = brInst.getTrueBlock();
                            BasicBlock falseBlock = brInst.getFalseBlock();
                            BrInst brInst2 = (BrInst) falseBlock.getLastInst();
                            if(brInst2 == null || brInst2.isJump()){
                                continue;
                            }
                            if(!(brInst2.getJudVal() instanceof BinaryInst binInst2
                                    && binInst2.getOp().equals(OP.FGe)
                                    && binInst2.getLeftVal().equals(binInst.getLeftVal())
                                    && binInst2.getRightVal() instanceof ConstFloat constFloat2)){
                                continue;
                            }
                            if(constFloat.getValue() < 0
                                    || constFloat.getValue() != -constFloat2.getValue()){
                                continue;
                            }
                            if(!trueBlock.equals(brInst2.getTrueBlock())){
                                continue;
                            }

                            binInst.replaceUsedWith(f.buildI1Number(1));
                        }
                    }
                }
            }
        }
    }

    private void PeepHole5(IRModule module){
        for(Function function : module.getFunctions()){
            if(function.getName().equals("@main")){
                BasicBlock bb = function.getBbEntry();
                for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                    Instruction inst = instNode.getValue();
                    if(instNode.getNext() == null) break;
                    Instruction nxtInst = instNode.getNext().getValue();
                    if(inst instanceof GepInst gepInst
                            && gepInst.getTarget() instanceof GlobalVar globalVar
                            && globalVar.isAllZero()
                            && nxtInst instanceof StoreInst storeInst
                            && storeInst.getValue() instanceof ConstInteger constInt
                            && constInt.getValue() == 0){
                        storeInst.removeSelf();
                    }
                }
            }
        }
    }

    private void PeepHole4(IRModule module){
        for(Function function : module.getFunctions()){
            for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
                BasicBlock bb = bbNode.getValue();
                ArrayList<Instruction> memInsts = new ArrayList<>();
                for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                    Instruction inst = instNode.getValue();
                    if(inst instanceof StoreInst || inst instanceof LoadInst){
                        memInsts.add(inst);
                    }
                }

                for(int i = 0; i < memInsts.size() - 1; i++){
                    Instruction inst = memInsts.get(i);
                    Instruction nxtInst = memInsts.get(i + 1);
                    if(inst instanceof StoreInst storeInst && nxtInst instanceof LoadInst loadInst
                            && storeInst.getPointer().equals(loadInst.getPointer())){
                        Value value = storeInst.getValue();
                        loadInst.replaceUsedWith(value);
                        loadInst.removeSelf();
                    }
                }
            }
        }
    }

    private void PeepHole3(IRModule module){
        for(Function function : module.getFunctions()){
            for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
                BasicBlock bb = bbNode.getValue();
                ArrayList<CmpInst> cmpInsts = new ArrayList<>();
                for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                    Instruction inst = instNode.getValue();
                    if(inst instanceof CmpInst cmpInst){
                        cmpInsts.add(cmpInst);
                    }
                }

                for(CmpInst cmpInst : cmpInsts){
                    if(!cmpInst.getOp().equals(OP.Ne)){
                        continue;
                    }
                    Value left = cmpInst.getLeftVal();
                    Value right = cmpInst.getRightVal();
                    if(left instanceof CmpInst){
                        if(right instanceof ConstInteger constInteger
                                && constInteger.getValue() == 0){
                            cmpInst.replaceUsedWith(left);
                            cmpInst.removeSelf();
                        }
                    }
                    else if(right instanceof CmpInst){
                        if(left instanceof ConstInteger constInteger
                                && constInteger.getValue() == 0){
                            cmpInst.replaceUsedWith(right);
                            cmpInst.removeSelf();
                        }
                    }
                }
            }
        }
    }

    @Override
    public String getName() {
        return "PeepHole";
    }
}
