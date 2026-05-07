package opt;

import entities.*;
import handler.QuaHandler;
import util.Strings;

import java.util.*;

public class QuaOptimize {

    private static QuaHandler.QuaResult quaResult;
    private static final List<QuaBlock> quaBlocks = new ArrayList<>();
    private static final Map<String, Integer> labelMap = new HashMap<>();
    public static final Map<String, Set<String>> funcCallMap = new HashMap<>();
    private static final Set<String> visFunc = new HashSet<>();

    public static void opt(QuaHandler.QuaResult quaResult) {
        QuaOptimize.quaResult = quaResult;
        initOpt();
        funcCallMap.clear();
        visFunc.clear();
        detectFuncCall();
        quaBlocksOpt();
    }

    private static void quaBlocksOpt() {
        quaResult.funcOrder().forEach(func -> {
            initOpt();
            DoubleList<Quaternion> quaternionDoubleList = quaResult.funcQua().get(func);
            buildQuaBlocks(quaternionDoubleList);
            buildJumpMap();
            QuaBlock inBlock = quaBlocks.get(0);
            inBlock.constCalculate();
            quaternionDoubleList.clear();
            inBlock.buildQuaternionList(quaternionDoubleList);
            jumpOpt(quaternionDoubleList);
        });
    }

    private static void initOpt() {
        quaBlocks.clear();
        labelMap.clear();
    }

    private static void buildQuaBlocks(DoubleList<Quaternion> funcQua) {
        LinkedList<Quaternion> tempQua = new LinkedList<>();
        DoubleList<Quaternion>.Iterator iter = funcQua.iterator();
        while (iter.hasNext()) {
            Quaternion qua = iter.next();
            if (qua.getOp() == Quaternion.Operator.DEFINE_LABEL) {
                if (!tempQua.isEmpty()) {
                    quaBlocks.add(new QuaBlock(tempQua));
                    tempQua.clear();
                }
                tempQua.add(qua);
                labelMap.put(qua.getName(0), quaBlocks.size());
            } else {
                tempQua.add(qua);
                if (qua.getOp() == Quaternion.Operator.EQZ ||
                        qua.getOp() == Quaternion.Operator.JUMP ||
                        qua.getOp() == Quaternion.Operator.RETURN) {
                    quaBlocks.add(new QuaBlock(tempQua));
                    tempQua.clear();
                }
            }
        }
        if (!tempQua.isEmpty()) {
            quaBlocks.add(new QuaBlock(tempQua));
        }
    }

    private static void buildJumpMap() {
        QuaBlock returnBlock = new QuaBlock(new LinkedList<>());
        quaBlocks.add(returnBlock);
        returnBlock.addNext(returnBlock);
        for (int i = 0, len = quaBlocks.size(); i < len - 1; i++) {
            QuaBlock quaBlock = quaBlocks.get(i);
            Quaternion last = quaBlock.getLast();
            QuaBlock next;
            if (last.getOp() == Quaternion.Operator.JUMP) {
                next = quaBlocks.get(labelMap.get(last.getName(0)));
            } else if (last.getOp() == Quaternion.Operator.RETURN) {
                next = returnBlock;
            } else {
                QuaBlock nextBlock = quaBlocks.get(i + 1);
                if (!nextBlock.getQua().isEmpty()) {
                    Quaternion first = nextBlock.getFirst();
                    if (first.getOp() == Quaternion.Operator.DEFINE_LABEL) {
                        quaBlock.getQua().add(new Quaternion(Quaternion.Operator.JUMP, Val.string(first.getName(0))));
                    } else {
                        Val newJump = Val.label(Strings.NEW_JUMP);
                        quaBlock.getQua().add(new Quaternion(Quaternion.Operator.JUMP, newJump));
                        nextBlock.getQua().addHead(new Quaternion(Quaternion.Operator.DEFINE_LABEL, newJump));
                    }
                }
                next = nextBlock;
            }
            quaBlock.addNext(next);
            if (last.getOp() == Quaternion.Operator.EQZ) {
                QuaBlock nextBlock = quaBlocks.get(labelMap.get(last.getName(1)));
                quaBlock.addNext(nextBlock);
            }
        }
    }

    private static void detectFuncCall() {
        quaResult.funcOrder().forEach(func -> {
            String funcName = func.first;
            funcCallMap.put(funcName, new HashSet<>());
            quaResult.funcQua().get(func).forEach(quaternion -> {
                if (quaternion.getOp() == Quaternion.Operator.CALL) {
                    String name = quaternion.getName(0);
                    if (!Strings.POST_FUNC_NAMES.contains(name)) {
                        funcCallMap.get(funcName).add(name);
                    }
                }
            });
        });
        dfsFunc(Strings.MAIN);
    }

    private static void dfsFunc(String current) {
        if (visFunc.contains(current)) {
            return;
        }
        visFunc.add(current);
        for (String callFunc : funcCallMap.get(current)) {
            dfsFunc(callFunc);
        }
    }

    private static void jumpOpt(DoubleList<Quaternion> quaList) {
        DoubleList<Quaternion>.Iterator iter = quaList.iterator();
        Map<String, DoubleList<DoubleList<Quaternion>.Iterator>> use = new HashMap<>();
        Map<String, DoubleList<Quaternion>.Iterator> def = new HashMap<>();
        while (iter.hasNext()) {
            Quaternion q = iter.next();
            if (q.getOp() == Quaternion.Operator.JUMP) {
                String label = q.getName(0);
                if (!use.containsKey(label)) {
                    use.put(label, new DoubleList<>());
                }
                use.get(label).add(quaList.copyIterator(iter));
            } else if (q.getOp() == Quaternion.Operator.DEFINE_LABEL) {
                def.put(q.getName(0), quaList.copyIterator(iter));
            } else if (q.getOp() == Quaternion.Operator.EQZ) {
                String label = q.getName(1);
                if (!use.containsKey(label)) {
                    use.put(label, new DoubleList<>());
                }
                use.get(label).add(quaList.copyIterator(iter));
            }
        }
        for (int i = 0; i < 2; ++i) {
            // j label --- delete
            //label:
            for (String label : use.keySet()) {
                DoubleList<DoubleList<Quaternion>.Iterator>.Iterator useIter = use.get(label).iterator();
                while (useIter.hasNext()) {
                    iter = useIter.next();
                    if (iter.getObj() != null && iter.getObj().getOp() == Quaternion.Operator.JUMP) {
                        Quaternion nextQ = iter.nextNotMove();
                        if (nextQ != null && nextQ.getOp() == Quaternion.Operator.DEFINE_LABEL
                                && nextQ.getName(0).equals(label)) {
                            iter.remove();
                            useIter.remove();
                        }
                    }
                }
            }

            // label: no use
            Set<String> shouldDelete = new HashSet<>();
            for (String label : def.keySet()) {
                if (use.get(label).isEmpty()) {
                    def.get(label).remove();
                    shouldDelete.add(label);
                }
            }
            for (String label : shouldDelete) {
                def.remove(label);
                use.remove(label);
            }
            shouldDelete.clear();

            //label: --- label -> label2
            // j label2
            for (String label : def.keySet()) {
                iter = def.get(label);
                Quaternion nextQ = iter.nextNotMove();
                if (nextQ != null && nextQ.getOp() == Quaternion.Operator.JUMP) {
                    String to = nextQ.getName(0);
                    DoubleList<DoubleList<Quaternion>.Iterator>.Iterator useIter = use.get(label).iterator();
                    while (useIter.hasNext()) {
                        DoubleList<Quaternion>.Iterator nextIter = useIter.next();
                        Quaternion target = nextIter.getObj();
                        if (target != null) {
                            if(target.getOp() == Quaternion.Operator.JUMP) {
                                target.getVals().set(0, Val.string(to));
                            } else if (target.getOp() == Quaternion.Operator.EQZ) {
                                target.getVals().set(1, Val.string(to));
                            }
                            use.get(to).add(nextIter);
                        }
                    }
                    shouldDelete.add(label);
                    iter.remove();
                }
            }
            for (String label : shouldDelete) {
                def.remove(label);
                use.remove(label);
            }
            shouldDelete.clear();

        }

    }

}
