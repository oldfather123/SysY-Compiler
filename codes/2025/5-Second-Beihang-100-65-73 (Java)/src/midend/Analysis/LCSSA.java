package midend.Analysis;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.*;

/**
 * 请注意
 * 在使用该分析前，务必确保LoopInfo和CFG的有效性，并确保符合Simplify格式，不要跟折叠同时使用
 */
public class LCSSA {

    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            buildLCSSA4Func(function);
            useCheck(function);  // 安全性检查，检查LCSSA是否已经生效
        }
    }

    public static void buildLCSSA4Func(Function function) {
        for (BasicBlock basicBlock : function.getBasicBlockList()) {
            for (Instruction instruction : basicBlock.getInstrList()) {
                if (instruction instanceof PhiInstr phiInstr) {
                    phiInstr.setLCSSA(false);
                }
            }
        }

        for (LoopInfo loop : function.getAncientLoopInfo()) {
            buildLCSSA4Loop(loop);
        }
    }

    public static void buildLCSSA4Loop(LoopInfo loop) {
        for (LoopInfo childLoop : loop.getChildLoop()) {
            buildLCSSA4Loop(childLoop);
        }

        for (BasicBlock loopBB : loop.getBodyBlocks()) {
            for (Instruction instruction : loopBB.getInstrList()) {
                if (isEscaped(instruction, loop)) {
                    insertPhi2Exits(instruction, loop);
                }
            }
        }
    }

    public static boolean isEscaped(Instruction instr, LoopInfo loop) {
        for (Value user : instr.getUserList()) {
            if (user instanceof Instruction userInstr) {
                if (!userInstr.getParentBB().isContainsInLoop(loop)) {
//                    // fixme: 逃逸拦截，有的半中间定义但是header有出口的情况实际上原来就已经被拦截了，判断逃逸应该忽略掉原有的拦截
                    if (userInstr instanceof PhiInstr phi && loop.getExitTargets().contains(phi.getParentBB())) {
                        phi.setLCSSA(true);
                        continue;
                    }
                    return true;
                }
            }
        }
        return false;
    }

    public static void insertPhi2Exits(Instruction instr, LoopInfo loop) {
        Set<BasicBlock> exits = loop.getExitTargets();  //exit不在循环里，但是被循环头支配
        HashMap<BasicBlock, PhiInstr> phiMap = new HashMap<>();
        for (BasicBlock exitBB : exits) {
            PhiInstr phi = new PhiInstr(instr.getType(),
                    exitBB.getParentFunc().getAndAddRegIdx(), exitBB, true);
            for (BasicBlock pre : exitBB.getPres()) {
                phi.addOperand(pre, instr);
            }
            phi.insertBefore(exitBB.getInstrList().getFirst());

            phiMap.put(exitBB, phi);

            // 进行替换
            ArrayList<Value> users = new ArrayList<>(instr.getUserList());
            for (Value user : users) {
                if (user instanceof Instruction userInstr) {
                    if (userInstr.getParentBB().isContainsInLoop(loop)) {
                        continue;
                    }
                    if (userInstr instanceof PhiInstr phiUser) {
                        if (phiUser.isLCSSA()) {
                            // 如果是LCSSA，那么应该是子循环的，没必要再换
                            continue;
                        }
                        if (phiUser.getParentBB() == exitBB) {
                            phiUser.modifyUse(instr, phi);
                            instr.getUserList().remove(phiUser);
                            phi.getUserList().add(phiUser);
                        } else {
                            HashSet<BasicBlock> incomingBBs = phiUser.getKeyByValueMulti(instr);
                            for (BasicBlock incomingBB : incomingBBs) {
                                if (exitBB.getDomSet().contains(incomingBB)) {
                                    // 只能换对应的这个
                                    phiUser.conditionalModifyValue(incomingBB, instr, phi);
                                }
                            }
                        }
                    } else {
                        if (exitBB.getDomSet().contains(userInstr.getParentBB())) {
                            userInstr.modifyUse(instr, phi);
                            phi.getUserList().add(userInstr);
                            instr.getUserList().remove(userInstr);
                        }
                    }
                }
            }
        }

        // 补丁
        for (Value instruction : new ArrayList<>(instr.getUserList())) {
            if (instruction instanceof PhiInstr phi) {
                if (!phi.getParentBB().isContainsInLoop(loop) &&
                        !loop.getExitTargets().contains(phi.getParentBB())) {
                    // 说明有双路 - 汇聚逃逸
                    ArrayList<Set<BasicBlock>> domfrontiers = new ArrayList<>();
                    for (BasicBlock exitBB : exits) {
                        domfrontiers.add(exitBB.getDominanceFrontier());
                    }
                    Set<BasicBlock> domfrontier = intersectAll(domfrontiers);
                    // 任意exit的支配边界里一定有支配phi的
                    for (BasicBlock df : domfrontier) {
                        Set<BasicBlock> phiInComings = phi.getOperandMap().keySet();
                        boolean dom = false;
                        for (BasicBlock incomingBB : phiInComings) {
                            if (df.getDomSet().contains(incomingBB)) {
                                dom = true;
                                break;
                            }
                        }
                        if (!dom) {
                            continue;
                        }
                        // 这就是汇聚点
                        PhiInstr convergePhi = new PhiInstr(phi.getType(),
                                phi.getParentBB().getParentFunc().getAndAddRegIdx(),
                                df);
                        for (Map.Entry<BasicBlock, PhiInstr> entry : phiMap.entrySet()) {
                            convergePhi.addOperand(entry.getKey(), entry.getValue());
                        }
                        convergePhi.insertBefore(df.getFirstInstr());
                        for (BasicBlock incomingBB : phi.getKeyByValueMulti(instr)) {
                            if (df.getDomSet().contains(incomingBB)) {
                                phi.conditionalModifyValue(incomingBB, instr, convergePhi);
//                                instr.getUserList().remove(phi);
//                                convergePhi.getUserList().add(phi);
                            }
                        }
                    }

                }
            } else if (instruction instanceof Instruction userInst) {
                if (!userInst.getParentBB().isContainsInLoop(loop) &&
                        !loop.getExitTargets().contains(userInst.getParentBB())) {
                    // 说明有双路 - 汇聚逃逸
                    ArrayList<Set<BasicBlock>> domfrontiers = new ArrayList<>();
                    for (BasicBlock exitBB : exits) {
                        domfrontiers.add(exitBB.getDominanceFrontier());
                    }
                    Set<BasicBlock> domfrontier = intersectAll(domfrontiers);
                    // 任意exit的支配边界里一定有支配phi的
                    for (BasicBlock df : domfrontier) {
                        if (df.getDomSet().contains(userInst.getParentBB())) {
                            // 这就是汇聚点
                            PhiInstr convergePhi = new PhiInstr(instr.getType(),
                                    userInst.getParentBB().getParentFunc().getAndAddRegIdx(),
                                    df);
                            for (Map.Entry<BasicBlock, PhiInstr> entry : phiMap.entrySet()) {
                                convergePhi.addOperand(entry.getKey(), entry.getValue());
                            }
                            convergePhi.insertBefore(df.getFirstInstr());
                            userInst.modifyUse(instr, convergePhi);
                            instr.getUserList().remove(userInst);
                            convergePhi.getUserList().add(userInst);

                            break;
                        }
                    }

                }
            }
        }
    }

    public static Set<BasicBlock> intersectAll(List<Set<BasicBlock>> sets) {
        if (sets == null || sets.isEmpty()) return Collections.emptySet();

        Set<BasicBlock> result = new HashSet<>(sets.getFirst());
        for (int i = 1; i < sets.size(); i++) {
            result.retainAll(sets.get(i)); // 保留交集元素
            if (result.isEmpty()) break;
        }
        return result;
    }

    public static void useCheck(Function function) {
        for (LoopInfo loop : function.getAncientLoopInfo()) {
            for (BasicBlock loopBB : loop.getBodyBlocks()) {
                for (Instruction instruction : loopBB.getInstrList()) {
                    for (Value user : instruction.getUserList()) {
                        if (user instanceof Instruction userInstr) {
                            if (!userInstr.getParentBB().isContainsInLoop(loop)) {
                                if (!(userInstr instanceof PhiInstr phi && phi.isLCSSA())) {
                                    System.out.println(instruction);
                                    System.out.println(userInstr);
//                                    System.out.println("LCSSA ERR : FUNC INVALID");
                                    throw new RuntimeException("LCSSA ERR : FUNC INVALID");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
