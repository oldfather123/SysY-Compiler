package mid.Optimizer.Memory;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Instruction.GlobalDecl;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.ControllFlow.ControlFlowGraph;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class LoadLVN {
    /*
        遍历支配树上的load-store指令，可以以简单的形式进行标号
     */
    private final ControlFlowGraph CFG = Optimizer.instance().getCFG();

    public void optimize() {
        agreesiveLVN();
        loadLVNOpt();
    }

    public void agreesiveLVN() {
        /*
            激进的内存优化，包括：
            1. 没有对应load或call的地址及其所有user删除 （更复杂的情况留给a2a）
            2. 没有store或call指令的全局数组，load或call指令直接代替
         */

        HashSet<Value> firstTypeAddrs = new HashSet<>();

        for (Function f : IRManager.getModule().getDecledFunctions()) {
            for (BasicBlock b : f.getBlocks()) {
                for (Instruction instruction : b.getInstructionList()) {
                    if (instruction instanceof Alloca alloca && firstTypeAddrCheck(alloca)) {
                        firstTypeAddrs.add(alloca);
                    }
                }
            }
        }

        for (GlobalDecl globalDecl : IRManager.getModule().getGlobalDecls()) {
            if (firstTypeAddrCheck(globalDecl)) {
                firstTypeAddrs.add(globalDecl);
            }
        }

        firstTypeAddrs.forEach(this::destoryAllUser);

        // 下面是全局数组的替换
        ArrayList<GlobalDecl> secondTypeAddrs = new ArrayList<>();
        for (GlobalDecl globalDecl : IRManager.getModule().getGlobalDecls()) {
            if (globalDecl.isArray() &&
                    globalDecl.getUserList().stream().noneMatch(user -> (user instanceof Store || user instanceof Call))
                    && secondTypeAddrCheck(globalDecl)) {
                secondTypeAddrs.add(globalDecl);
            }
        }

        for (GlobalDecl decl : secondTypeAddrs) {
            HashMap<Value, Integer> arrayToOffset = new HashMap<>();
            collectOffset(decl, 0, arrayToOffset);
            for (Value v : arrayToOffset.keySet()) {
                int offset = arrayToOffset.get(v);
                for (User user : v.getUserList()) {
                    if (user instanceof Load load) {
                        load.beReplacedBy((new ConstNumber(decl.getConstValAtIndex(offset))).withType(
                                !load.isFloat()
                        ));
                        load.getBlock().removeInstruction(load);
                        load.destroy();
                    }
                }
            }
        }
    }

    private boolean firstTypeAddrCheck(Value addr) {
        /*
            若考虑其gep调用树，均没有load指令
         */
        for (User user : addr.getUserList()) {
            if (user instanceof Load) {
                return false;
            }
            if (user instanceof Call call) {
                Function f = call.getCallingFunction();
                if (f instanceof LibFunction) {
                    return false;
                }
                if (f.equals(call.getBlock().getFunction())) {
                    continue;
                }
                if (firstTypeAddrCheck(f.getParam().getParams().get(
                        call.getOperandList().indexOf(addr) - 1))) {
                    continue;
                }
                return false;
            } else if (user instanceof GetElementPtr gep) {
                if (!firstTypeAddrCheck(gep)) {
                    return false;
                }
            }
        }
        return true;
    }

    private boolean secondTypeAddrCheck(Value addr) {
        /*
            若考虑其gep调用树，均没有store指令
         */
        for (User user : addr.getUserList()) {
            if (user instanceof Store || user instanceof Call) {
                return false;
            } else if (user instanceof GetElementPtr gep) {
                if (!secondTypeAddrCheck(gep)) {
                    return false;
                }
            }
        }
        return true;
    }

    private void destoryAllUser(Value v) {
        for (User user : v.getUserList()) {
            destoryAllUser(user);
        }
        if (v instanceof GlobalDecl globalDecl) {
            IRManager.getModule().removeGlobalDecl(globalDecl);
            globalDecl.destroy();
        } else if (v instanceof Instruction i) {
            i.getBlock().removeInstruction(i);
            i.destroy();
        }
    }

    private void collectOffset(Value v, int base, HashMap<Value, Integer> arrayToOffset) {
        // 仅收集有价值（也就是偏移为常量）的gep
        arrayToOffset.put(v, base);

        for (User user : v.getUserList()) {
            if (user instanceof GetElementPtr gep) {
                if (gep.getElemIndex() instanceof ConstNumber n) {
                    int index = n.getVal().intValue();
                    collectOffset(gep, base + index, arrayToOffset);
                }
            }
        }
    }


    public void loadLVNOpt() {
        for (Function f : IRManager.getModule().getDecledFunctions()) {
            /*
            一段连续的支配序列，其中的每个块都仅有一个（或零个）后继
            将所有块拆分成若干个连续支配序列进行分析，每段序列中的store+若干load可以进行LVN
            */
            HashSet<ArrayList<BasicBlock>> continuousDominBlocks =
                    continousBlockSeq(f.getEntranceBlock(), new HashSet<>());
            for (ArrayList<BasicBlock> continousBlock : continuousDominBlocks) {
                HashMap<GetElementPtr, Value> gepToStorage = new HashMap<>();
                for (BasicBlock b : continousBlock) {
                    for (Instruction instr : new ArrayList<>(b.getInstructionList())) {
                        if (instr instanceof Load load) {
                            Value addr = load.getAddr();
                            if (!(addr instanceof GetElementPtr gep)) {
                                continue;
                            }
                            if (gepToStorage.containsKey(gep)) {
                                load.beReplacedBy(gepToStorage.get(gep));
                                load.getBlock().removeInstruction(load);
                                load.destroy();
                            } else {
                                gepToStorage.put(gep, load);
                            }
                        } else if (instr instanceof Store store) {
                            Value addr = store.getAddr();
                            if (!(addr instanceof GetElementPtr gep)) {
                                continue;
                            }
                            HashSet<GetElementPtr> relatedAddrs = new HashSet<>();
                            Value ptr = gep.getPtr();
                            for (GetElementPtr gepi : gepToStorage.keySet()) {
                                if (gepi.getPtr().equals(ptr)) {
                                    relatedAddrs.add(gepi);
                                }
                            }
                            for (GetElementPtr gepi : relatedAddrs) {
                                gepToStorage.remove(gepi);
                            }
                            gepToStorage.put(gep, store.getSrc());
                        } else if (instr instanceof Call call &&
                                Optimizer.instance().hasSideEffect(call.getCallingFunction())) {
                            boolean modified = false;
                            // TODO: call一定修改内存吗
                            for (Value v : call.getOperandList()) {
                                if (v instanceof GetElementPtr gep) {
                                    modified = true;
                                    HashSet<GetElementPtr> relatedAddrs = new HashSet<>();
                                    Value ptr = gep.getPtr();
                                    for (GetElementPtr gepi : gepToStorage.keySet()) {
                                        if (gepi.getPtr().equals(ptr)) {
                                            relatedAddrs.add(gepi);
                                        }
                                    }
                                    for (GetElementPtr gepi : relatedAddrs) {
                                        gepToStorage.remove(gepi);
                                    }
                                }
                            }
                            if (modified) {
                                for (GlobalDecl ptr : IRManager.getModule().getGlobalDecls()) {
                                    if (!ptr.isArray()) {
                                        continue;
                                    }
                                    HashSet<GetElementPtr> relatedAddrs = new HashSet<>();
                                    for (GetElementPtr gepi : gepToStorage.keySet()) {
                                        if (gepi.getPtr().equals(ptr)) {
                                            relatedAddrs.add(gepi);
                                        }
                                    }
                                    for (GetElementPtr gepi : relatedAddrs) {
                                        gepToStorage.remove(gepi);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    }

    public HashSet<ArrayList<BasicBlock>> continousBlockSeq(BasicBlock head, HashSet<BasicBlock> visited) {
        HashSet<ArrayList<BasicBlock>> seqs = new HashSet<>();
        ArrayList<BasicBlock> seq = new ArrayList<>();
        BasicBlock curBlock = head;
        while (true) {
            visited.add(curBlock);
            HashSet<BasicBlock> children = CFG.getChildren(curBlock);
            HashSet<BasicBlock> parents = CFG.getParents(curBlock);
            if (parents != null && parents.size() > 1) {
                seqs.add(seq);
                ArrayList<BasicBlock> newSeq = new ArrayList<>();
                newSeq.add(curBlock);
                if (children == null || children.size() == 0) {
                    seqs.add(newSeq);
                    return seqs;
                } else if (children.size() == 1) {
                    BasicBlock child = (BasicBlock) children.toArray()[0];
                    seqs.addAll(continousBlockSeq(child, visited));
                    for (ArrayList<BasicBlock> sqeI : seqs) {
                        if (sqeI.size() > 0 && sqeI.get(0).equals(child)) {
                            sqeI.add(0, curBlock);
                            break;
                        }
                    }
                    return seqs;
                } else {
                    seqs.add(newSeq);
                    for (BasicBlock child : children) {
                        if (!visited.contains(child)) {
                            seqs.addAll(continousBlockSeq(child, visited));
                        }
                    }
                    return seqs;
                }
            }

            seq.add(curBlock);
            if (children == null || children.size() == 0) {
                seqs.add(seq);
                return seqs;
            } else if (children.size() == 1) {
                curBlock = (BasicBlock) children.toArray()[0];
            } else {
                seqs.add(seq);
                for (BasicBlock child : children) {
                    if (!visited.contains(child)) {
                        seqs.addAll(continousBlockSeq(child, visited));
                    }
                }
                return seqs;
            }
        }
    }
}
