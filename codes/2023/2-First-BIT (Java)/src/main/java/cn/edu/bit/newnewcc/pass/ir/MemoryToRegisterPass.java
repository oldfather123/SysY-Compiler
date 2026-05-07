package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.AllocateInst;
import cn.edu.bit.newnewcc.ir.value.instruction.LoadInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;
import cn.edu.bit.newnewcc.ir.value.instruction.StoreInst;
import org.antlr.v4.runtime.misc.Pair;

import java.util.*;

/**
 * 将对栈上变量的 load,store 语句尽可能转化为寄存器操作
 */
public class MemoryToRegisterPass {

    private MemoryToRegisterPass() {
    }

    /**
     * 可提升的变量集合，此集合实际上是 latestDefineMap 的 keySet
     */
    private final Set<AllocateInst> promotableAllocateInstructions = new HashSet<>();

    /**
     * 每个变量在每个基本块中最后一次被 store 的值
     */
    private final Map<AllocateInst, Map<BasicBlock, Value>> lastDefineMap = new HashMap<>();

    /**
     * 待填充的 phi 指令列表
     */
    private final Queue<Pair<AllocateInst, PhiInst>> phiFillQueue = new ArrayDeque<>();

    private boolean isPromotableAddress(Value value) {
        if (value instanceof AllocateInst allocateInst) {
            return promotableAllocateInstructions.contains(allocateInst);
        } else {
            return false;
        }
    }

    /**
     * 从函数中提取可提升的 alloca 语句，并将其放入 promotableAllocateInstructions 和 latestDefineMap
     *
     * @param function 待提取的函数
     */
    private void getPromotableVariablesFromFunction(Function function) {
        function.getEntryBasicBlock().getLeadingInstructions().forEach(leadingInstruction -> {
            if (leadingInstruction instanceof AllocateInst allocateInst) {
                boolean isUsedOnlyInLoadStore = true;
                for (Operand operand : allocateInst.getUsages()) {
                    var user = operand.getInstruction();
                    if (!(user instanceof LoadInst || user instanceof StoreInst)) {
                        isUsedOnlyInLoadStore = false;
                        break;
                    }
                }
                if (isUsedOnlyInLoadStore) {
                    promotableAllocateInstructions.add(allocateInst);
                    lastDefineMap.put(allocateInst, new HashMap<>());
                }
            }
        });
    }

    /**
     * 获取变量在基本块中的上一次定义
     * <p>
     * 该函数首先试图从 lastDefineMap 中寻找结果。若结果不存在，则创建phi语句(非入口块)或是默认值(入口块)作为结果 <br>
     *
     * @param function     基本块所在的函数
     * @param basicBlock   基本块
     * @param allocateInst 变量
     * @return 变量在基本块中的上一次定义值
     */
    private Value getLastDefine(Function function, BasicBlock basicBlock, AllocateInst allocateInst) {
        var lastDefineMap = this.lastDefineMap.get(allocateInst);
        // 若该值没有上一次定义，则需要创建phi语句(非入口块)或是默认值(入口块)
        if (!lastDefineMap.containsKey(basicBlock)) {
            if (basicBlock == function.getEntryBasicBlock()) {
                // 若运行到此处，说明原代码未初始化该变量就使用了它
                // 此时应该产生一个ub，但是方便起见我们为其赋予默认初始值
                lastDefineMap.put(basicBlock, allocateInst.getAllocatedType().getZeroInitialization());
            } else {
                var phiInstruction = new PhiInst(allocateInst.getAllocatedType());
                basicBlock.addInstruction(phiInstruction);
                lastDefineMap.put(basicBlock, phiInstruction);
                phiFillQueue.add(new Pair<>(allocateInst, phiInstruction));
            }
        }
        return lastDefineMap.get(basicBlock);
    }

    /**
     * 尽量函数中的的 alloca 语句及其衍生语句替换为寄存器形式
     *
     * @param function 待处理的函数
     */
    private void transformMemoryToRegister(Function function) {
        getPromotableVariablesFromFunction(function);
        // 扫描所有语句，自动匹配块内的 load 与 store ，并创建必须的空 phi 语句，处理得到部分的 lastDefineMap
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (Instruction instruction : basicBlock.getMainInstructions()) {
                if (instruction instanceof LoadInst loadInst) {
                    var address = loadInst.getAddressOperand();
                    if (isPromotableAddress(address)) {
                        // 使用该值的上一次定义替换该 loadInst
                        // 此处未清理该 loadInst ，留着最后与 alloca, store 一并清理
                        loadInst.replaceAllUsageTo(getLastDefine(function, basicBlock, (AllocateInst) address));
                    }
                }
                if (instruction instanceof StoreInst storeInst) {
                    var address = storeInst.getAddressOperand();
                    if (isPromotableAddress(address)) {
                        lastDefineMap.get((AllocateInst) address).put(basicBlock, storeInst.getValueOperand());
                    }
                }
            }
        }
        // 循环处理未填入操作数的 phi 语句
        while (!phiFillQueue.isEmpty()) {
            var pair = phiFillQueue.remove();
            var allocateInst = pair.a;
            var phiInst = pair.b;
            var basicBlock = phiInst.getBasicBlock();
            for (BasicBlock entryBlock : basicBlock.getEntryBlocks()) {
                phiInst.addEntry(entryBlock, getLastDefine(function, entryBlock, allocateInst));
            }
        }
        // 清理多余的 store, load 语句
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (Instruction instruction : basicBlock.getInstructions()) {
                if (instruction instanceof LoadInst loadInst) {
                    var address = loadInst.getAddressOperand();
                    if (isPromotableAddress(address)) {
                        loadInst.waste();
                    }
                } else if (instruction instanceof StoreInst storeInst) {
                    var address = storeInst.getAddressOperand();
                    if (isPromotableAddress(address)) {
                        storeInst.waste();
                    }
                }
            }
        }
        // 清理多余的 alloca 语句
        for (Instruction instruction : function.getEntryBasicBlock().getLeadingInstructions()) {
            if (isPromotableAddress(instruction)) {
                instruction.waste();
            }
        }
    }

    public static void runOnModule(Module module) {
        module.getFunctions().forEach(function -> new MemoryToRegisterPass().transformMemoryToRegister(function));
    }
}
