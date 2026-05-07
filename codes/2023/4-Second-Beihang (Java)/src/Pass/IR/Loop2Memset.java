package Pass.IR;

import IR.IRBuildFactory;
import IR.IRModule;
import IR.Type.PointerType;
import IR.Value.BasicBlock;
import IR.Value.ConstInteger;
import IR.Value.Function;
import IR.Value.Instructions.*;
import IR.Value.Value;
import IR.Visitor;
import Pass.IR.Utils.IRLoop;
import Pass.IR.Utils.LoopAnalysis;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;

public class Loop2Memset implements Pass.IRPass {
    private GepInst gepInst;
    private StoreInst storeInst;
    private Value size;
    private final IRBuildFactory f = IRBuildFactory.getInstance();

    @Override
    public void run(IRModule module) {
        for(Function function : module.getFunctions()){
            for(IRLoop topLoop : function.getTopLoops()){
                runLoop2Memset(topLoop);
            }
        }
    }

    private void runLoop2Memset(IRLoop loop){
        if(loop.getSubLoops().size() != 0){
            for(IRLoop subLoop : loop.getSubLoops()){
                runLoop2Memset(subLoop);
            }
        }

        if(canChangeToMemset(loop)){
            BasicBlock enteringBb = loop.getHead().getPreBlocks().get(0);
            Instruction lastInst = enteringBb.getLastInst();
            Value pointer = gepInst.getTarget();

            ArrayList<Value> memsetValues = new ArrayList<>();
            //  4 * n
            BinaryInst binInst = f.getBinaryInst(size, f.buildNumber(4), OP.Mul, size.getType());
            binInst.insertBefore(lastInst);
            //  call memset
            memsetValues.add(pointer);
            memsetValues.add(f.buildNumber(0));
            memsetValues.add(binInst);
            CallInst callInst = f.getCallInst(Visitor.MemsetFunc, memsetValues);
            callInst.insertBefore(lastInst);

            gepInst.removeSelf();
            storeInst.removeSelf();
        }
    }

    //  目前只考虑一维参数数组的初始化
    //  TODO: 考虑二维数组，局部数组
    private boolean canChangeToMemset(IRLoop loop){
        if(!loop.isSimpleLoop() || !loop.isSetIndVar()){
            return false;
        }
        if(loop.getBbs().size() != 2){
            return false;
        }
        BasicBlock head = loop.getHead();
        BasicBlock latch;
        for(BasicBlock latchBb : loop.getLatchBlocks()){
            latch = latchBb;
        }

        int gepCnt = 0;
        int storeCnt = 0;
        for(BasicBlock bb : loop.getBbs()){
            for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                Instruction inst = instNode.getValue();
                if(inst instanceof GepInst _gepInst){
                    gepCnt++;
                    gepInst = _gepInst;
                }
                else if(inst instanceof StoreInst _storeInst){
                    storeCnt++;
                    storeInst = _storeInst;
                }
            }
        }
        //  gep + store
        if(gepCnt != 1 || storeCnt != 1){
            return false;
        }
        //  store 0
        if(!(storeInst.getValue() instanceof ConstInteger constInt)){
            return false;
        }
        if (constInt.getValue() != 0) {
            return false;
        }
        if(!storeInst.getPointer().equals(gepInst)){
            return false;
        }
        if(!(loop.getItInit() instanceof ConstInteger constInt2)){
            return false;
        }
        if(constInt2.getValue() != 0) {
            return false;
        }

        //  目前只考虑参数数组
        PointerType pointerType = (PointerType) gepInst.getTarget().getType();
        if(!pointerType.getEleType().isIntegerTy()){
            return false;
        }

        size = loop.getItEnd();

        return true;
    }

    @Override
    public String getName() {
        return "Loop2Memset";
    }
}
