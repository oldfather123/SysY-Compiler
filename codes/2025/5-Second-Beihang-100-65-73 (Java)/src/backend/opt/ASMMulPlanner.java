package backend.opt;

import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.rv.arithmetic.RVAdd;
import backend.asm.instr.rv.arithmetic.RVMul;
import backend.asm.instr.rv.arithmetic.RVSll;
import backend.asm.instr.rv.arithmetic.RVSub;
import backend.asm.instr.rv.others.RVLi;
import backend.asm.instr.rv.others.RVMove;
import backend.asm.register.Reg;
import backend.asm.register.store.RegStore;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;
import frontend.ir.instr.Instruction;
import util.Pair;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static backend.opt.ASMMulPlanner.MulOps.MUL_COMPLEX;
import static backend.opt.ASMMulPlanner.MulOps.MUL_ZERO;
import static backend.opt.ASMMulPlanner.MulOps.MUL_VAR;


public class ASMMulPlanner {
    private static final boolean opt = true;
    private static final int INSTR_MAX = 3;

    private static final Map<Long, MulOp> planMap = new HashMap<>();
    private static ASMBasicBlock block;
    private static RegStore regStore;

    private static boolean isInit = false;

    public static void mulConst(Reg ans, Reg src, int imm, ASMBasicBlock asmBlock, RegStore store) {
        block = asmBlock;
        regStore = store;
        MulOp mulOp = findPlan(imm);
        if (mulOp instanceof MulOp.MulOpComplex || !opt) {
            VirIReg tmpReg = new VirIReg();
            new RVLi(new ASMImmediate(imm), block, tmpReg);
            new RVMul(src, tmpReg, block, ans);
        } else if (mulOp instanceof MulOp.MulOpVar) {
            new RVMove(src, block, ans);
        } else if (mulOp instanceof MulOp.MulOpConst) {
            new RVMove(regStore.getZero(), block, ans);
        } else if (mulOp instanceof MulOp.MulAction action) {
            Reg reg = visitAction(src, action);
            new RVMove(reg, block, ans);
        } else {
            throw new RuntimeException("wrong mulOp type!");
        }
    }

    public static Reg visitAction(Reg src, MulOp.MulAction action) {
        MulOp left = action.left;
        MulOp right = action.right;
        VirIReg tmpReg = new VirIReg();
        if (action.type == MulOp.MulAction.ActionType.SHL) {
            if (!(right instanceof MulOp.MulOpConst rightConst)) {
                throw new RuntimeException("SHL right must be constant");
            }
            if (left instanceof MulOp.MulOpVar) {
                tmpReg.setDouble(true);
                new RVSll(src, new ASMImmediate(rightConst.value), block, tmpReg);
            } else {
                if (!(left instanceof MulOp.MulAction leftAction)) {
                    throw new RuntimeException("SHL left must be action or var");
                }
                tmpReg.setDouble(true);
                Reg leftReg = visitAction(src, leftAction);
                new RVSll(leftReg, new ASMImmediate(rightConst.value), block, tmpReg);
            }
        } else if (action.type == MulOp.MulAction.ActionType.ADD
                || action.type == MulOp.MulAction.ActionType.SUB) {
            if (left instanceof MulOp.MulOpConst c1 && c1.value != 0
                    || right instanceof MulOp.MulOpConst c2 && c2.value != 0) {
                throw new RuntimeException("ADD/SUB left or right must be var or action or 0");
            }
            Reg leftReg = left instanceof MulOp.MulOpConst ? regStore.getZero() :
                    left instanceof MulOp.MulAction leftAction ? visitAction(src, leftAction) : src;
            Reg rightReg = right instanceof MulOp.MulOpConst ? regStore.getZero() :
                    right instanceof MulOp.MulAction rightAction ? visitAction(src, rightAction) : src;
            if (action.type == MulOp.MulAction.ActionType.ADD) {
                new RVAdd(leftReg, rightReg, block, tmpReg);
            } else {
                new RVSub(leftReg, rightReg, block, tmpReg);
            }
        }
        return tmpReg;
    }

    public static MulOp findPlan(long imm) {
        if (!isInit) initPlan();
        return planMap.getOrDefault(imm, MUL_COMPLEX);
    }

    private static void initPlan() {
        List<List<Pair<Long, MulOp>>> allPlans = new ArrayList<>();
        List<Pair<Long, MulOp>> level0Plans = new ArrayList<>();
        addPlan(level0Plans, 0, MUL_ZERO);
        addPlan(level0Plans, 1, MUL_VAR);
        allPlans.add(level0Plans);

        for (int levelSum = 1; levelSum <= INSTR_MAX; levelSum++) {
            List<Pair<Long, MulOp>> levelSumPlans = new ArrayList<>();

            // SHL
            int levelShl = levelSum - 1;
            for (Pair<Long, MulOp> left : allPlans.get(levelShl)) {
                long immLeft = left.first;
                MulOp opLeft = left.second;
                if (immLeft == 0) continue;
                for (int right = 1; ; right++) {
                    long immNew = immLeft << right;
                    if ((immNew & (1L << 63)) != 0) break;
                    MulOp.MulAction opNew = new MulOp.MulAction(MulOp.MulAction.ActionType.SHL, opLeft, new MulOp.MulOpConst(right));
                    addPlan(levelSumPlans, immNew, opNew);
                }
            }

            // ADD/SUB
            for (int levelLeft = 0; levelLeft < levelSum; levelLeft++) {
                int levelRight = levelSum - levelLeft - 1;
                for (Pair<Long, MulOp> left : allPlans.get(levelLeft)) {
                    long immLeft = left.first;
                    MulOp opLeft = left.second;
                    for (Pair<Long, MulOp> right : allPlans.get(levelRight)) {
                        long immRight = right.first;
                        MulOp opRight = right.second;
                        long addNew = immLeft + immRight;
                        MulOp.MulAction opAdd = new MulOp.MulAction(MulOp.MulAction.ActionType.ADD, opLeft, opRight);
                        addPlan(levelSumPlans, addNew, opAdd);
                        long subNew = immLeft - immRight;
                        MulOp.MulAction opSub = new MulOp.MulAction(MulOp.MulAction.ActionType.SUB, opLeft, opRight);
                        addPlan(levelSumPlans, subNew, opSub);
                    }
                }
            }

            allPlans.add(levelSumPlans);
        }
        isInit = true;
    }

    private static void addPlan(List<Pair<Long, MulOp>> curLevelPlans, long imm, MulOp mulOp) {
        if (!planMap.containsKey(imm)) {
            planMap.put(imm, mulOp);
            curLevelPlans.add(new Pair<>(imm, mulOp));
        }
    }

    public sealed abstract static class MulOp permits MulOp.MulOpConst, MulOp.MulOpVar, MulOp.MulOpComplex, MulOp.MulAction {
        private final int level;

        public MulOp(int level) {
            this.level = level;
        }

        public static final class MulOpConst extends MulOp {
            private final int value;

            public MulOpConst(int value) {
                super(0);
                this.value = value;
            }
        }

        public static final class MulOpVar extends MulOp {
            public MulOpVar() {
                super(0);
            }
        }

        public static final class MulOpComplex extends MulOp {
            public MulOpComplex() {
                super(0x3fffffff);
            }
        }

        public static final class MulAction extends MulOp {
            public enum ActionType {
                ADD, SUB, SHL
            }

            private final ActionType type;
            private final MulOp left, right;

            public MulAction(ActionType type, MulOp left, MulOp right) {
                super(left.level + right.level + 1);
                this.type = type;
                this.left = left;
                this.right = right;
            }
        }
    }

    public static final class MulOps {
        public static final MulOp MUL_ZERO = new MulOp.MulOpConst(0);
        public static final MulOp MUL_VAR = new MulOp.MulOpVar();
        public static final MulOp MUL_COMPLEX = new MulOp.MulOpComplex();

        private MulOps() {
        }
    }

}
