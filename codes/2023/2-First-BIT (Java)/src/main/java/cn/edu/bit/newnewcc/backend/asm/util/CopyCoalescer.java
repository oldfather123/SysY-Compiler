package cn.edu.bit.newnewcc.backend.asm.util;

import cn.edu.bit.newnewcc.backend.asm.controller.LifeTimeController;
import cn.edu.bit.newnewcc.backend.asm.controller.LifeTimeInterval;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.backend.asm.operand.RegisterReplaceable;

import java.util.*;

public class CopyCoalescer {
    private final ArrayList<AsmInstruction> instructions;
    private final LifeTimeController lifeTimeController;
    private final Map<LifeTimeInterval, LifeTimeInterval> value = new HashMap<>();
    private final DSU<Integer> dsu = new DSU<>();
    private final List<LifeTimeInterval> intervals = new ArrayList<>();
    private final Map<Integer, Set<Integer>> clique = new HashMap<>();
    private final Map<Integer, Set<Integer>> edges = new HashMap<>();

    public CopyCoalescer(ArrayList<AsmInstruction> instructions, LifeTimeController lifeTimeController) {
        this.instructions = instructions;
        this.lifeTimeController = lifeTimeController;
    }

    private LifeTimeInterval getValue(LifeTimeInterval lifeTimeInterval) {
        if (!value.get(lifeTimeInterval).equals(lifeTimeInterval)) {
            value.put(lifeTimeInterval, getValue(value.get(lifeTimeInterval)));
        }
        return value.get(lifeTimeInterval);
    }

    private void getIntervals() {
        for (int x : lifeTimeController.getVRegKeySet()) {
            intervals.addAll(lifeTimeController.getInterval(x));
        }
        Collections.sort(intervals);
    }

    private void getValues() {
        Map<Integer, LifeTimeInterval> lastActive = new HashMap<>();
        for (var interval : intervals) {
            value.put(interval, interval);
            int defID = interval.range.a.getInstID();
            var inst = instructions.get(defID);
            if (AsmInstructions.isMoveVToV(inst)) {
                var id = AsmInstructions.getMoveVReg(inst);
                if (id.a.equals(interval.getVRegID())) {
                    var sourceID = id.b;
                    value.put(interval, getValue(lastActive.get(sourceID)));
                    dsu.merge(interval.getVRegID(), sourceID);
                }
            }
            lastActive.put(interval.getVRegID(), interval);
        }
    }

    private void addEdge(int x, int y) {
        if (x == y) {
            throw new RuntimeException("coalesce error! conference on same value");
        }
        edges.get(x).add(y);
        edges.get(y).add(x);
    }

    private void removeEdge(int x, int y) {
        edges.get(x).remove(y);
        edges.get(y).remove(x);
    }

    private void getEdges() {
        Set<LifeTimeInterval> activeSet = new HashSet<>();
        for (var x : lifeTimeController.getVRegKeySet()) {
            edges.put(x, new HashSet<>());
        }
        for (var now : intervals) {
            activeSet.removeIf(last -> last.range.b.compareTo(now.range.a) < 0);
            for (var last : activeSet) {
                if (!getValue(last).equals(getValue(now)) && dsu.getfa(last.getVRegID()).equals(dsu.getfa(now.getVRegID()))) {
                    addEdge(last.getVRegID(), now.getVRegID());
                }
            }
            activeSet.add(now);
        }
        for (var i : lifeTimeController.getVRegKeySet()) {
            int x = dsu.getfa(i);
            if (!clique.containsKey(x)) {
                clique.put(x, new HashSet<>());
            }
            clique.get(x).add(i);
        }
    }

    public Register getVReg(Integer x) {
        return lifeTimeController.getVReg(x);
    }

    private final List<Integer> constructList = new ArrayList<>();
    private void coalesce() {
        Map<Register, Register> trueValue = new HashMap<>();
        for (var x : lifeTimeController.getVRegKeySet()) {
            trueValue.put(getVReg(x), getVReg(x));
        }
        for (var x : clique.keySet()) {
            var points = clique.get(x);
            if (points.isEmpty()) {
                continue;
            }
            while (true) {
                int maxID = -1, maxValue = -1;
                for (var point : points) {
                    if (edges.get(point).size() > maxValue) {
                        maxValue = edges.get(point).size();
                        maxID = point;
                    }
                }
                if (maxValue <= 0) {
                    break;
                }
                points.remove(maxID);
                var out = edges.get(maxID).toArray();
                for (var u : out) {
                    removeEdge(maxID, (Integer)u);
                }
            }
            var array = points.toArray();
            int v = (Integer)array[0];
            for (int i = 1; i < array.length; i++) {
                int u = (Integer)array[i];
                trueValue.put(getVReg(u), getVReg(v));
                lifeTimeController.mergePoints(v, u);
            }
            constructList.add(v);
        }
        for (var inst : instructions) {
            for (int i : AsmInstructions.getVRegId(inst)) {
                var rp = (RegisterReplaceable) inst.getOperand(i);
                Register reg = rp.getRegister();
                inst.setOperand(i, rp.withRegister(trueValue.get(getVReg(reg.getAbsoluteIndex()))));
            }
        }
    }

    private List<AsmInstruction> instructionFilter() {
        List<AsmInstruction> newInstructionList = new ArrayList<>();
        for (var inst : instructions) {
            if (AsmInstructions.isMoveVToV(inst)) {
                var r = AsmInstructions.getMoveVReg(inst);
                if (r.a.equals(r.b)) {
                    continue;
                }
            }
            newInstructionList.add(inst);
        }
        lifeTimeController.buildInstID(newInstructionList);
        for (var x : constructList) {
            lifeTimeController.constructInterval(lifeTimeController.getVReg(x));
        }
        return newInstructionList;
    }

    public List<AsmInstruction> work() {
        getIntervals();
        getValues();
        getEdges();
        coalesce();
        return instructionFilter();
    }
}
