package Pass;

import Backend.component.ObjModule;
import Driver.Config;
import IR.IRModule;
import Pass.IR.*;
import Pass.Obj.MergeBlock;
import Pass.Obj.Peephole;
import Pass.Pass.*;

import java.util.ArrayList;

public class PassManager {
    private static final PassManager passManager = new PassManager();
    public static PassManager getInstance(){
        return passManager;
    }
    ArrayList<IRPass> irPasses = new ArrayList<>();
    ArrayList<IRPass> mirPasses = new ArrayList<>();
    ArrayList<ObjPass> objPasses = new ArrayList<>();


    /*
    * 一些irPass顺序的想法：
    * 1. ConstFold要在InstComb之前
    * 2. GlobalValueLocalize必须在Mem2reg之前
    * 3. MergeBb必须在Mem2reg前
    * 4. InstComb必须在DCE前
    * 5. LoopUnroll紧跟LCSSA
    * 6. LVN做在FuncInLine之前
    * ...
    *
    * */
    private PassManager(){
        //  这里放入所有pass,控制pass的顺序
        //  基础优化
        //  2023-08-06: 只开这几个功能都过了，剩下的优化都放O1里
        irPasses.add(new GlobalValueLocalize());
        irPasses.add(new Mem2Reg());


        irPasses.add(new FuncInLine());
        irPasses.add(new GlobalValueLocalize());
        irPasses.add(new Mem2Reg());
        irPasses.add(new LocalArrayLift());

        Pass();

        mirPasses.add(new RemovePhi());
        if(Config.isO1) {
            //  循环相关优化
            irPasses.add(new LCSSA());
            irPasses.add(new LoopUnroll());
            Pass();

            //  数组相关优化
//            irPasses.add(new GepFuse());
            irPasses.add(new GlobalArrayInit());
            irPasses.add(new ArrayGVN());
            Pass();

//              第二次循环优化
            irPasses.add(new LoopFold());
            irPasses.add(new LoopFusion());
            irPasses.add(new RemoveUselessLoop());
            irPasses.add(new AggressiveDCE());
            irPasses.add(new GCM());
//            Pass();

//              最后做取模的优化，方便判断循环模式
//            irPasses.add(new MathOptimization());
            irPasses.add(new Rem2DivMulSub());
//            irPasses.add(new GepSplit());
//            Pass();

            objPasses.add(new MergeBlock());
            objPasses.add(new Peephole());
        }

    }

    //  Pass里一套基本优化顺序，可以多次使用
    private void Pass(){
        irPasses.add(new RemoveUselessPhi());
        irPasses.add(new InstComb());
        irPasses.add(new DCE());
        irPasses.add(new ConstArrayFold());
        irPasses.add(new ConstFold());
        irPasses.add(new InstComb());
        irPasses.add(new ConstFold());
        irPasses.add(new DCE());
        irPasses.add(new RemoveUselessStore());
        irPasses.add(new PeepHole());
        irPasses.add(new GVN());
        irPasses.add(new GCM());
        irPasses.add(new MergeBB());
        irPasses.add(new PeepHole());
        irPasses.add(new RemoveUselessPhi());
        irPasses.add(new MergeBB());
        irPasses.add(new RemoveUselessPhi());
        irPasses.add(new ConstFold());
        irPasses.add(new MergeBB());
        irPasses.add(new DCE());
        irPasses.add(new RemoveUselessPhi());
    }


    public void runIRPasses(IRModule irModule){
        for(IRPass irPass : irPasses){
            irPass.run(irModule);
        }
    }

    public void runObjPasses(ObjModule objModule){
        for(ObjPass objPass : objPasses){
            objPass.run(objModule);
        }
    }

    public void runMirPasses(IRModule irModule){
        for(IRPass irPass : mirPasses){
            irPass.run(irModule);
        }
    }
}
