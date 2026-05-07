package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.controller.LifeTimePoint;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmCall;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmJump;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLabel;

import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

public class DeadInstEliminationWithLifeTimeOptimizer implements Optimizer {
    @Override
    public boolean runOn(AsmFunction function) {
        var lifeTimeController = function.getLifeTimeController();
        var instList = function.getInstrList();
        lifeTimeController.getAllRegLifeTime(function.getInstrList());
        Set<AsmInstruction> removeList = new HashSet<>();
        for (var reg : lifeTimeController.getRegKeySet()) {
            LifeTimePoint last = null;
            for (var point : lifeTimeController.getPoints(reg)) {
                if (point.isDef()) {
                    if (last != null && last.isDef()) {
                        var inst = last.getIndex().getSourceInst();
                        if (!(inst instanceof AsmLabel || inst instanceof AsmCall)) {
                            removeList.add(inst);
                        }
                    }
                }
                last = point;
            }
            if (last != null && last.isDef()) {
                var inst = last.getIndex().getSourceInst();
                if (!(inst instanceof AsmLabel || inst instanceof AsmCall)) {
                    removeList.add(inst);
                }
            }
        }
        int count = 0;
        Iterator<AsmInstruction> instIterator = instList.iterator();
        while (instIterator.hasNext()) {
            AsmInstruction inst = instIterator.next();
            if (removeList.contains(inst)) {
                instIterator.remove();
                ++count;
            }
        }
        return count > 0;
    }
}
