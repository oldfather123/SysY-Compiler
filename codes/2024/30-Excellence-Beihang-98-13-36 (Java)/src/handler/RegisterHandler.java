package handler;

import entities.*;
import opt.QuaOptimize;
import util.Strings;

import java.util.*;

import static entities.Registers.*;

public class RegisterHandler {

    private static int freeRegistersRCount = S_SIZE, freeRegistersFCount = FS_SIZE;
    public static final Map<String, Register> mem2regR = new HashMap<>(), mem2regF = new HashMap<>();

    public static void handle(Map<String, DoubleList<Assembler>> assemblerMap, boolean shouldAllocate) {
        if (shouldAllocate) {
            assemblerMap.forEach((k, v) -> handle(v));
        } else {
            freeRegistersRCount = S_SIZE;
            freeRegistersFCount = FS_SIZE;
            mem2regR.clear();
            mem2regF.clear();
            assemblerMap.forEach((k, v) -> detect(v));
            buildMem2Reg();
        }
    }

    private static void handle(DoubleList<Assembler> assemblerList) {
        RegisterAllocator allocator = new RegisterAllocator();
        DoubleList<Assembler>.Iterator iter = assemblerList.iterator();
        for (int idx = 0; iter.hasNext(); idx++) {
            if (iter.next() instanceof Statement statement) {
                List<Register> args = statement.getArgs();
                for (Register reg : args) {
                    if (reg.is(Register.Type.TEMP_R)) {
                        allocator.putLineR(reg.getNum(), idx);
                    } else if (reg.is(Register.Type.TEMP_F)) {
                        allocator.putLineS(reg.getNum(), idx);
                    }
                }
            }
        }
        int maxUsedRegister = 0, delta = 0;
        iter = assemblerList.iterator();
        for (int idx = 0; iter.hasNext(); idx++) {
            Assembler assembler = iter.next();
            if (assembler instanceof Statement statement) {
                List<Register> args = statement.getArgs();
                for (int i = 0; i < args.size(); i++) {
                    Register reg = args.get(i);
                    if (reg.is(Register.Type.TEMP_R)) {
                        if (reg.isAddress()) {
                            args.set(i, allocator.getR(reg.getNum()).addr());
                        } else {
                            args.set(i, allocator.getR(reg.getNum()));
                        }
                    } else if (reg.is(Register.Type.TEMP_F)) {
                        args.set(i, allocator.getF(reg.getNum()));
                    }
                }
                allocator.tryFreeR(idx - delta);
                allocator.tryFreeS(idx - delta);
            } else if (assembler instanceof Mark mark) {
                Pair<List<Integer>, List<Integer>> usedRegister = allocator.getUsedRegister();
                maxUsedRegister = Math.max(maxUsedRegister, usedRegister.first.size() + usedRegister.second.size());
                iter.remove();
                int now = 8 + mark.getValue();
                if (mark.type == Mark.Type.LOAD) {
                    for (int rNum : usedRegister.first) {
                        iter.add(new Statement(Statement.Op.LD, r(rNum), Registers.sp(now)));
                        now += 8;
                        delta++;
                        idx++;
                    }
                    for (int sNum : usedRegister.second) {
                        iter.add(new Statement(Statement.Op.FLD, f(sNum), Registers.sp(now)));
                        now += 8;
                        delta++;
                        idx++;
                    }
                } else {
                    for (int rNum : usedRegister.first) {
                        iter.add(new Statement(Statement.Op.SD, r(rNum), Registers.sp(now)));
                        now += 8;
                        delta++;
                        idx++;
                    }
                    for (int sNum : usedRegister.second) {
                        iter.add(new Statement(Statement.Op.FSD, f(sNum), Registers.sp(now)));
                        now += 8;
                        delta++;
                        idx++;
                    }
                }
                idx--;
                delta--;
            }
        }
        for (iter = assemblerList.iterator(); iter.hasNext(); ) {
            Assembler assembler = iter.next();
            if (assembler instanceof Statement statement) {
                List<Register> args = statement.getArgs();
                for (int i = 0; i < args.size(); i++) {
                    Register reg = args.get(i);
                    if (reg.is(Register.Type.TEMP_D)) {
                        int num = reg.getNum();
                        int newNum = (num > 0 ? 1 : -1) * (Math.abs(num) - (ALL_SIZE - maxUsedRegister + allocator.getExtra()) * 8);
                        args.set(i, Register.num(newNum));
                    }
                }
            }
        }
        for (iter = assemblerList.iterator(); iter.hasNext(); ) {
            if (iter.next() instanceof Statement statement) {
                if (statement.opIs(Statement.Op.ADDI)) {
                    int val = statement.getArg(2).getNum();
                    if (val < -2048 || val > 2047) {
                        iter.prev();
                        iter.add(new Statement(Statement.Op.LI, a(4), statement.getArg(2)));
                        iter.next();
                        iter.set(new Statement(Statement.Op.ADD, statement.getArg(0), statement.getArg(1), a(4)));
                    }
                }
            }
        }
        for (iter = assemblerList.iterator(); iter.hasNext(); ) {
            Assembler assembler = iter.next();
            if (assembler instanceof Statement statement) {
                for (int idx2 = 0, size = statement.getArgs().size(); idx2 < size; ++idx2) {
                    Register reg = statement.getArgs().get(idx2);
                    String label = reg.getLabel();
                    if (label != null) {
                        if (label.contains(Strings.RIGHT_ARROW)) {
                            String[] names = label.split(Strings.RIGHT_ARROW);
                            statement.getArgs().set(idx2, Register.parse(names[0]));
                            iter.prev();
                            iter.add(new Statement(Statement.Op.SD, Register.parse(names[0]), Registers.sp(-Integer.parseInt(names[1]) * 8 + 8)));
                        } else if (label.contains(Strings.LEFT_ARROW)) {
                            String[] names = label.split(Strings.LEFT_ARROW);
                            statement.getArgs().set(idx2, Register.parse(names[1]));
                            iter.prev();
                            iter.add(new Statement(Statement.Op.LD, Register.parse(names[1]), Registers.sp(-Integer.parseInt(names[0]) * 8 + 8)));
                        } else if (label.contains(Strings.DOUBLE_ARROW)) {
                            String[] names = label.split(Strings.DOUBLE_ARROW);
                            statement.getArgs().set(idx2, Register.parse(names[1]));
                            iter.prev();
                            iter.add(new Statement(Statement.Op.SD, Register.parse(names[1]), Registers.sp(-Integer.parseInt(names[2]) * 8 + 8)));
                            iter.add(new Statement(Statement.Op.LD, Register.parse(names[1]), Registers.sp(-Integer.parseInt(names[0]) * 8 + 8)));
                        }
                    }
                }
            }
        }
    }

    private static void detect(DoubleList<Assembler> assemblerList) {
        RegisterAllocator allocator = new RegisterAllocator();
        DoubleList<Assembler>.Iterator iter = assemblerList.iterator();
        for (int idx = 0; iter.hasNext(); idx++) {
            if (iter.next() instanceof Statement statement) {
                List<Register> args = statement.getArgs();
                for (Register reg : args) {
                    if (reg.is(Register.Type.TEMP_R)) {
                        allocator.putLineR(reg.getNum(), idx);
                    } else if (reg.is(Register.Type.TEMP_F)) {
                        allocator.putLineS(reg.getNum(), idx);
                    }
                }
            }
        }
        int delta = 0;
        iter = assemblerList.iterator();
        for (int idx = 0; iter.hasNext(); idx++) {
            Assembler assembler = iter.next();
            if (assembler instanceof Statement statement) {
                List<Register> args = statement.getArgs();
                for (int i = 0; i < args.size(); i++) {
                    Register reg = args.get(i);
                    if (reg.is(Register.Type.TEMP_R)) {
                        if (reg.isAddress()) {
                            args.set(i, allocator.getR(reg.getNum()).addr());
                        } else {
                            args.set(i, allocator.getR(reg.getNum()));
                        }
                    } else if (reg.is(Register.Type.TEMP_F)) {
                        args.set(i, allocator.getF(reg.getNum()));
                    }
                }
                allocator.tryFreeR(idx - delta);
                allocator.tryFreeS(idx - delta);
            }
        }
        freeRegistersRCount = Math.min(freeRegistersRCount, allocator.getMaxNoUseSR());
        freeRegistersFCount = Math.min(freeRegistersFCount, allocator.getMaxNoUseSF());
    }

    private static boolean bfsLoop(String name) {
        Set<String> loop = new HashSet<>();
        Queue<String> queue = new LinkedList<>();
        queue.add(name);
        while (!queue.isEmpty()) {
            String funcName = queue.poll();
            if (loop.contains(funcName)) {
                return true;
            }
            loop.add(funcName);
            queue.addAll(QuaOptimize.funcCallMap.get(funcName));
        }
        return false;
    }

    private static void buildMem2Reg() {
        Set<String> loop = new HashSet<>();
        for (String funcName : QuaOptimize.funcCallMap.keySet()) {
            if (bfsLoop(funcName)) {
                loop.add(funcName);
            }
        }
        List<Triple<String, String, Long>> fTimes =
                QuaHandler.funcFValUseTimes.stream().sorted(Comparator.comparingLong(o -> -o.third)).toList();
        List<Triple<String, String, Long>> rTimes =
                QuaHandler.funcRValUseTimes.stream().sorted(Comparator.comparingLong(o -> -o.third)).toList();
        int rIndex = S_SIZE, fIndex = FS_SIZE;
        for (int i = 0, len = Math.min(fTimes.size(), freeRegistersFCount); i < len; i++) {
            Triple<String, String, Long> triple = fTimes.get(i);
            if (!loop.contains(triple.first)) {
                mem2regF.put(triple.second, Registers.fs(--fIndex));
            }
        }
        for (int i = 0, len = Math.min(rTimes.size(), freeRegistersRCount); i < len; i++) {
            Triple<String, String, Long> triple = rTimes.get(i);
            if (!loop.contains(triple.first)) {
                mem2regR.put(triple.second, Registers.s(rIndex--));
            }
        }
    }

}
