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
import frontend.ir.instr.otherop.cmp.Cmp;
import frontend.ir.instr.otherop.cmp.CmpCond;
import frontend.ir.instr.otherop.cmp.ICmpInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.FParam;
import frontend.ir.structure.Function;
import frontend.ir.symbols.ArrayInitVal;
import frontend.ir.symbols.SymTab;
import frontend.ir.symbols.Symbol;

import java.util.ArrayList;
import java.util.List;

/**
 * （递归）函数记忆化，通过全局数组减少递归运算
 * 因为需要跳转到结束块，所以必须放在合并块之前
 * 为了减少哈希计算，决定对于要操作的递归函数，要在递归结束条件判断之后才做哈希
 * 因为没有控制流图实在不知道怎么找返回值类型了，决定将这个减少哈希计算的操作放到后面 FMHBR 里
 * todo: 现在找递归结束条件的原则是直接返回常数或者参数，但是其实更合适的应该是去找最后一个能支配所有递归调用的块，在它那做哈希
 */
public class FuncMemorize {
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
                
                // 建立两个基本块，并准备两个常数对象
                BasicBlock hashBlk = new BasicBlock(0, function.getAndAddBlkIndex());
                BasicBlock preRetBlk  = new BasicBlock(0, function.getAndAddBlkIndex());
                hashBlk.setTag("hashBlk");
                preRetBlk.setTag("hashRetBlk");
                
                ConstInt baseVal  = new ConstInt(hash_base);
                ConstInt modVal   = new ConstInt(hash_mod);
//                ConstInt baseVal2 = new ConstInt(hash_base2);
//                ConstInt modVal2  = new ConstInt(hash_mod2);
                
                // 将所有的参数转化为整数以备哈希
                ArrayList<Value> castArgs = new ArrayList<>();
                for (FParam fParam : function.getFParams()) {
                    if (fParam.getDataType() == DataType.INT) {
                        castArgs.add(fParam);
                    } else {
                        // 浮点数乘以一百放大差异
                        // FMulInstr fMul = new FMulInstr(function.getAndAddRegIndex(), fParam, new ConstFloat(100f));
                        // hashBlk.addInstruction(fMul);

                        Fp2Si cast = new Fp2Si(function.getAndAddRegIndex(), fParam);
                        hashBlk.addInstruction(cast);
                        castArgs.add(cast);
                    }
                }
                // final int argSize = castArgs.size();
                
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
                
                // 检查一下之前有没有算过这个值，如果算过进入准备返回，否则正常开始计算
                LoadInstr usedVal = new LoadInstr(function.getAndAddRegIndex(), globalUsed, usedPtr);
                hashBlk.addInstruction(usedVal);
                Cmp usedCond = new ICmpInstr(function.getAndAddRegIndex(), CmpCond.NE, usedVal, castArgs.get(0));
                hashBlk.addInstruction(usedCond);
                BranchInstr branch = new BranchInstr(usedCond, (BasicBlock) function.getBasicBlocks().getHead(), preRetBlk);
                hashBlk.addInstruction(branch);
                
                // 构建直接返回块：取出对应的记录结果并返回
                LoadInstr dataVal = new LoadInstr(function.getAndAddRegIndex(), globalData, dataPtr);
                preRetBlk.addInstruction(dataVal);
                StoreInstr storeRetData = new StoreInstr(dataVal, function.getFuncSymTab().getSym(""));
                preRetBlk.addInstruction(storeRetData);
                JumpInstr jump = new JumpInstr((BasicBlock) function.getBasicBlocks().getTail());
                preRetBlk.addInstruction(jump);
                
                // 在返回之前加一个 store 存储计算数值
                BasicBlock retBlk = (BasicBlock) function.getBasicBlocks().getTail();
                for (BasicBlock toRet : retBlk.getPres()) {
                    if (toRet == preRetBlk) {
                        continue;
                    }
                    Instruction jump2ret = toRet.getEndInstr();
                    LoadInstr loadData   = new LoadInstr(function.getAndAddRegIndex(), function.getFuncSymTab().getSym(""));
                    loadData.insertBefore(jump2ret);
                    StoreInstr storeData = new StoreInstr(loadData, globalData, dataPtr);
                    storeData.insertBefore(jump2ret);
                    StoreInstr storeUsed = new StoreInstr(castArgs.get(0), globalUsed, usedPtr);
                    storeUsed.insertBefore(jump2ret);
                }
                
                // 将两个新块插入函数头
                function.getBasicBlocks().addToHead(preRetBlk);
                function.getBasicBlocks().addToHead(hashBlk);
                
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
}
