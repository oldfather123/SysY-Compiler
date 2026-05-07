package cn.edu.bit.newnewcc.pass.ir.util;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.GlobalVariable;
import cn.edu.bit.newnewcc.ir.value.instruction.*;

import java.util.*;

/**
 * 用于高效分析地址是否为全局变量 <br>
 */
// 思路如下：
// 每当遇到一个待定的值，先向上搜索其所有可能的来源，并将所有值标记为待定（pending）
// 若搜索到了全局源，则从全局源开始向下传播全局地址标记
// 最后，没有被标记的待定值都是局部地址，因为其找不到任何全局源
public class GlobalAddressDetector {

    private final Map<Value, Boolean> addressTypeCache = new HashMap<>();
    private final Queue<Value> spreadingGlobalAddresses = new ArrayDeque<>();
    private final Set<Value> pendingAddresses = new HashSet<>();

    // 确定了address为全局地址
    private void addGlobalAddress(Value address) {
        if (addressTypeCache.containsKey(address)) {
            assert addressTypeCache.get(address);
            return;
        }
        addressTypeCache.put(address, true);
        pendingAddresses.remove(address);
        spreadingGlobalAddresses.add(address);
    }

    /**
     * 向上搜索地址源头的过程
     *
     * @param address 待搜索的地址
     */
    // 将所有搜索到的值定义为pending状态
    //
    // 若当前搜索到的是：
    // 确定的全局变量：调用addGlobalAddress
    // 确定的局部变量：什么也不做，全部搜索完毕后会将pending状态的所有值标记为局部变量
    // 由其他地址确定的语句：对其他地址调用judgeAddress
    private void judgeAddress(Value address) {
        if (addressTypeCache.containsKey(address)) return;
        if (pendingAddresses.contains(address)) return;
        pendingAddresses.add(address);
        if (address instanceof GlobalVariable || address instanceof Function.FormalParameter || address instanceof LoadInst) {
            addGlobalAddress(address);
        } else if (address instanceof AllocateInst) {
            // 是局部变量，什么也不做
        } else if (address instanceof GetElementPtrInst getElementPtrInst) {
            judgeAddress(getElementPtrInst.getRootOperand());
        } else if (address instanceof PhiInst phiInst) {
            phiInst.forEach((basicBlock, value) -> judgeAddress(value));
        } else if (address instanceof BitCastInst bitCastInst) {
            judgeAddress(bitCastInst.getSourceOperand());
        } else {
            throw new IllegalArgumentException(String.format("Cannot analysis variable of type %s.", address.getClass()));
        }
    }

    public boolean isGlobalAddress(Value address) {
        if (!addressTypeCache.containsKey(address)) {
            judgeAddress(address);
            while (!spreadingGlobalAddresses.isEmpty()) {
                var globalAddress = spreadingGlobalAddresses.remove();
                // 向下传播全局地址标记
                for (Operand usage : globalAddress.getUsages()) {
                    var userInstruction = usage.getInstruction();
                    // 传播到所有 返回值是地址 且 受全局地址影响 的语句上
                    if (userInstruction instanceof GetElementPtrInst || userInstruction instanceof PhiInst) {
                        addGlobalAddress(userInstruction);
                    }
                }
            }
            for (Value pendingAddress : pendingAddresses) {
                addressTypeCache.put(pendingAddress, false);
            }
            pendingAddresses.clear();
        }
        assert addressTypeCache.containsKey(address);
        assert spreadingGlobalAddresses.isEmpty();
        assert pendingAddresses.isEmpty();
        return addressTypeCache.get(address);
    }

}
