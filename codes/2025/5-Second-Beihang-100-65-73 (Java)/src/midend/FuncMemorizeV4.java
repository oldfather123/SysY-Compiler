package midend;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.instr.binop.MulInstr;
import frontend.ir.instr.binop.SRemInstr;
import frontend.ir.instr.convop.Fp2Si;
import frontend.ir.instr.convop.ZextInstr;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.objecttype.derived.TArray;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.symbol.ArrayInitVal;
import frontend.ir.symbol.GlbObjPtr;
import frontend.ir.symbol.SymTab;
import frontend.ir.symbol.Symbol;

import java.util.ArrayList;
import java.util.List;

/**
 * 说明：此优化自我们的前作《海底小分队》迁移
 * （递归）函数记忆化，通过全局数组减少递归运算
 * 想要添加 phi,可以放在 mem2reg 之后
 * 仍然需要返回块,需要在合并块之前
 * 这是第二版,决定将哈希计算放在函数调用操作之前而不是被调用者的开头
 * 注意，这一版需要放在内联后面，还需要做全局标量装填
 */
public class FuncMemorizeV4 {
    // TODO: 数组参数也要记忆化 - 要做一个针对在main中唯一调用（在初始化完成后只读）且只读全局数组/只读数组参数的记忆化
    private static final int hash_base = 128;
    private static final int hash_base2 = 514;
    private static final int hash_mod = 65536;
    private static final int hash_mod2 = 10086;
    public static final int MAX_PARAMS = 4;
    public static final int EXEMPT_INSTR_NUM = 256;

    public static void execute(List<Function> functions, SymTab globalSymTab) {
        for (Function function : functions) {
            if (function.canBeMemorized()) {
                // 标记函数被做过记忆化
                function.setMemorized(true);

                int paramNum = function.getFParams().size();

                // 建立三个全局数组，data 和 used, params
                Type funcRetType = function.getType();

                TArray globalArrayDataType = new TArray(funcRetType, hash_mod);
                String ident = "global_data_" + function.getName();
                Value pointerData = new GlbObjPtr(globalArrayDataType, ident);
                ArrayInitVal globalDataInit = new ArrayInitVal.ArrayZeroInitVal(globalArrayDataType);
                Symbol globalData = new Symbol(ident,
                        false, true, globalArrayDataType, globalDataInit, pointerData);
                globalSymTab.addObject(globalData, -1);

                TArray globalArrayUsedType = new TArray(new TInt(), hash_mod);
                String identUsed = "global_used_" + function.getName();
                Value pointerUsed = new GlbObjPtr(globalArrayUsedType, identUsed);
                ArrayInitVal globalUsedInit = new ArrayInitVal.ArrayZeroInitVal(globalArrayUsedType);
                Symbol globalUsed = new Symbol(identUsed, false,
                        true, globalArrayUsedType, globalUsedInit, pointerUsed);
                globalSymTab.addObject(globalUsed, -1);

                TArray globalArrayParamsOneType = new TArray(new TInt(), MAX_PARAMS);
                TArray globalArrayParamsType = new TArray(globalArrayParamsOneType, hash_mod);
                String identParams = "global_params_" + function.getName();
                Value pointerParams = new GlbObjPtr(globalArrayParamsType, identParams);
                ArrayInitVal globalParamsInit  = new ArrayInitVal.ArrayZeroInitVal(globalArrayParamsType);
                Symbol globalParams = new Symbol(identParams, false,
                        true, globalArrayParamsType, globalParamsInit, pointerParams);
                globalSymTab.addObject(globalParams, -1);

                // 针对每一次递归调用做一次哈希
                ArrayList<CallInstr> recursiveCallInstrList = getRecursiveCallInstr(function);
                for (CallInstr call : recursiveCallInstrList) {
                    // 将 call 所在基本块一分为三
                    ArrayList<BasicBlock> blks = splitBlk(call);

                    // 建立两个基本块，并准备两个常数对象
                    BasicBlock hashBlk = new BasicBlock(function.getAndAddBlkIdx());
                    BasicBlock loadBlk  = new BasicBlock(function.getAndAddBlkIdx());
                    // 冲突检查块
                    BasicBlock checkBlk = new BasicBlock(function.getAndAddBlkIdx());

                    IntConst baseVal  = new IntConst(hash_base);
                    IntConst modVal   = new IntConst(hash_mod);

                    // 将所有的参数转化为整数以备哈希
                    ArrayList<Value> castArgs = new ArrayList<>();
                    for (Value param : call.getRParams()) {
                        if (param.getType() instanceof TInt) {
                            castArgs.add(param);
                        } else if (param.getType() instanceof TFloat){
                            Fp2Si cast = new Fp2Si(function.getAndAddRegIdx(), param, hashBlk);
                            castArgs.add(cast);
                        }
                    }

                    // 哈希计算
                    Value hashVal  = hashCal(hashBlk, function, castArgs, modVal,  baseVal);
//                Value hashVal2 = hashCal(hashBlk, function, castArgs, modVal2, baseVal2);

                    // 找到数组对应的位置并检查是否已经计算过
                    ArrayList<Value> indexList = new ArrayList<>();
                    indexList.add(new IntConst(0));
                    indexList.add(hashVal);
                    GEPInstr dataPtr = new GEPInstr(function.getAndAddRegIdx(), new ArrayList<>(indexList), pointerData, hashBlk);
                    GEPInstr usedPtr = new GEPInstr(function.getAndAddRegIdx(), new ArrayList<>(indexList), pointerUsed, hashBlk);

                    // 检查一下之前有没有算过这个值，如果算过准备替换，否则正常开始递归调用
                    LoadInstr usedVal = new LoadInstr(function.getAndAddRegIdx(), new TInt(), usedPtr, hashBlk);
                    CmpInstr usedCond = new CmpInstr(function.getAndAddRegIdx(), CmpInstr.CmpCond.INE,
                            usedVal, new IntConst(1), hashBlk);
                    new BranchInstr(usedCond, blks.get(1), checkBlk, hashBlk); // 分别对应要递归计算和不要递归计算

                    // 冲突检查
                    ArrayList<CmpInstr> cmpInstrs = new ArrayList<>();
                    ArrayList<GEPInstr> gepInstrs = new ArrayList<>();
                    for (int i = 0; i < paramNum; i++) {
                        ArrayList<Value> paramsIndexList = new ArrayList<>();
                        paramsIndexList.add(new IntConst(0));
                        paramsIndexList.add(hashVal);
                        paramsIndexList.add(new IntConst(i));
                        GEPInstr GEPofParam = new GEPInstr(function.getAndAddRegIdx(),
                                paramsIndexList, pointerParams, checkBlk);
                        LoadInstr loadParam = new LoadInstr(function.getAndAddRegIdx(), new TInt(), GEPofParam, checkBlk);
                        CmpInstr cmpParam = new CmpInstr(function.getAndAddRegIdx(), CmpInstr.CmpCond.IEQ,
                                loadParam, castArgs.get(i), checkBlk);
                        cmpInstrs.add(cmpParam);
                        gepInstrs.add(GEPofParam);
                    }
                    if (paramNum == 1) {
                        new BranchInstr(cmpInstrs.getFirst(), loadBlk, blks.get(1), checkBlk);
                    } else {
                        Value result = new ZextInstr(function.getAndAddRegIdx(), cmpInstrs.getFirst(), checkBlk);
                        for (int i = 1; i < paramNum; i++) {
                            ZextInstr zextInstrNow = new ZextInstr(function.getAndAddRegIdx(), cmpInstrs.get(i), checkBlk);
                            result = new AddInstr(function.getAndAddRegIdx(), result, zextInstrNow, checkBlk);
                        }
                        CmpInstr checker = new CmpInstr(function.getAndAddRegIdx(), CmpInstr.CmpCond.IEQ, result, new IntConst(paramNum), checkBlk);
                        new BranchInstr(checker, loadBlk, blks.get(1), checkBlk);
                    }

                    // 构建直接返回块：取出对应的记录结果并替换 call 的 use (加个phi)
                    LoadInstr dataVal = new LoadInstr(function.getAndAddRegIdx(), funcRetType, dataPtr, loadBlk);
                    new JumpInstr(blks.get(2), loadBlk);

                    PhiInstr phi = new PhiInstr(funcRetType, function.getAndAddRegIdx(), blks.get(2));
                    phi.addOperand(loadBlk, dataVal);
                    call.replaceUseWith(phi);
                    phi.addOperand(blks.get(1), call);
                    phi.insertBefore(blks.get(2).getFirstInstr());

                    // 在返回之后加 store 存储计算数值
                    StoreInstr storeData = new StoreInstr(call, dataPtr, blks.get(1));
                    storeData.setUse(call);
                    storeData.setUse(dataPtr);
                    storeData.insertAfter(call);
                    IntConst usedFlag = new IntConst(1);
                    StoreInstr storeUsed = new StoreInstr(usedFlag, usedPtr, blks.get(1));
                    storeUsed.setUse(usedFlag);
                    storeUsed.setUse(usedPtr);
                    storeUsed.insertAfter(call);

                    for (int i = 0; i < paramNum; i++) {
                        StoreInstr storeParam = new StoreInstr(castArgs.get(i), gepInstrs.get(i), blks.get(1));
                        storeParam.setUse(castArgs.get(i));
                        storeParam.setUse(gepInstrs.get(i));
                        storeParam.insertAfter(call);
                    }

                    // 将两个新块插入函数
                    hashBlk.insertAfter(blks.getFirst());
                    blks.getFirst().getLastInstr().forceRemoveFromList();
                    new JumpInstr(hashBlk, blks.getFirst());

                    loadBlk.insertAfter(blks.get(1));
                    checkBlk.insertAfter(hashBlk);
                }

                // 重新将 alloca 提前到新的头
//                function.allocaRearrangement();
            }
        }
    }

    private static Value hashCal(BasicBlock hashBlk, Function function,
                                 List<Value> args, Value modVal, Value baseVal) {
        Value hashVal = args.getFirst();

        // 初始hash -> 保证非负
        hashVal = new SRemInstr(function.getAndAddRegIdx(), hashVal, modVal, hashBlk);
        hashVal = makeNonNegative(hashVal, modVal, function, hashBlk);

        if (args.size() > 1) {
            for (int i = 1; i < args.size(); i++) {
                // hash = (hash * base + arg[i]) % mod
                hashVal = new MulInstr(function.getAndAddRegIdx(), hashVal, baseVal, hashBlk);
                hashVal = new AddInstr(function.getAndAddRegIdx(), hashVal, args.get(i), hashBlk);
                hashVal = new SRemInstr(function.getAndAddRegIdx(), hashVal, modVal, hashBlk);
                hashVal = makeNonNegative(hashVal, modVal, function, hashBlk);
            }
        }

        return hashVal;
    }

    /**
     * 强制取模结果为非负
     */
    private static Value makeNonNegative(Value val, Value modVal,
                                         Function function, BasicBlock blk) {
        // tmp = val + mod
        Value tmp = new AddInstr(function.getAndAddRegIdx(), val, modVal, blk);
        // hash = tmp % mod
        return new SRemInstr(function.getAndAddRegIdx(), tmp, modVal, blk);
    }

    private static ArrayList<CallInstr> getRecursiveCallInstr(Function function) {
        ArrayList<CallInstr> recursiveCallInstrList = new ArrayList<>();
        BasicBlock basicBlock = function.getFirstBlk();
        while (basicBlock != null) {
            Instruction instruction = basicBlock.getFirstInstr();
            while (instruction != null) {
                if (instruction instanceof CallInstr call && call.getCallee().equals(function)) {
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
        Function parentFunc = call.getParentBB().getParentFunc();

        BasicBlock prevBlk = call.getParentBB();
        BasicBlock callBlk = new BasicBlock(parentFunc.getAndAddBlkIdx());
        BasicBlock aftBlk = new BasicBlock(parentFunc.getAndAddBlkIdx());

        callBlk.insertAfter(prevBlk);
        aftBlk.insertAfter(callBlk);

        Instruction afterCall = (Instruction) call.getNext();
        call.liteRemoveFromList();
        callBlk.addInstruction(call);
        new JumpInstr(aftBlk, callBlk);

        while (afterCall != null) {
            Instruction tmp = (Instruction) afterCall.getNext();
            if (afterCall instanceof Terminator) {
                afterCall.liteRemoveFromList();
                prevBlk.getUsedList().remove(afterCall);
                prevBlk.open();
                aftBlk.addInstruction(afterCall);
                aftBlk.getUsedList().add(afterCall);
                aftBlk.close();
            } else {
                afterCall.liteRemoveFromList();
                aftBlk.addInstruction(afterCall);
            }
            afterCall = tmp;
        }
        for (BasicBlock suc : new ArrayList<>(prevBlk.getSucs())) {
            suc.delPre(prevBlk);
            prevBlk.delSuc(suc);
            suc.addPre(aftBlk);
            aftBlk.addSuc(suc);
            for (Instruction instr : suc.getInstrList()) {
                if (instr instanceof PhiInstr phi) {
                    phi.modifyUse(prevBlk, aftBlk);
                    prevBlk.getUserList().remove(phi);
                    aftBlk.getUserList().add(phi);
                } else {
                    break;
                }
            }
        }

        new JumpInstr(callBlk, prevBlk);

        ArrayList<BasicBlock> splitBlks = new ArrayList<>();
        splitBlks.add(prevBlk);
        splitBlks.add(callBlk);
        splitBlks.add(aftBlk);
        return splitBlks;
    }
}
