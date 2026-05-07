package backend.operand;

import backend.component.RiscvInstr;
import ir.pass.analyze.Loop;

import java.util.ArrayList;

public class RiscvReg extends RiscvOperand{
    public boolean isPreColored() {
        return this instanceof RiscvPhyReg;
    }

    public long loopFactor = 0;
    public void addDefOrUse(int loopDepth, Loop loop){
        if(loopDepth==0){
            loopFactor++;
        }else{
//            long tmp = 10;
//            loopDepth --;
//            while(loopDepth>0){
//                tmp*= 10;
//                loopDepth --;
//            }
            loopFactor+=loop.times;
        }
    }
}
