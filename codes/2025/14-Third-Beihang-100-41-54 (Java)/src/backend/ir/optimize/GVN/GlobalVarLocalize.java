package backend.ir.optimize.GVN;

import backend.ir.entity.*;
import backend.ir.entity.insts.GetElemPtrInst;
import backend.ir.entity.insts.Inst;
import backend.ir.entity.insts.LoadInst;
import backend.ir.entity.insts.StoreInst;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;

public class GlobalVarLocalize {

    private final Program program;

    private final boolean optimize;

    public GlobalVarLocalize(Program program,boolean optimize){
        this.program = program;this.optimize = optimize;
    }

    public void runOptimize(){
        for(Value value : program.getUsees()){
            if(value instanceof GlobalVar){
                Set<LoadInst> removeLoadSet = globalVarForConst((GlobalVar) value);
                if(!removeLoadSet.isEmpty()){
                    for(LoadInst loadInst : removeLoadSet) removeUselessLoadInst(loadInst,(LiteralConst) ((GlobalVar)value).getUsee(0));
                }
            }
        }
    }

    /**
     * 将全局只读变量削减为Const
     * @param globalVar 全局变量
     */
    public Set<LoadInst> globalVarForConst(GlobalVar globalVar){
        boolean isReadOnly = true;
        if(globalVar.isArray()) return new HashSet<>();
        for(Value value : globalVar.getUsers()){
            assert value instanceof Inst;
            if(value instanceof StoreInst){
                isReadOnly = false; break;
            }
        }
        if(isReadOnly){
            Set<LoadInst> removeLoadInstSet = new HashSet<>();
            assert globalVar.getUsee(0) instanceof LiteralConst;
            for(Value value : globalVar.getUsers()){
                assert value instanceof LoadInst;
                removeLoadInstSet.add((LoadInst) value);
            }
            return removeLoadInstSet;
        }
        else return new HashSet<>();
    }

    public void removeUselessLoadInst(LoadInst loadInst,LiteralConst literalConst){
        for(Value value : program.getUsees()){
            if(value instanceof Function function){
                for(BasicBlock basicBlock : function.getAllBasicBlocks()){
                   for(Value useValue : basicBlock.getUsees()){
                       if(useValue instanceof Inst){
                           ((Inst)(useValue)).replaceWithNewValue(loadInst,literalConst);
                       }
                   }
                   for(Value useValue : basicBlock.getUsees()){
                       if(useValue instanceof LoadInst && loadInst.getName().equals(useValue.getName())){
                           basicBlock.getUsees().remove(useValue);break;
                       }
                   }
               }
            }
        }
    }

    //TODO:其余的全局变量局部化处理： 1: 非递归的全局变量局部化等

}
