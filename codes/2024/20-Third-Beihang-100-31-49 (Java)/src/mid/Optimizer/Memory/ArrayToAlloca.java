package mid.Optimizer.Memory;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class ArrayToAlloca {
    /*
        考虑的目标是：
            1. 是局部数组 TODO:全局数组呢？
            2. 所有的gep user都使用constnumber，或是仅有一个gep user
            3. 作为参数传递时，仍满足以上条件 TODO:这里作为参数传递直接忽略了
        对这样的数组，可以收集其所有的gep,将其均替换为alloca
        似乎应该在constfold和func inline之后，mem2reg之前
     */

    public void optimize() {
        for (Function f : IRManager.getModule().getDecledFunctions()) {
            BasicBlock b = f.getEntranceBlock();
            for (Instruction i : new ArrayList<>(b.getInstructionList())) {
                if (!(i instanceof Alloca alloca)) {
                    break;
                }
                if (alloca.getType().equals(ValueType.ARRAY)) {
                    constToVar(alloca);
                    ssaToVar(alloca);
                }
            }
        }
    }

    public void constToVar(Alloca alloca) {
        /*
            每个array及其使用者，key:Alloca，value:Offset(Int)
         */
        HashMap<Instruction, Integer> arrayToOffset = new HashMap<>();

        // 收集其所有使用者
        try {
            collectOffset(alloca, 0, arrayToOffset);
        } catch (Exception ignored) {
            return;
        }

        // 在这里处理memset，替换为若干个store
        for (Instruction v : new HashSet<>(arrayToOffset.keySet())) {
            if (v instanceof Call call && call.getCallingFunction() instanceof LibFunction.Memset memset) {
                Value ptr = call.getOperandList().get(1);
//                Value val = call.getOperandList().get(2);
                BasicBlock block = call.getBlock();
                int index = block.getInstructionList().indexOf(call);

                int base = arrayToOffset.get(v);
                for (Integer i : new HashSet<>(arrayToOffset.values())) {
                    // 暂时认为一定是0
                    GetElementPtr addr = new GetElementPtr(ptr, i);
                    Store splitedStore = new Store(new ConstNumber(0), addr);
                    arrayToOffset.put(addr, base + i);
                    block.addInstructionAt(index, splitedStore);
                    block.addInstructionAt(index, addr);
                }
                arrayToOffset.remove(call);
                call.getBlock().removeInstruction(call);
                call.destroy();
            }
        }


        HashMap<Integer, Alloca> offsetToAlloca = new HashMap<>();
        HashMap<Integer, BasicBlock> allocaInBlock = new HashMap<>();
        ArrayList<BasicBlock> bfsDominTree = Optimizer.instance().bfsDominTreeArray(
                alloca.getBlock().getFunction().getEntranceBlock());
        for (Instruction v : arrayToOffset.keySet()) {
            if (v.equals(alloca)) {
                // 防止a2a中途gep的opreand变化
                continue;
            }
            int offset = arrayToOffset.get(v);
            if (!offsetToAlloca.containsKey(offset)) {
                Alloca replaceAlloca = new Alloca(!v.isFloat());
                offsetToAlloca.put(offset, replaceAlloca);
                v.getBlock().getFunction().getEntranceBlock().addInstrAtEntry(replaceAlloca);
                allocaInBlock.put(offset, v.getBlock());
            } else {
                if (bfsDominTree.indexOf(v.getBlock()) < bfsDominTree.indexOf(allocaInBlock.get(offset))) {
                    v.getBlock().addInstructionAt(
                            v.getBlock().getInstructionList().indexOf(v), offsetToAlloca.get(offset)
                    );
                    allocaInBlock.get(offset).removeInstruction(offsetToAlloca.get(offset));
                    allocaInBlock.put(offset, v.getBlock());
                    offsetToAlloca.get(offset).setBlock(v.getBlock());
                }
            }
            v.beReplacedBy(offsetToAlloca.get(offset));
            v.destroy();
            v.getBlock().removeInstruction(v);
        }
    }

    private void collectOffset(Instruction v, int base, HashMap<Instruction, Integer> arrayToOffset)
            throws Exception {
        arrayToOffset.put(v, base);

        for (User user : v.getUserList()) {
            if (user instanceof GetElementPtr gep) {
                if (gep.getElemIndex() instanceof ConstNumber n) {
                    int index = n.getVal().intValue();
                    collectOffset(gep, base + index, arrayToOffset);
                } else {
                    throw new Exception();
                }
            } else if (user instanceof Call call) {
                if (call.getCallingFunction() instanceof LibFunction.Memset memset) {
                    // 对于一个memset，先检查能否将其替换为若干个store；若可以，则存起来等确定一定可以进行2alloca之后再转换
                    Value size = call.getOperandList().get(3);
                    if (!(size instanceof ConstNumber)) {
                        throw new Exception();
                    }
                    arrayToOffset.put(call, base);
                    continue;
                }
                throw new Exception();
            }
        }
    }

    private void ssaToVar(Alloca alloca) {
        if (alloca.getUserList().size() == 1) {
            User user = alloca.getUserList().get(0);
            if (!(user instanceof GetElementPtr gep)) {
                return;
            }
            if (!gep.getUserList().stream().allMatch(u -> (u instanceof Load || u instanceof Store))) {
                return;
            }

            Alloca replacedAlloca = new Alloca(!gep.isFloat());
            gep.beReplacedBy(replacedAlloca);
            gep.getBlock().getFunction().getEntranceBlock().addInstrAtEntry(replacedAlloca);
            gep.destroy();
            gep.getBlock().removeInstruction(gep);
        }
    }
}
