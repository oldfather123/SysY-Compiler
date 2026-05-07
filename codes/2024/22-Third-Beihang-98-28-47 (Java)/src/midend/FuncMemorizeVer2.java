package midend;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.instr.binop.MulInstr;
import frontend.ir.instr.binop.SRemInstr;
import frontend.ir.instr.convop.Fp2Si;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.EmptyInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.otherop.cmp.Cmp;
import frontend.ir.instr.otherop.cmp.CmpCond;
import frontend.ir.instr.otherop.cmp.ICmpInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.FParam;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;
import frontend.ir.symbols.ArrayInitVal;
import frontend.ir.symbols.SymTab;
import frontend.ir.symbols.Symbol;

import java.util.ArrayList;
import java.util.List;

/**
 * （递归）函数记忆化，通过全局数组减少递归运算
 * 想要添加 phi,可以放在 mem2reg 之后
 * 仍然需要返回块,需要在合并块之前
 * 这是第二版,决定将哈希计算放在函数调用操作之前而不是被调用者的开头
 */
public class FuncMemorizeVer2 {
    private static final int hash_base = 100;
    private static final int hash_base2 = 514;
    private static final int hash_mod = 65536;
    private static final int hash_mod2 = 10086;
    
    public static void execute(ArrayList<Function> functions, SymTab globalSymTab) {
        for (Function function : functions) {
            if (function.canBeMemorized()) {
                // 标记函数被做过记忆化
                function.setMemorized();
                
                // 建立两个全局数组，data 和 used
                DataType funcRetType = function.getDataType();
                ArrayList<Integer> globalDataLimList = new ArrayList<>();
                globalDataLimList.add(hash_mod);
                ArrayInitVal globalDataInit = new ArrayInitVal(funcRetType, globalDataLimList, true);
                Symbol globalData = new Symbol("global_data_" + function.getName(), funcRetType,
                        globalDataLimList, false, true, globalDataInit);
                globalSymTab.addSym(globalData);
                
                ArrayList<Integer> globalUsedLimList = new ArrayList<>();
                globalUsedLimList.add(hash_mod);
                ArrayInitVal globalUsedInit = new ArrayInitVal(DataType.INT, globalUsedLimList, true);
                Symbol globalUsed = new Symbol("global_used_" + function.getName(), DataType.INT,
                        globalUsedLimList, false, true, globalUsedInit);
                globalSymTab.addSym(globalUsed);
                
                // 针对每一次递归调用做一次哈希
                ArrayList<CallInstr> recursiveCallInstrList = getRecursiveCallInstr(function);
                for (CallInstr call : recursiveCallInstrList) {
                    // 将 call 所在基本块一分为三
                    ArrayList<BasicBlock> blks = splitBlk(call);
                    
                    // 建立两个基本块，并准备两个常数对象
                    BasicBlock hashBlk = new BasicBlock(0, function.getAndAddBlkIndex());
                    BasicBlock loadBlk  = new BasicBlock(0, function.getAndAddBlkIndex());
                    hashBlk.setTag("hashBlk");
                    loadBlk.setTag("hashRetBlk");
                    
                    ConstInt baseVal  = new ConstInt(hash_base);
                    ConstInt modVal   = new ConstInt(hash_mod);
                    
                    // 将所有的参数转化为整数以备哈希
                    ArrayList<Value> castArgs = new ArrayList<>();
                    for (Value param : call.getRParams()) {
                        if (param.getDataType() == DataType.INT) {
                            castArgs.add(param);
                        } else {
                            // 浮点数乘以一百放大差异
                            // FMulInstr fMul = new FMulInstr(function.getAndAddRegIndex(), fParam, new ConstFloat(100f));
                            // hashBlk.addInstruction(fMul);
                            
                            Fp2Si cast = new Fp2Si(function.getAndAddRegIndex(), param);
                            hashBlk.addInstruction(cast);
                            castArgs.add(cast);
                        }
                    }
                    
                    // 哈希计算
                    Value hashVal  = hashCal(hashBlk, function, castArgs, modVal,  baseVal);
//                Value hashVal2 = hashCal(hashBlk, function, castArgs, modVal2, baseVal2);
                    
                    // 找到数组对应的位置并检查是否已经计算过
                    ArrayList<Value> indexList = new ArrayList<>();
                    indexList.add(hashVal);
                    GEPInstr dataPtr = new GEPInstr(function.getAndAddRegIndex(), new ArrayList<>(indexList), globalData);
                    GEPInstr usedPtr = new GEPInstr(function.getAndAddRegIndex(), new ArrayList<>(indexList), globalUsed);
                    hashBlk.addInstruction(dataPtr);
                    hashBlk.addInstruction(usedPtr);
                    
                    // 检查一下之前有没有算过这个值，如果算过准备替换，否则正常开始递归调用
                    LoadInstr usedVal = new LoadInstr(function.getAndAddRegIndex(), globalUsed, usedPtr);
                    hashBlk.addInstruction(usedVal);
                    Cmp usedCond = new ICmpInstr(function.getAndAddRegIndex(), CmpCond.NE, usedVal, castArgs.get(0));
                    hashBlk.addInstruction(usedCond);
                    BranchInstr branch = new BranchInstr(usedCond, blks.get(1), loadBlk); // 分别对应要递归计算和不要递归计算
                    hashBlk.addInstruction(branch);
                    
                    // 构建直接返回块：取出对应的记录结果并替换 call 的 use (加个phi)
                    LoadInstr dataVal = new LoadInstr(function.getAndAddRegIndex(), globalData, dataPtr);
                    loadBlk.addInstruction(dataVal);
                    loadBlk.addInstruction(new JumpInstr(blks.get(2)));
                    
                    EmptyInstr fromCall = new EmptyInstr(call.getDataType());
                    ArrayList<Value> values = new ArrayList<>();
                    values.add(fromCall);
                    values.add(dataVal);
                    ArrayList<BasicBlock> prtBlks = new ArrayList<>();
                    prtBlks.add(blks.get(1));
                    prtBlks.add(loadBlk);
                    PhiInstr phi = new PhiInstr(function.getAndAddPhiIndex(), call.getDataType(), values, prtBlks);
                    call.replaceUseTo(phi);
                    phi.modifyUse(fromCall, call);
                    blks.get(2).addInstrToHead(phi);
                    
                    // 在返回之后加 store 存储计算数值
                    StoreInstr storeData = new StoreInstr(call, globalData, dataPtr);
                    storeData.insertAfter(call);
                    StoreInstr storeUsed = new StoreInstr(castArgs.get(0), globalUsed, usedPtr);
                    storeUsed.insertAfter(call);
                    
                    // 将两个新块插入函数
                    hashBlk.insertAfter(blks.get(0));
                    blks.get(0).setRet(false);
                    blks.get(0).addInstruction(new JumpInstr(hashBlk));
                    
                    loadBlk.insertAfter(blks.get(1));
                }
                
                // 重新将 alloca 提前到新的头
                function.allocaRearrangement();
            }
        }
    }
    
    private static Value hashCal(BasicBlock hashBlk, Function function, List<Value> args, Value modVal, Value baseVal) {
        Value hashVal = args.get(0);
        
        hashVal = new SRemInstr(function.getAndAddRegIndex(), hashVal, modVal);
        hashBlk.addInstruction((Instruction) hashVal);
        
        if (args.size() > 1) {
            for (int i = 1; i < args.size(); i++) {
                hashVal = new MulInstr(function.getAndAddRegIndex(), hashVal, baseVal);
                hashBlk.addInstruction((Instruction) hashVal);
                hashVal = new AddInstr(function.getAndAddRegIndex(), hashVal, args.get(i));
                hashBlk.addInstruction((Instruction) hashVal);
                hashVal = new SRemInstr(function.getAndAddRegIndex(), hashVal, modVal);
                hashBlk.addInstruction((Instruction) hashVal);
            }
        }
        
        return hashVal;
    }
    
    private static ArrayList<CallInstr> getRecursiveCallInstr(Function function) {
        ArrayList<CallInstr> recursiveCallInstrList = new ArrayList<>();
        BasicBlock basicBlock = (BasicBlock) function.getBasicBlocks().getHead();
        while (basicBlock != null) {
            Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
            while (instruction != null) {
                if (instruction instanceof CallInstr && ((CallInstr) instruction).getFuncDef().equals(function)) {
                    recursiveCallInstrList.add((CallInstr) instruction);
                }
                instruction = (Instruction) instruction.getNext();
            }
            basicBlock = (BasicBlock) basicBlock.getNext();
        }
        return recursiveCallInstrList;
    }
    
    /**
     * 将以一条 call 指令为界分为: 前, call, 后, 三个块
     */
    private static ArrayList<BasicBlock> splitBlk(CallInstr call) {
        ArrayList<BasicBlock> blks = new ArrayList<>();
        BasicBlock prevBlk = call.getParentBB();
        Procedure procedure = (Procedure) prevBlk.getParent().getOwner();
        Instruction ins = (Instruction) call.getNext();
        call.removeFromListWithUseRemain();
        
        BasicBlock callBlk = new BasicBlock(prevBlk.getLoopDepth(), procedure.getAndAddBlkIndex());
        BasicBlock nextBlk = new BasicBlock(prevBlk.getLoopDepth(), procedure.getAndAddBlkIndex());
        callBlk.insertAfter(prevBlk);
        nextBlk.insertAfter(callBlk);
        
        // 把原来块对应的 phi 全换成对最后块的
        for (BasicBlock suc : prevBlk.getSucs()) {
            Instruction phi = (Instruction) suc.getInstructions().getHead();
            while (phi instanceof PhiInstr) {
                ((PhiInstr) phi).modifyPrtBlk(prevBlk, nextBlk);
                phi = (Instruction) phi.getNext();
            }
        }
        
        callBlk.addInstruction(call);
        callBlk.addInstruction(new JumpInstr(nextBlk));
        
        while (ins != null) {
            Instruction tmp = (Instruction) ins.getNext();
            if (ins instanceof Terminator) {
                ins.removeFromList();
                nextBlk.addInstruction(ins.cloneShell(procedure));
            } else {
                ins.removeFromListWithUseRemain();
                nextBlk.addInstruction(ins);
            }
            ins = tmp;
        }
        
        blks.add(prevBlk);
        blks.add(callBlk);
        blks.add(nextBlk);
        
        return blks;
    }
}
