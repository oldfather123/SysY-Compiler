package backend.ir.optimize.GVN;

import backend.ir.entity.*;
import backend.ir.entity.insts.*;
import frontend.lexer.entity.Token;
import frontend.lexer.entity.TokenType;
import javax.print.DocFlavor;
import javax.swing.*;
import java.lang.reflect.AnnotatedArrayType;
import java.util.*;

public class GlobalVarNumbering {

    private final Program program;

    private final boolean optimize;

    public GlobalVarNumbering(Program program,boolean optimize){
        this.program = program;this.optimize = optimize;
    }

    public int runOptimize(){
        program.generateDominantTree();
        int sumModifyNumber = 0;
        for(Value value : program.getUsees()){
            if(value instanceof Function){
                sumModifyNumber += runBasicBlockGVN(((Function)value).getSuperBlock(),new HashMap<>(),new HashSet<>());
            }
        }
        return sumModifyNumber;
    }

    public int runBasicBlockGVN(BasicBlock basicBlock,Map<String,Inst> NumberingMap,Set<BasicBlock> basicBlockSet){
        Map<String, Inst> globalVarNumberingMap = new HashMap<>(NumberingMap);

        int modifyNumber = 0;
        if(basicBlockSet.contains(basicBlock)) return modifyNumber;
        basicBlockSet.add(basicBlock);
        for(Inst instruction : basicBlock.getInstructions()){
            //TODO: 常量折叠接口
            if(isGVNInstruction(instruction)){
                String GVNEmbedding = GVNEmbeddingInst(instruction);
                if(globalVarNumberingMap.containsKey(GVNEmbedding)){
                    //TODO: 加入包菜算法
                    if(instruction instanceof LoadInst){
                        boolean hasModify = foundIsValueChangeInTheRoute(instruction.getUsee(0),globalVarNumberingMap.get(GVNEmbedding),instruction);
                        if(!hasModify){
                            Value replaceValue = globalVarNumberingMap.get(GVNEmbedding);
                            for(Value value : instruction.getUsers()){
                                if(value instanceof Inst) ((Inst)(value)).replaceWithNewValue(instruction,replaceValue);
                            }
                            basicBlock.getUsees().remove(instruction);
                            modifyNumber ++ ;
                        }
                        else globalVarNumberingMap.put(GVNEmbedding,instruction);
                        //globalVarNumberingMap.put(GVNEmbedding,instruction);
                    }
                    else{
                        Value replaceValue = globalVarNumberingMap.get(GVNEmbedding);
                        for(Value value : instruction.getUsers()){
                            if(value instanceof Inst) ((Inst)(value)).replaceWithNewValue(instruction,replaceValue);
                        }
                        basicBlock.getUsees().remove(instruction);
                        modifyNumber ++ ;
                    }
                }
                else globalVarNumberingMap.put(GVNEmbedding,instruction);
            }
        }
        if(basicBlock.getOutBlocks() == null ) return modifyNumber;
        for(BasicBlock nextBasicBlock : basicBlock.getChildDominateBlocks()){
            modifyNumber += runBasicBlockGVN(nextBasicBlock,globalVarNumberingMap,basicBlockSet);
        }
        return modifyNumber;
    }

    public boolean isGVNInstruction(Inst instruction){
        switch (instruction) {
            case AllocateInst allocateInst -> {return false;}
            case InputInst inputInst -> {return false;}
            case StoreInst storeInst -> {return false;}
            case OutputInst outputInst -> {return false;}
            case PCopyInst pCopyInst -> {return false;}
            case PHIInst phiInst -> {return false;}
            case RetInst retInst -> {return false;}
            case TransInst transInst -> {return false;}
            case UCJumpInst ucJumpInst -> {return false;}
            case CallInst callInst -> {
                /*if(callInst.getUsee(0) instanceof Function){
                    Function function = (Function) instruction.getUsees().getFirst();
                    return function.isGVNInstruction();
                }
                else return false;*/
                return false;
            }
            //那么，可以进行GVN的指令只包括：
            //1、BinOPInst 2、CallInst 3、CmpInst 4、GetElemPtrInst 5、LoadInst
            case null, default -> {
                return true;
            }
        }
    }

    public String GVNEmbeddingInst(Inst inst){
        switch (inst){
            case BinOpInst binOpInst ->{
                TokenType opType = binOpInst.opType;
                String valueName1 = binOpInst.getUsee(0).getName(),valueName2 = binOpInst.getUsee(1).getName();

                if(opType.equals(TokenType.ADD) || opType.equals(TokenType.MUL)){
                    if(valueName1.compareTo(valueName2) < 0){
                        String valueName3 = valueName1;valueName1 = valueName2;valueName2 = valueName3;
                    }
                }
                return valueName1 + "_" + opType.content + "_" + valueName2;
            }
            case CmpInst cmpInst ->{
                TokenType opType = cmpInst.opType;
                String valueName1 = cmpInst.getUsee(0).getName(),valueName2 = cmpInst.getUsee(1).getName();

                if(opType.equals(TokenType.GT)){
                    opType = TokenType.LE;
                    String valueName3 = valueName1;valueName1 = valueName2;valueName2 = valueName3;
                }
                else if(opType.equals(TokenType.GE)){
                    opType = TokenType.LT;
                    String valueName3 = valueName1;valueName1 = valueName2;valueName2 = valueName3;
                }
                return valueName1 + "_" + opType.content + "_" + valueName2;
            }
            case LoadInst loadInst ->{
                return loadInst.getUsee(0).getName();
            }
            case GetElemPtrInst getElemPtrInst ->{
                StringBuilder emBeddingName = new StringBuilder(getElemPtrInst.getUsee(0).getName());
                for(int index = 1;index < getElemPtrInst.getUsees().size();index ++ ){
                    emBeddingName.append("[");
                    emBeddingName.append(getElemPtrInst.getUsee(index).getName());
                    emBeddingName.append("]");
                }
                return emBeddingName.toString();
            }
            case CallInst callInst -> {
                Function function = (Function)(callInst.getUsee(0));
                StringBuilder embeddingName = new StringBuilder(function.getName());
                embeddingName.append("(");
                for(Argument argument : function.getArguments()) embeddingName.append(argument.getName()).append(",");
                embeddingName.append(")");
                return embeddingName.toString();
            }
            default -> {return inst.toString();}
        }
    }

    public boolean foundIsValueChangeInTheRoute(Value value,Inst a,Inst b){
        BasicBlock basicBlockA = a.getBasicBlock();
        BasicBlock basicBlockB = b.getBasicBlock();
        Set<BasicBlock> routeBlockSetA = findPossibleOutBasicBlock(basicBlockA,new HashSet<>());
        Set<BasicBlock> routeBlockSetB = findPossibleInBasicBlock(basicBlockB,new HashSet<>());;
        Set<BasicBlock> commonBasicBlockSet = mergeTwoSet(routeBlockSetA,routeBlockSetB);
        commonBasicBlockSet.add(basicBlockA);commonBasicBlockSet.add(basicBlockB);

        boolean hasModify = false,hasMove = false;Set<Function> functionSet = new HashSet<>();
        if(a.getName().equals(b.getName())) return true;

        for(BasicBlock basicBlock : commonBasicBlockSet){
            List<Inst> instructionList = basicBlock.getInstructions();
            for(int index = 0;index < instructionList.size();index ++ ){
                if(basicBlock.equals(basicBlockB) && instructionList.get(index).equals(b)) break;
                else if(basicBlock.equals(basicBlockA) && !hasMove){
                    while(index < instructionList.size() && !instructionList.get(index).getName().equals(a.getName())) index ++ ;
                    hasMove = true;
                }
                else hasModify = findIsInstModify(basicBlock, instructionList.get(index), value, functionSet);
                if(hasModify) return hasModify;
            }
        }
        return hasModify;
    }

    public Set<BasicBlock> findPossibleOutBasicBlock(BasicBlock basicBlock, Set<BasicBlock> basicBlockSet){
        for(BasicBlock nexBasicBlock : basicBlock.getOutBlocks()){
            if(!basicBlockSet.contains(nexBasicBlock)){
                basicBlockSet.add(nexBasicBlock);
                basicBlockSet.addAll(findPossibleOutBasicBlock(nexBasicBlock,basicBlockSet));
            }
        }
        return basicBlockSet;
    }

    public Set<BasicBlock> findPossibleInBasicBlock(BasicBlock basicBlock, Set<BasicBlock> basicBlockSet){
        for(BasicBlock nexBasicBlock : basicBlock.getInBlocks()){
            if(!basicBlockSet.contains(nexBasicBlock)){
                basicBlockSet.add(nexBasicBlock);
                basicBlockSet.addAll(findPossibleOutBasicBlock(nexBasicBlock,basicBlockSet));
            }
        }
        return basicBlockSet;
    }

    public static <F> Set<F> mergeTwoSet(Set<F> a,Set<F> b){
        Set<F> commonSet = new HashSet<>();
        for(F value : a) {
            if (b.contains(value)) commonSet.add(value);
        }
        return commonSet;
    }

    public boolean findIsInstModify(BasicBlock basicBlock,Inst inst,Value value,Set<Function> functionSet){
        if(inst instanceof StoreInst){
            Value storeValue = (inst).getUsee(1);
            return storeValue.getName().equals(value.getName());
        }
        else if(inst instanceof CallInst){
            if(inst.getUsee(0) instanceof Function){
                Function function = (Function)(inst.getUsee(0));
                if(!functionSet.contains(function)) return true;
                else{
                    functionSet.add(function);
                    boolean isModify = false;
                    for(BasicBlock functionBasic : function.getAllBasicBlocks()){
                        for(Inst instruction : functionBasic.getInstructions()){
                            isModify |= findIsInstModify(functionBasic,instruction,value,functionSet);
                        }
                    }
                    return isModify;
                }
            }
            else return true;
        }
        else return false;
    }
}
