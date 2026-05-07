package backend.optimizer;

import backend.RISCVCode.RISCVFunction;
import backend.RISCVCode.RISCVBlock;
import backend.RISCVCode.RISCVInstruction.RISCVInstruction;
import backend.RISCVCode.RISCVInstruction.RISCVLd;
import backend.RISCVCode.RISCVInstruction.RISCVMv;
import backend.RISCVCode.RISCVInstruction.RISCVSd;
import backend.RISCVCode.RISCVOperand;

import java.util.Iterator;
/**
 * LoadStoreMV优化
 */
public class LSOpt implements OptForRISCV {

    @Override
    public void Optimize(RISCVFunction riscvFunction) {
        for (RISCVBlock block : riscvFunction.getBlocks()) {
            Iterator<RISCVInstruction> iterator = block.getInstructions().iterator();
            RISCVInstruction last = null;

            while (iterator.hasNext()) {
                RISCVInstruction now = iterator.next();
                if (last != null) {
                    if (isRedundancyLoad(last, now)) {
                        iterator.remove();
                    } else if (isRedundancyStore(last, now)) {
                        iterator.remove();
                        last = null;
                        continue;
                    } else if (isRedundancyMove(last, now)) {
                        iterator.remove();
                    } else if (isRedundancyMoveSame(now)) {
                        iterator.remove();
                    }
                }
                last = now;
            }
        }
    }

    // 判断是否是冗余的Load指令
    private boolean isRedundancyLoad(RISCVInstruction lastCode, RISCVInstruction nowCode) {
        if (!(lastCode instanceof RISCVSd store) || !(nowCode instanceof RISCVLd load)) {
            return false;
        }
        RISCVOperand loadDest = load.getDest();
        RISCVOperand storeDest = store.getDest();
        RISCVOperand loadAddr = load.getOperand1();
        RISCVOperand storeAddr = store.getOperand1();
        return loadDest.getName().equals(storeDest.getName()) && loadAddr.getName().equals(storeAddr.getName());
    }

    // 判断是否是冗余的Store指令
    private boolean isRedundancyStore(RISCVInstruction lastCode, RISCVInstruction nowCode) {
        if (!(lastCode instanceof RISCVSd lastStore) || !(nowCode instanceof RISCVSd newStore)) {
            return false;
        }
        RISCVOperand lastStoreAddr = lastStore.getOperand1();
        RISCVOperand newStoreAddr = newStore.getOperand1();
        return lastStoreAddr.getName().equals(newStoreAddr.getName());
    }

    // 删除mv t0 t0
    private boolean isRedundancyMoveSame(RISCVInstruction nowCode) {
        if (!(nowCode instanceof RISCVMv riscvMv)) {
            return false;
        }
        RISCVOperand src = riscvMv.getSrc();
        RISCVOperand dest = riscvMv.getDest();
        return src.getName().equals(dest.getName());
    }

    // 删除mv s0 t0, mv t0 s0中的第二条指令
    private boolean isRedundancyMove(RISCVInstruction lastCode, RISCVInstruction nowCode) {
        if (!(lastCode instanceof RISCVMv lastMove) || !(nowCode instanceof RISCVMv newMove)) {
            return false;
        }
        RISCVOperand lastMoveSrc = lastMove.getSrc();
        RISCVOperand lastMoveDest = lastMove.getDest();
        RISCVOperand newMoveSrc = newMove.getSrc();
        RISCVOperand newMoveDest = newMove.getDest();
        return lastMoveDest.getName().equals(newMoveSrc.getName()) && lastMoveSrc.getName().equals(newMoveDest.getName());
    }
}