package Pass.IR;

import Frontend.AST;
import IR.IRModule;
import IR.Value.BasicBlock;
import IR.Value.Const;
import IR.Value.Function;
import IR.Value.Instructions.BrInst;
import IR.Value.Instructions.CallInst;
import IR.Value.Instructions.Instruction;
import IR.Value.Instructions.RetInst;
import IR.Value.Value;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.HashSet;

//  当ret和putint/putch都不为变量时，进行激进的死代码删除
public class AggressiveDCE implements Pass.IRPass {

    @Override
    public String getName() {
        return "AggressiveDCE";
    }

    @Override
    public void run(IRModule module) {
        HashSet<Instruction> usefulInsts = new HashSet<>();
        BasicBlock endBb = null;
        for(Function function : module.getFunctions()){
            if(function.getName().equals("@main")){
                for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
                    BasicBlock bb = bbNode.getValue();
                    for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                        Instruction inst = instNode.getValue();
                        if(inst instanceof CallInst callInst){
                            String funcName = callInst.getFunction().getName();
                            //  库函数不在entry的情况太复杂了，暂时不做
                            if(funcName.equals("@getint") || funcName.equals("@getch")
                                    || function.getName().equals("@getarray")){
                                if(!inst.getParentbb().equals(function.getBbEntry())){
                                    return ;
                                }
                            }
                            else if(funcName.equals("@putint") || funcName.equals("@putch")
                                    || funcName.equals("@putarray")){
                                usefulInsts.add(callInst);
                                if(endBb == null){
                                    endBb = callInst.getParentbb();
                                }
                                else if(!endBb.equals(callInst.getParentbb())){
                                    return;
                                }
                            }
                        }
                        else if(inst instanceof RetInst retInst){
                            usefulInsts.add(retInst);
                            if(endBb == null){
                                endBb = retInst.getParentbb();
                            }
                            else if(!endBb.equals(retInst.getParentbb())){
                                return;
                            }
                        }
                    }
                }
            }
            if(endBb == null){
                return;
            }

            for(Instruction inst : usefulInsts){
                for(Value value : inst.getUseValues()){
                    if(!(value instanceof Const)){
                        return;
                    }
                }
            }

            BasicBlock bbEntry = function.getBbEntry();
            if(bbEntry.equals(endBb)){
                return;
            }
            BrInst brInst = (BrInst) bbEntry.getLastInst();
            if(!brInst.isJump()){
                return;
            }

            brInst.setJumpBlock(endBb);
            bbEntry.getNxtBlocks().clear();
            bbEntry.setNxtBlock(endBb);
            endBb.getPreBlocks().clear();
            endBb.setPreBlock(bbEntry);
        }
    }
}
