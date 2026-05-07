package midend.SSA;

import Utils.CustomList;
import debug.DEBUG;
import frontend.ir.DataType;
import frontend.ir.Use;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstFloat;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.EmptyInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.io.IOException;
import java.util.*;

public class Mem2Reg {
//    private static int phiCnt;//TODO:!!!!!
    private static Function curFunc;
    public static void execute(ArrayList<Function> functions) throws IOException {
        for (Function function : functions) {
            curFunc = function;
            removeAlloc(function);
        }
    }
    
    private static void removeAlloc(Function function) throws IOException {
        BasicBlock block = (BasicBlock) function.getBasicBlocks().getHead();
        Instruction instr = (Instruction) block.getInstructions().getHead();
        while (instr != null) {
//                System.out.println(instr.print());
            if (instr instanceof AllocaInstr && instr.getPointerLevel() == 1) {
//                    if (cnt <= 10) {
//                        BufferedWriter writer = new BufferedWriter(new FileWriter("testPhi" + cnt++));
//                        function.printIR(writer);
//                        writer.close();
//                    }
                remove(instr);
            }
            instr = (Instruction) instr.getNext();
        }
    }
    
    private static void remove(Instruction instr) {
        ArrayList<BasicBlock> defBlks = new ArrayList<>();
        ArrayList<BasicBlock> useBlks = new ArrayList<>();
        ArrayList<Instruction> defIns = new ArrayList<>();
        ArrayList<Instruction> useIns = new ArrayList<>();
//        useDBG(instr);
        Use use = instr.getBeginUse();
//        System.out.println("be used: " + instr.print());
        for (; use != null; use = (Use) use.getNext()) {
            Instruction ins = use.getUser();
            BasicBlock blk = ins.getParentBB();
            if (ins instanceof StoreInstr) {
                defBlks.add(blk);
                defIns.add(ins);
            } else if (ins instanceof LoadInstr) {
                useBlks.add(blk);
                useIns.add(ins);
            } else {
                throw new RuntimeException("Furina said you are wrong!");
            }
        }
//        System.out.println("defIns: ");
//        for (Instruction ins : defIns) {
//            System.out.println(ins.print() + " ParentBlk: " + ins.getParentBB());
//        }
//        System.out.println("useIns: ");
//        for (Instruction ins : useIns) {
//            System.out.println(ins.print() + " ParentBlk: " + ins.getParentBB());
//        }
        //to be improved
        if (useBlks.isEmpty()) {
//            for (Instruction ins : defIns) {
//                ins.removeFromList();
//            }
        } else if (defIns.size() == 1 && defBlks.get(0).getDoms().containsAll(useBlks)) {
            //不处理，未定义的初始值
            Instruction store = defIns.get(0);//store指令
            assert store instanceof StoreInstr;
            Value toStoreValue = ((StoreInstr) store).getValue();//要被使用的值1
            for (Instruction load : useIns) {//load指令
                load.replaceUseTo(toStoreValue);
            }
        } else {
            ArrayList<BasicBlock> toPuts = new ArrayList<>();
            Queue<BasicBlock> W = new LinkedList<>(defBlks);
            while (!W.isEmpty()) {
                BasicBlock X = W.poll();
                for (BasicBlock Y : X.getDF()) {
                    if (!toPuts.contains(Y)) {
//                        System.out.println(Y);
                        toPuts.add(Y);
                        if (!defBlks.contains(Y)) {
                            W.add(Y);
                        }
                    }
                }
            }
//            System.out.println();
            
            
            for (BasicBlock block : toPuts) {
                ArrayList<Value> values = new ArrayList<>();
                ArrayList<BasicBlock> prtBlks = new ArrayList<>();
                for (BasicBlock blk : block.getPres()) {
                    values.add(new EmptyInstr(DataType.VOID));
                    prtBlks.add(blk);
                }
                Instruction phi = new PhiInstr(curFunc.getAndAddPhiIndex(), instr.getDataType(), values, prtBlks);
                block.addInstrToHead(phi);
                useIns.add(phi);
                defIns.add(phi);
                useBlks.add(block);
                defBlks.add(block);
            }
            
            BasicBlock block = (BasicBlock) instr.getParentBB().getParent().getHead();
            Stack<Value> S = new Stack<>();
            dfs4rename(S, block, useIns, defIns);
        }
        
        for (Instruction ins : useIns) {
            if (!(ins instanceof PhiInstr)) {
                ins.removeFromList();
            }
        }
        for (Instruction ins : defIns) {
            if (!(ins instanceof PhiInstr)) {
                ins.removeFromList();
            }
        }
        instr.removeFromList();
    }
    
    private static void dfs4rename(Stack<Value> S, BasicBlock now, ArrayList<Instruction> useIns, ArrayList<Instruction> defIns) {
        int cnt = 0;
        for (CustomList.Node item : now.getInstructions()) {
            Instruction instr = (Instruction) item;
            if (!(instr instanceof PhiInstr) && useIns.contains(instr)) {
                //changeValue
                assert instr instanceof LoadInstr;
                instr.replaceUseTo(getStackValue(S, instr.getDataType()));
            }
            if (defIns.contains(instr)) {
                assert instr instanceof StoreInstr || instr instanceof PhiInstr;
                if (instr instanceof StoreInstr) {
                    S.push(((StoreInstr) instr).getValue());
                } else {
                    S.push(instr);
                }
                cnt++;
            }
        }
        HashSet<BasicBlock> sucs = now.getSucs();
        for (BasicBlock block : sucs) {
            //block 是 now 的某个后继。顺序保存在了PhiInstr里面
            for (CustomList.Node item : block.getInstructions()) {
                Instruction instr = (Instruction) item;
                if (!(instr instanceof PhiInstr)) {
                    break;
                }
                if (useIns.contains(instr)) {
                    ((PhiInstr)instr).modifyUse(getStackValue(S, instr.getDataType()), now);
                }
            }
        }
        
        for (BasicBlock nextBlk : now.getIDoms()) {
            dfs4rename(S, nextBlk, useIns, defIns);
        }
        
        for (int i = 0; i < cnt; i++) {
            S.pop();
        }
    }
    
    public static Value getStackValue(Stack<Value> S, DataType type) {
        if (S.isEmpty()) {
            if (type == DataType.FLOAT) {
                return ConstFloat.Zero;
            } else {
                return ConstInt.Zero;
            }
        }
        return S.peek();
    }
    
    
    private static void useDBG(Instruction instr) {
        DEBUG.dbgPrint1(instr.print());
        DEBUG.dbgPrint1("use:");
        for (Use use = instr.getBeginUse(); use != null; use = (Use) use.getNext()) {
            DEBUG.dbgPrint2(use.getUser().print());
        }
    }
}
