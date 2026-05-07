package midend;

import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.convop.ZextInstr;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.List;

/**
 * 这个 PASS 其实是对前端生成 IR 的一个修正：
 * 原本对于条件判断中的二元比较需要先拓展成 i32 再和 0 比较转成 i1 以适配多种情况
 * 现在将后面两条多余的指令都省去了
 */
public class BranchInsSimplify {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            for (BasicBlock basicBlock : function.getBasicBlockList()) {
                Instruction ins = basicBlock.getLastInstr();
                Instruction pre;
                while (ins != null) {
                    pre = (Instruction) ins.getPrev();
                    
                    if (ins instanceof CmpInstr cmpInstr
                            && cmpInstr.getOperand1() instanceof ZextInstr zextInstr
                            && zextInstr.getValue() instanceof CmpInstr originCmp
                            && cmpInstr.getOperand2() instanceof IntConst intConst
                            && intConst.getConstVal().intValue() == 0) {
                        
                        if (cmpInstr.getCond() == CmpInstr.CmpCond.INE) {
                            cmpInstr.replaceUseWith(originCmp);
                            
                            cmpInstr.removeFromList();
                            zextInstr.removeFromList();
                        } else if (cmpInstr.getCond() == CmpInstr.CmpCond.IEQ) {
                            cmpInstr.replaceUseWith(originCmp);
                            originCmp.invertCond();
                            
                            cmpInstr.removeFromList();
                            zextInstr.removeFromList();
                        }
                    }
                    
                    ins = pre;
                }
            }
        }
    }
}
