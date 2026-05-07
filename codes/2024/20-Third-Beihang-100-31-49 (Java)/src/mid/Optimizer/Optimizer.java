package mid.Optimizer;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.ControllFlow.ControlFlowGraph;
import mid.Optimizer.ControllFlow.DominAnalyzer;
import mid.Optimizer.Loop.IndValueLift;
import mid.Optimizer.Loop.IndValueReuse;
import mid.Optimizer.Loop.LCSSA;
import mid.Optimizer.Loop.LoopAnalyze;
import mid.Optimizer.Loop.LoopFusion;
import mid.Optimizer.Loop.LoopSimplify;
import mid.Optimizer.Loop.LoopUnroll;
import mid.Optimizer.Loop.LoopUnswitch;
import mid.Optimizer.Loop.MinLoopAnalyze;
import mid.Optimizer.Loop.ScalarEvolution;
import mid.Optimizer.Memory.*;
import mid.Optimizer.RedundancyElim.*;
import utils.Runner;
import utils.tools.Timer;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class Optimizer {
    private static final Optimizer INSTANCE = new Optimizer();

    private ControlFlowGraph CFG;

    private DominAnalyzer dominAnalyzer;
    private LoopAnalyze loopAnalyze;

    private FunctionOptimize functionOptimize;

    private Optimizer() {
    }

    public static Optimizer instance() {
        return INSTANCE;
    }

    public ControlFlowGraph getCFG() {
        return CFG;
    }

    public DominAnalyzer getDominAnalyzer() {
        return dominAnalyzer;
    }

    public void mem2regOnly() {
        IRManager.getInstance().setAutoInsert(false);
        if (Timer.INSTANCE.timeOut()) {
            return;
        }
        FunctionOptimize functionOptimize = new FunctionOptimize();
        functionOptimize.analyze();
        this.functionOptimize = functionOptimize;
        analyze();
        if (Timer.INSTANCE.timeOut()) {
            return;
        }
        // 不存入寄存器也会爆栈...
        Mem2Reg mem2Reg = new Mem2Reg();
        mem2Reg.optimize();
        // DCE删去无用指令，否则寄存器分配会出问题
        (new DeadCode()).removeUnusedInstructions();
//        (new LocalArrayLift()).liftForStack();
//        (new ConstFold()).finalOptimize(true);
//        (new GCM()).optimize();
        Runner.output("ir_opt_phi.ll", IRManager.getModule());
        mem2Reg.phiToMove();
    }

    public void backendBasicOpt() {
        IRManager.getInstance().setAutoInsert(false);
        if (Timer.INSTANCE.timeOut()) {
            return;
        }
        FunctionOptimize functionOptimize = new FunctionOptimize();
        functionOptimize.analyze();
        this.functionOptimize = functionOptimize;
        analyze();
        if (Timer.INSTANCE.timeOut()) {
            return;
        }
        // 不内联传参会爆栈
        functionOptimize.optimizeForStack(200);
        functionOptimize.analyze();
        analyze();
        if (Timer.INSTANCE.timeOut()) {
            return;
        }
        // 不存入寄存器也会爆栈...
        Mem2Reg mem2Reg = new Mem2Reg();
        mem2Reg.optimize();
        // DCE删去无用指令，否则寄存器分配会出问题
        analyze();
        (new LocalArrayLift()).liftForStack();
        (new ConstFold()).finalOptimize(true);
        (new GCM()).optimize();
        Runner.output("ir_opt_phi.ll", IRManager.getModule());
        mem2Reg.phiToMove();
    }


    public void optimize() throws IOException {
        IRManager.getInstance().setAutoInsert(false);

        // 函数分析
        FunctionOptimize functionOptimize = new FunctionOptimize();
        functionOptimize.analyze();
        this.functionOptimize = functionOptimize;

        // 流图-支配分析 + 循环分析 + 死代码删除
        analyze();
        // 尾递归优化 + 函数调用
        (new TailRecuElim()).optimize();
        functionOptimize.analyze();
        analyze();
        // 函数内联
        functionOptimize.optimize();
        analyze();
        Straighten straighten = new Straighten();
        straighten.optimize();
        analyze();

        // 全局变量局部化
        GlobalDeclLocalization globalDeclLocalization = new GlobalDeclLocalization();
        globalDeclLocalization.optimize();
        analyze();

        // 局部数组变量化
        ArrayToAlloca arrayToAlloca = new ArrayToAlloca();
        arrayToAlloca.optimize();
        analyze();
        (new LoadLVN()).optimize();

        // Memory To Register
        Mem2Reg mem2Reg = new Mem2Reg();
        mem2Reg.optimize();

        analyze();

        (new JumpThreading()).optimize();

        (new TailRecuElim()).recuInfer();

        // 修改了图结构，重新分析
        analyze();
        for (int i = 0; i < 3 && !Timer.INSTANCE.timeOut(); i++) {
            loopOpt();
            StrengthOpt();
        }
        if (!Timer.INSTANCE.timeOut()) {
            finalConstFoldOpt();

            straighten.optimize();
        }

        // 局部数组提升
        LocalArrayLift localArrayLift = new LocalArrayLift();
        localArrayLift.optimize();

        // 消除phi
        Runner.output("ir_opt_phi.ll", IRManager.getModule());
        mem2Reg.phiToMove();
        Runner.output("ir_opt_move.ll", IRManager.getModule());
    }

    public void analyze() {
        //预处理跳转指令，以便流图分析
        DeadCode deadCode = new DeadCode();
        deadCode.scanJump();
        //流图分析
        ControlFlowGraph CFG = new ControlFlowGraph();
        CFG.analyze();
        this.CFG = CFG;
        //删除不可达块，以便支配分析和phi的插入;并删除无用指令
        if (deadCode.optimize()) {
            //重新进行流图分析
            CFG = new ControlFlowGraph();
            CFG.analyze();
            this.CFG = CFG;
        }
        //支配分析
        DominAnalyzer dominAnalyzer = new DominAnalyzer();
        dominAnalyzer.analyze(CFG);
        this.dominAnalyzer = dominAnalyzer;

        //循环分析
        LoopAnalyze loopAnalyze = new LoopAnalyze();
        loopAnalyze.analyze();
        this.loopAnalyze = loopAnalyze;
    }

    private void constFoldOpt() {
        do {
            analyze();
            (new GVN()).optimize();
            (new GCM()).optimize();
        } while ((new ConstFold()).optimize());
    }

    private void finalConstFoldOpt() {
        do {
            analyze();
            (new GVN()).optimize();
            (new GCM()).optimize();
        } while ((new ConstFold()).finalOptimize(false));
    }

    private void loopOpt() {
        if (loopAnalyze.getLoops().size() == 0 || Timer.INSTANCE.timeOut()) {
            return;
        }
        loopOptAnalyze();
        // 归纳变量的死代码删除+编译期求值
        (new IndValueLift()).optimize();
        (new ScalarEvolution()).remLift();
        // 循环内的归纳变量复用（相当于归纳变量的LVN)
        (new IndValueReuse()).optimize();
        (new GCM()).optimize();
        (new ConstFold(true)).optimize();
        // 循环展开
        (new LoopUnroll()).optimize();
        // 去开关化
//        (new LoopUnswitch()).optimize();
        // 循环交换
//        (new LoopInterChange()).optimize();
        // 循环融合
        (new LoopFusion()).optimize();
        // 清理循环信息
        analyze();
        (new LoopSimplify()).optimize();
        (new ScalarEvolution()).optimize();
        (new ConstFold(true)).optimize();
        (new IndValueReuse()).optimize();
        (new DeadCode()).clearLoopInfo();
        analyze();
    }

    public void loopOptAnalyze() {
        // 循环化简
        LoopSimplify loopSimplify = new LoopSimplify();
        loopSimplify.optimize();
        dominAnalyze();
        // scev
        ScalarEvolution scalarEvolution = new ScalarEvolution();
        (new ConstFold(true)).optimize();
        scalarEvolution.optimize();
        // 最小循环的退出条件分析
        (new MinLoopAnalyze()).analyze();
        // constfold以计算循环次数
        (new ConstFold(true)).optimize();
        // lcssa
        LCSSA lcssa = new LCSSA();
        lcssa.optimize();
    }

    private void finalLoopOpt() throws IOException {
        if (loopAnalyze.getLoops().size() == 0) {
            return;
        }
        // 循环化简
        LoopSimplify loopSimplify = new LoopSimplify();
        loopSimplify.optimize();
        dominAnalyze();
        // lcssa
        LCSSA lcssa = new LCSSA();
        lcssa.optimize();
        // scev
        ScalarEvolution scalarEvolution = new ScalarEvolution();
        (new ConstFold(true)).optimize();
        scalarEvolution.optimize();
        // 可能需要GCM修一修
        (new GCM()).optimize();
        // 最小循环的退出条件分析
        (new MinLoopAnalyze()).analyze();
        // constfold以便循环退出条件分析
        (new ConstFold(true)).optimize();
        // 归纳变量的死代码删除+编译期求值
        (new IndValueLift()).optimize();
        // 循环内的归纳变量复用（相当于归纳变量的LVN)
        (new IndValueReuse()).optimize();
        (new ConstFold(true)).optimize();
        (new GCM()).optimize();
        // 循环展开
        (new LoopUnroll()).optimizeWitUAJ();
        (new DeadCode()).clearLoopInfo();
        (new Straighten()).optimize();
        analyze();
    }

    public void StrengthOpt() {
        constFoldOpt();
        // 经过constfold之后效果更好
        (new ArrayToAlloca()).optimize();
        analyze();
        (new Mem2Reg()).optimize();
        analyze();
        (new LoadLVN()).optimize();

        constFoldOpt();
        (new DeadCode()).optimize();
        analyze();
    }


    public ArrayList<BasicBlock> bfsDominTreeArray(BasicBlock entranceBlock) {
        return dominAnalyzer.bfsDominTreeArray(entranceBlock);
    }

    public LoopAnalyze getLoopAnalyze() {
        return loopAnalyze;
    }

    public void dominAnalyze() {
        (new DeadCode()).scanJump();
        CFG = new ControlFlowGraph();
        CFG.analyze();
        dominAnalyzer = new DominAnalyzer();
        dominAnalyzer.analyze(CFG);
    }

    public void dominAnalyze(Function f) {
        cfgAnalyze(f);
        dominAnalyzer.analyze(CFG, f);
    }

    public void cfgAnalyze(Function f) {
        (new DeadCode()).scanJump();
        CFG.analyze(f);
    }

    public boolean hasSideEffect(Function function) {
        return functionOptimize.hasSideEffect(function);
    }

    public void splitBlockPredcessors(BasicBlock block, BasicBlock newBlock, HashSet<BasicBlock> predcessors) {
        /*
            preds+others -> block
            preds->block; others->newBlk; newBlk->block
         */
        CFG.splitBlockPredcessors(block, newBlock, predcessors);
        dominAnalyzer.analyze(CFG, block.getFunction());

        // 处理跳转和Phi
        HashMap<Phi, Phi> originPhiToNew = new HashMap<>();
        for (BasicBlock precessor : predcessors) {
            // 将原本pred来源的phi cond提取到newBlock上
            for (Instruction instr : block.getInstructionList()) {
                if (instr instanceof Phi phi && phi.getOperandList().contains(precessor)) {
                    Value v = phi.valueFromBlock(precessor);
                    phi.removeOperand(precessor);
                    if (!originPhiToNew.containsKey(phi)) {
                        originPhiToNew.put(phi, new Phi(!phi.isFloat(),
                                IRManager.getInstance().declareTempVar()));
                        newBlock.addInstrAtEntry(originPhiToNew.get(phi));
                    }
                    originPhiToNew.get(phi).addCond(v, precessor);
                }
            }
        }

        for (Phi phi : originPhiToNew.keySet()) {
            phi.addCond(originPhiToNew.get(phi), newBlock);
        }

        for (BasicBlock precessor : predcessors) {
            //修改末尾的跳转方向为新的nextBlock
            Instruction instruction = precessor.getLastInstruction();
            if (instruction instanceof Br br) {
                br.redirectTo(block, newBlock);
            }
        }
    }
}
