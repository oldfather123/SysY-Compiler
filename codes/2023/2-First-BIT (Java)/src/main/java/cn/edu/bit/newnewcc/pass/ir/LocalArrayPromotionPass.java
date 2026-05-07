package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.type.ArrayType;
import cn.edu.bit.newnewcc.ir.type.PointerType;
import cn.edu.bit.newnewcc.ir.value.*;
import cn.edu.bit.newnewcc.ir.value.constant.ConstArray;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.instruction.*;
import cn.edu.bit.newnewcc.pass.ir.structure.DomTree;

import java.util.*;

// 思路 & 笔记
//
// 对于每个可提升的局部数组，应当满足：
// 1. 所有写入均在支配树的一条根向路径上
// 2. 所有写入均写入常数（含memset）
// 3. 所有读取，应当在支配树上最后一次写入之后，即位于最后一次写入的基本块的子树内
//
// 无法追踪的地址来源包括：
// 1. 函数传参
// 2. Phi
public class LocalArrayPromotionPass {

    private final Module module;
    private final Function function;
    private final DomTree domTree;

    // 共三类地址：
    // 1. 知道源数组 & 知道偏移量
    // 2. 知道源数组 & 不知道偏移量 (offset=-1)
    // 3. 不知道源数组 (null)
    private static class LocalAddress {
        public final AllocateInst allocateInst;
        private int offset;

        private LocalAddress(LocalAddress localAddress) {
            this.allocateInst = localAddress.allocateInst;
            this.offset = localAddress.offset;
        }

        public LocalAddress(AllocateInst allocateInst) {
            this.allocateInst = allocateInst;
            this.offset = 0;
        }

        public void scale(int rate) {
            if (this.offset == -1) return;
            offset *= rate;
        }

        public void offset(int offset) {
            if (this.offset == -1) return;
            this.offset += offset;
        }

        public int getOffset() {
            return offset;
        }

        public void randomOffset() {
            this.offset = -1;
        }

        public static LocalAddress clone(LocalAddress localAddress) {
            return new LocalAddress(localAddress);
        }

    }

    private enum PromoteState {
        NOT_ACCESSED,
        WRITING,
        READING,
        WRITTEN,
        FAILED
    }

    private final Map<AllocateInst, PromoteState> stateMap = new HashMap<>();
    private final Map<AllocateInst, SortedMap<Integer, Constant>> arrayElementMap = new HashMap<>();
    private final Map<Instruction, LocalAddress> addressMap = new HashMap<>();

    private LocalArrayPromotionPass(Module module, Function function) {
        this.module = module;
        this.function = function;
        this.domTree = DomTree.buildOver(function);
    }

    private void collectPotentialArrays() {
        for (Instruction leadingInstruction : function.getEntryBasicBlock().getLeadingInstructions()) {
            if (leadingInstruction instanceof AllocateInst allocateInst && allocateInst.getAllocatedType() instanceof ArrayType arrayType) {
                addressMap.put(allocateInst, new LocalAddress(allocateInst));
                stateMap.put(allocateInst, PromoteState.NOT_ACCESSED);
                arrayElementMap.put(allocateInst, new TreeMap<>());
            }
        }
    }

    private void collectAddressInfo(BasicBlock block) {
        for (Instruction instruction : block.getInstructions()) {
            if (isInheritUse(instruction)) {
                if (instruction instanceof GetElementPtrInst getElementPtrInst) {
                    var rootAddressValue = getElementPtrInst.getRootOperand();
                    if (!(rootAddressValue instanceof Instruction)) continue;
                    var rootAddress = addressMap.get(rootAddressValue);
                    if (rootAddress == null) continue;
                    var currentAddress = LocalAddress.clone(rootAddress);
                    var currentType = ((PointerType) rootAddressValue.getType()).getBaseType();
                    for (int i = 0; i < getElementPtrInst.getIndicesSize(); i++) {
                        if (i != 0) {
                            var arrayType = (ArrayType) currentType;
                            currentAddress.scale(arrayType.getLength());
                            currentType = arrayType.getBaseType();
                        }
                        if (getElementPtrInst.getIndexAt(i) instanceof ConstInt constInt) {
                            currentAddress.offset(constInt.getValue());
                        } else {
                            currentAddress.randomOffset();
                        }
                    }
                    addressMap.put(getElementPtrInst, currentAddress);
                } else if (instruction instanceof BitCastInst bitCastInst) {
                    var rootAddressValue = bitCastInst.getSourceOperand();
                    if (!(rootAddressValue instanceof Instruction)) continue;
                    var rootAddress = addressMap.get(rootAddressValue);
                    if (rootAddress == null) continue;
                    var currentAddress = LocalAddress.clone(rootAddress);
                    // 计算放缩率
                    var currentType = bitCastInst.getSourceType();
                    assert currentType instanceof PointerType;
                    currentType = ((PointerType) currentType).getBaseType();
                    var targetType = bitCastInst.getTargetType();
                    assert targetType instanceof PointerType;
                    targetType = ((PointerType) targetType).getBaseType();
                    int rate = (int) (currentType.getSize() / targetType.getSize());
                    // 计算完毕
                    currentAddress.scale(rate);
                    addressMap.put(bitCastInst, currentAddress);
                } else {
                    // 未来可能添加其他的地址读语句
                    throw new CompilationProcessCheckFailedException();
                }
                // 检查该地址是否被dirty语句使用。如果是，标记源地址无法提升
                if (addressMap.containsKey(instruction)) {
                    var address = addressMap.get(instruction);
                    boolean isDirty = false;
                    for (Operand usage : instruction.getUsages()) {
                        if (isDirtyUse(usage.getInstruction())) {
                            isDirty = true;
                            break;
                        }
                    }
                    if (isDirty) {
                        stateMap.put(address.allocateInst, PromoteState.FAILED);
                    }
                }
            }
        }
        for (BasicBlock domSon : domTree.getDomSons(block)) {
            collectAddressInfo(domSon);
        }
    }

    private void collectArrayElementInfo(BasicBlock block) {
        var writtenAddresses = new HashSet<AllocateInst>();
        for (Instruction instruction : block.getInstructions()) {
            if (isReadUse(instruction)) {
                if (instruction instanceof LoadInst loadInst) {
                    var addressValue = loadInst.getAddressOperand();
                    if (addressValue instanceof Instruction && addressMap.containsKey(addressValue)) {
                        var address = addressMap.get(addressValue);
                        PromoteState nextState = switch (stateMap.get(address.allocateInst)) {
                            case NOT_ACCESSED, WRITING, READING -> PromoteState.READING;
                            case WRITTEN, FAILED -> PromoteState.FAILED;
                        };
                        stateMap.put(address.allocateInst, nextState);
                    }
                } else {
                    // 未来可能添加其他的地址读语句
                    throw new CompilationProcessCheckFailedException();
                }
            } else if (isWriteUse(instruction)) {
                if (instruction instanceof StoreInst storeInst) {
                    var addressValue = storeInst.getAddressOperand();
                    if (addressValue instanceof Instruction && addressMap.containsKey(addressValue)) {
                        var address = addressMap.get(addressValue);
                        PromoteState nextState;
                        if (address.offset != -1 && storeInst.getValueOperand() instanceof Constant storedValue) {
                            arrayElementMap.get(address.allocateInst).put(address.offset, storedValue);
                            nextState = switch (stateMap.get(address.allocateInst)) {
                                case NOT_ACCESSED, WRITING -> PromoteState.WRITING;
                                case READING, WRITTEN, FAILED -> PromoteState.FAILED;
                            };
                        } else {
                            nextState = PromoteState.FAILED;
                        }
                        stateMap.put(address.allocateInst, nextState);
                        writtenAddresses.add(address.allocateInst);
                    }
                } else if (instruction instanceof CallInst callInst) {
                    assert callInst.getCallee().getValueName().equals("memset");
                    assert (((ConstInt) callInst.getArgumentAt(1)).getValue() & 0xff) == 0;
                    var addressValue = callInst.getArgumentAt(0); // memset 的地址是第一个参数
                    if (addressValue instanceof Instruction && addressMap.containsKey(addressValue)) {
                        var address = addressMap.get(addressValue);
                        assert address.offset == 0;
                        arrayElementMap.get(address.allocateInst).clear();
                        PromoteState nextState = switch (stateMap.get(address.allocateInst)) {
                            case NOT_ACCESSED, WRITING -> PromoteState.WRITING;
                            case READING, WRITTEN, FAILED -> PromoteState.FAILED;
                        };
                        stateMap.put(address.allocateInst, nextState);
                        writtenAddresses.add(address.allocateInst);
                    }
                } else {
                    // 未来可能添加其他的地址写语句
                    throw new CompilationProcessCheckFailedException();
                }
            }
        }
        for (BasicBlock domSon : domTree.getDomSons(block)) {
            collectArrayElementInfo(domSon);
        }
        for (AllocateInst writtenAddress : writtenAddresses) {
            PromoteState nextState = switch (stateMap.get(writtenAddress)) {
                case NOT_ACCESSED -> throw new CompilationProcessCheckFailedException(); // 没写过为什么会在 writtenAddress 里
                case WRITING, READING, WRITTEN -> PromoteState.WRITTEN;
                case FAILED -> PromoteState.FAILED;
            };
            stateMap.put(writtenAddress, nextState);
        }
    }

    private static class ConstArrayBuilder {
        private SortedMap<Integer, Constant> elementMap;
        private Iterator<Integer> it;
        private boolean hasNextKv;
        private int nextKey;
        private Constant nextValue;

        private ConstArrayBuilder() {
        }

        private void moveIterator() {
            // 自动跳过零初始化的值
            do {
                hasNextKv = it.hasNext();
                if (hasNextKv) {
                    nextKey = it.next();
                    nextValue = elementMap.get(nextKey);
                }
            } while (hasNextKv && nextValue == nextValue.getType().getZeroInitialization());
        }

        private ConstArray dfsBuild(ArrayType arrayType, int baseOffset) {
            int arraySize = arrayType.getFlattenedType().getLength();
            int arrayLength = arrayType.getLength();
            int subArraySize = arraySize / arrayLength;
            var initList = new ArrayList<Constant>();
            while (hasNextKv && nextKey < baseOffset + arraySize) {
                int index = (nextKey - baseOffset) / subArraySize;
                while (initList.size() < index) {
                    initList.add(arrayType.getBaseType().getZeroInitialization());
                }
                if (arrayType.getBaseType() instanceof ArrayType subArrayType) {
                    initList.add(dfsBuild(subArrayType, baseOffset + index * subArraySize));
                } else {
                    initList.add(nextValue);
                    moveIterator();
                }
            }
            return new ConstArray(arrayType.getBaseType(), arrayType.getLength(), initList);
        }

        private ConstArray build_(ArrayType arrayType, SortedMap<Integer, Constant> elementMap) {
            this.elementMap = elementMap;
            this.it = elementMap.keySet().iterator();
            moveIterator();
            return dfsBuild(arrayType, 0);
        }

        public static ConstArray build(ArrayType arrayType, SortedMap<Integer, Constant> elementMap) {
            return new ConstArrayBuilder().build_(arrayType, elementMap);
        }
    }

    private boolean promoteArrays() {
        boolean changed = false;
        for (AllocateInst allocateInst : stateMap.keySet()) {
            if (stateMap.get(allocateInst) == PromoteState.FAILED) continue;
            var constArray = ConstArrayBuilder.build((ArrayType) allocateInst.getAllocatedType(), arrayElementMap.get(allocateInst));
            var gv = new GlobalVariable(true, constArray);
            module.addGlobalVariable(gv);
            allocateInst.replaceAllUsageTo(gv);
            changed = true;
        }
        return changed;
    }

    private void clearPromotedWrites() {
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (Instruction instruction : basicBlock.getInstructions()) {
                if (isWriteUse(instruction)) {
                    Value addressValue;
                    if (instruction instanceof StoreInst storeInst) {
                        addressValue = storeInst.getAddressOperand();
                    } else if (instruction instanceof CallInst callInst) {
                        addressValue = callInst.getArgumentAt(0);
                    } else {
                        // 未来可能添加其他的地址写语句
                        throw new CompilationProcessCheckFailedException();
                    }
                    if (addressValue instanceof Instruction && addressMap.containsKey(addressValue)) {
                        var address = addressMap.get(addressValue);
                        if (stateMap.get(address.allocateInst) != PromoteState.FAILED) {
                            instruction.waste();
                        }
                    }
                }
            }
        }
    }

    private boolean runOnFunction() {
        collectPotentialArrays();
        if (stateMap.size() == 0) return false;
        collectAddressInfo(domTree.getDomRoot());
        collectArrayElementInfo(domTree.getDomRoot());
        boolean changed = promoteArrays();
        clearPromotedWrites();
        return changed;
    }

    private static boolean isInheritUse(Instruction instruction) {
        return instruction instanceof GetElementPtrInst ||
                instruction instanceof BitCastInst;
    }

    private static boolean isWriteUse(Instruction instruction) {
        if (instruction instanceof StoreInst) {
            return true;
        } else if (instruction instanceof CallInst callInst) {
            return callInst.getCallee().getValueName().equals("memset");
        } else {
            return false;
        }
    }

    private static boolean isReadUse(Instruction instruction) {
        return instruction instanceof LoadInst;
    }

    private static boolean isDirtyUse(Instruction instruction) {
        return instruction instanceof PhiInst ||
                (instruction instanceof CallInst callInst && !callInst.getCallee().getValueName().equals("memset"));
    }

    public static boolean runOnModule(Module module) {
        boolean changed = false;
        for (Function function : module.getFunctions()) {
            changed |= new LocalArrayPromotionPass(module, function).runOnFunction();
        }
        return changed;
    }
}
