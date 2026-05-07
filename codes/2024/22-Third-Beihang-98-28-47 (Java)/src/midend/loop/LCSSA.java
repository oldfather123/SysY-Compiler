package midend.loop;

import debug.DEBUG;
import frontend.ir.Use;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstInt;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.IdentityHashMap;

public class LCSSA {
    private static Function curFunc;
//    private static int phiCnt;
    public static void execute(ArrayList<Function> functions) {
        for (Function function : functions) {
            execute4func(function);
        }
    }

    public static void execute4func(Function function) {
        curFunc = function;
        addPhi(function);
    }

    private static void addPhi(Function function) {
        for (Loop loop : function.getOuterLoop()) {
            dfs4addPhi(loop);
        }
    }
    private static HashMap<BasicBlock, PhiInstr> exit2phi = new HashMap<>();

    private static void dfs4addPhi(Loop loop) {
        for (Loop innerLoop : loop.getInnerLoops()) {
            dfs4addPhi(innerLoop);
        }

        ArrayList<BasicBlock> exits = loop.getExits();
        if (exits.isEmpty()) {
            return;
        }
        for (BasicBlock blk : loop.getBlks()) {
            Instruction instr = (Instruction) blk.getInstructions().getHead();
            while (instr != null) {
                //从外界流入的值，在结束快就会出现至少两个来源 => phi
                if (liveInLoop(instr, loop)) {
                    //在每一个结束块加入phi
                    exit2phi.clear();
                    BasicBlock prt = instr.getParentBB();
                    for (BasicBlock exitBlk : loop.getExits()) {
                        //如果当前的指令的父块支配结束块，则会产生一个phi
                        if (!exit2phi.containsKey(exitBlk) && prt.getDoms().contains(exitBlk)) {
                            ArrayList<Value> phiValues = new ArrayList<>();
                            ArrayList<BasicBlock> prts = new ArrayList<>();
                            //phi的值由前驱决定
                            //将phi替换为后面使用他的
                            for (BasicBlock pre : exitBlk.getPres()) {
                                phiValues.add(instr);
                                prts.add(pre);
                            }
                            PhiInstr phi = new PhiInstr(curFunc.getAndAddPhiIndex(), instr.getDataType(), phiValues, prts);
//                            System.out.println(phi.print() + " " + phiCnt);
                            exitBlk.addInstrToHead(phi);
                            exit2phi.put(exitBlk, phi);
                        }
                    }
                    //rename
                    Use use = instr.getBeginUse();
                    while (use != null) {
                        Instruction user = use.getUser();
                        BasicBlock userBlk = getUserBlock(instr, user);
                        //phi中的prt与value的parentBB不一定相同！！！！
                        //fixme：？？？？你怎么又没有问题了
                        if(userBlk == prt || loop.getBlks().contains(userBlk)){
                            use = (Use) use.getNext();
                            continue;
                        }
                        if (userBlk == null) {
                            throw new RuntimeException("phi: " + user.print() + "used: " + instr.print());
                        }
                        PhiInstr phi = getPhiValue(userBlk, loop);
                        if (phi == null) {
                            user.modifyUse(instr, ConstInt.Zero);
                        } else {
                            user.modifyUse(instr, phi);
                        }
                        use = (Use) use.getNext();
                    }
                }
                instr = (Instruction) instr.getNext();
            }
        }
    }

    private static PhiInstr getPhiValue(BasicBlock userBlk, Loop loop) {
        //userBlk: 当前指令在循环后被使用的地方
        PhiInstr phi = exit2phi.get(userBlk);
        if (phi != null) {
            return phi;
        }
        BasicBlock iDomor = userBlk.getiDomor();
        if (!loop.getBlks().contains(iDomor)) {
            phi = getPhiValue(iDomor, loop);
            exit2phi.put(userBlk, phi);
            return phi;
        }
        ArrayList<Value> values = new ArrayList<>();
        ArrayList<BasicBlock> prtBlks = new ArrayList<>();
        for (BasicBlock pre : userBlk.getPres()) {
            values.add(getPhiValue(pre, loop));
            prtBlks.add(pre);
        }
        phi = new PhiInstr(curFunc.getAndAddPhiIndex(), values.get(0).getDataType(), values, prtBlks);
//        System.out.println(phi.print() + " " + phiCnt);
        userBlk.addInstrToHead(phi);
        exit2phi.put(userBlk, phi);
        return phi;
    }

    public static BasicBlock getUserBlock(Instruction instr, Instruction user){
        BasicBlock userBlk = user.getParentBB();
        if(user instanceof PhiInstr phi){
            int index = phi.getValues().indexOf(instr);
            if (index < 0) {
                throw new RuntimeException("user: " + user.print() + "used: " + instr.print());
            }
            userBlk = phi.getPrtBlks().get(index);
        }
        return userBlk;
    }

    //該指令的所有use的user是否在循环外被使用
    public static boolean liveInLoop(Instruction instr, Loop loop) {
        Use use = instr.getBeginUse();
        while (use != null) {
            BasicBlock userBlk = getUserBlock(instr, use.getUser());
            if (!loop.getBlks().contains(userBlk)) {
                return true;
            }
            use = (Use) use.getNext();
        }
        return false;
    }
}
