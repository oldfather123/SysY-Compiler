package entities;

import handler.QuaHandler;
import util.IntHandler;

import java.util.*;
import java.util.function.BiFunction;
import java.util.function.Consumer;

public class QuaBlock {

    private final int id;
    private final DoubleList<Quaternion> qua = new DoubleList<>();
    private final LinkedList<QuaBlock> next = new LinkedList<>();
    private final Map<String, FOI> constValues = new HashMap<>();

    public QuaBlock(LinkedList<Quaternion> qua) {
        this.qua.addAll(qua);
        id = IntHandler.get();
    }

    public DoubleList<Quaternion> getQua() {
        return qua;
    }

    public Quaternion getLast() {
        return qua.getTail();
    }

    public Quaternion getFirst() {
        return qua.getHead();
    }

    public void addNext(QuaBlock next) {
        this.next.add(next);
    }

    public void buildQuaternionList(DoubleList<Quaternion> result) {
        Set<Integer> visited = new HashSet<>();
        Queue<QuaBlock> queue = new LinkedList<>();
        queue.add(this);
        while (!queue.isEmpty()) {
            QuaBlock quaBlock = queue.poll();
            if (visited.contains(quaBlock.id)) {
                continue;
            }
            visited.add(quaBlock.id);
            queue.addAll(quaBlock.next);

            result.addAll(quaBlock.qua);
        }
    }

    public void constCalculate() {
        forAllBlock(quaBlock -> {
            DoubleList<Quaternion>.Iterator iterator = quaBlock.qua.iterator();
            while (iterator.hasNext()) {
                Quaternion q = iterator.next();
                for (int i = 0, size = q.getValCount(); i < size; ++i) {
                    if (constValues.containsKey(q.getVal(i).getName())) {
                        q.getVals().set(i, Val.constVal(constValues.get(q.getVal(i).getName())));
                    } else if (QuaHandler.constGlobalVal.containsKey(q.getVal(i).getName())) {
                        q.getVals().set(i, Val.constVal(QuaHandler.constGlobalVal.get(q.getVal(i).getName())));
                    }
                    q.getVal(i).updateConstArgs(constValues);
                }
                switch (q.getOp()) {
                    case PLUS, MINU, MULT, DIV, MOD, SLT, SLE, SGT, SGE, SEQ, SNE -> {
                        Val val1 = q.getVal(1), val2 = q.getVal(2);
                        if (val1.isConst() && val2.isConst()) {
                            BiFunction<FOI, FOI, FOI> func = switch (q.getOp()) {
                                case MINU -> FOI::sub_;
                                case MULT -> FOI::mul_;
                                case DIV -> FOI::div_;
                                case MOD -> FOI::mod_;
                                case SLT -> FOI::lt;
                                case SLE -> FOI::le;
                                case SGT -> FOI::gt;
                                case SGE -> FOI::ge;
                                case SEQ -> FOI::eq;
                                case SNE -> FOI::ne;
                                default -> FOI::add_;
                            };
                            FOI constValue = func.apply(val1.getConstValue(), val2.getConstValue());
                            constValues.put(q.getName(0), constValue);
                            iterator.remove();
                        }
                    }
                }
            }
        });
    }

    private void forAllBlock(Consumer<QuaBlock> func) {
        Set<Integer> visited = new HashSet<>();
        Queue<QuaBlock> queue = new LinkedList<>();
        queue.add(this);
        while (!queue.isEmpty()) {
            QuaBlock quaBlock = queue.poll();
            if (visited.contains(quaBlock.id)) {
                continue;
            }
            visited.add(quaBlock.id);
            queue.addAll(quaBlock.next);
            func.accept(quaBlock);
        }
    }

}
