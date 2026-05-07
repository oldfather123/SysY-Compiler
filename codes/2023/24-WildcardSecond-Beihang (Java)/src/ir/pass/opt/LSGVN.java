package ir.pass.opt;

import ir.instr.Call;
import ir.instr.GetElementPtrInst;
import ir.instr.Instr;
import ir.instr.Load;
import ir.instr.Store;
import ir.value.Arg;
import ir.value.BasicBlock;
import ir.value.Module;
import ir.value.user.Function;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.LinkedHashMap;

public class LSGVN {
    private static boolean needRepeat;

    public static boolean run(Module module) {
        needRepeat = false;
        module.functions.values().forEach(function ->
        {
            if (!function.blocks.isEmpty()) {
                runForFunc(function);
            }
        });
        return needRepeat;
    }

    private static boolean builtInMayChangeArray(Function function) {
        return !(function.name.equals("putarray") || function.name.equals("putfarray"));
    }

    private static void runForFunc(Function func) {
        for (BasicBlock block : func.blocks) {
            LinkedHashMap<Store, Integer> visitedStore = new LinkedHashMap<>();
            LinkedHashMap<Load, Integer> visitedLoad = new LinkedHashMap<>();
            LinkedHashMap<Call, Integer> visitedCall = new LinkedHashMap<>();
            ArrayList<Store> storeOrder = new ArrayList<Store>();
            ArrayList<Load> loadOrder = new ArrayList<Load>();
            ArrayList<Call> callOrder = new ArrayList<Call>();
            for (int i = 0; i < block.getInsts().size(); i++) {
                Instr instr = (Instr) block.getInsts().get(i);
                if (instr instanceof Load) {
                    if (instr.getOP(0) instanceof GetElementPtrInst) {
                        int pos = -1;
                        int order = -1;
                        for (int j = loadOrder.size() - 1; j >= 0; j--) {
                            var past = loadOrder.get(j);
                            if (past.getOP(0).equals(instr.getOP(0))) {
                                pos = j;
                                break;
                            }
                        }
                        if (pos != -1) {
                            order = visitedLoad.get(loadOrder.get(pos));
                            boolean canreplace = true;
                            for (int j = storeOrder.size() - 1;
                                 j >= 0 && visitedStore.get(storeOrder.get(j)) > order; j--) {
                                var past = storeOrder.get(j);
                                if (!(storeOrder.get(j).getOP(0) instanceof GetElementPtrInst)) {
                                    continue;
                                }
                                if (ArrayEliminate.mayConflict((GetElementPtrInst) past.getOP(0),
                                        (GetElementPtrInst) instr.getOP(0))) {
                                    canreplace = false;
                                    break;
                                }
                            }
                            if (!canreplace) {
                                visitedLoad.put((Load) instr, i);
                                loadOrder.add((Load) instr);
                                continue;
                            }
                            for (int j = callOrder.size() - 1; j >= 0 && visitedCall.get(callOrder.get(j)) > order;
                                 j--) {
                                var past = callOrder.get(j);
                                if (past.getFunction().blocks.isEmpty()) {// TODO: 2023/8/17
                                    if (builtInMayChangeArray(past.getFunction())) {
                                        for (var ope : past.getOperands()) {
                                            if (ope instanceof Arg || ope instanceof GetElementPtrInst) {
                                                if (ArrayEliminate.ptrConflit(
                                                        (GetElementPtrInst) instr.getOP(0), ope)) {
                                                    canreplace = false;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    canreplace = false;
                                }

                                if (!canreplace) {
                                    break;
                                }
                            }
                            if (canreplace) {
                                needRepeat = true;
                                instr.replaceAllUsers(loadOrder.get(pos));
                            } else {
                                loadOrder.add((Load) instr);
                                visitedLoad.put((Load) instr, i);
                            }
                        } else {
                            visitedLoad.put((Load) instr, i);
                            loadOrder.add((Load) instr);
                        }
                    } else if (instr.getOP(0) instanceof Arg) {
                        int pos = -1;
                        int order = -1;
                        for (int j = loadOrder.size() - 1; j >= 0; j--) {
                            var past = loadOrder.get(j);
                            if (past.getOP(0).equals(instr.getOP(0))) {
                                pos = j;
                                break;
                            }
                        }
                        if (pos != -1) {
                            order = visitedLoad.get(loadOrder.get(pos));
                            boolean canreplace = true;
                            for (int j = storeOrder.size() - 1;
                                 j >= 0 && visitedStore.get(storeOrder.get(j)) > order; j--) {
                                var past = storeOrder.get(j);
                                if (!(storeOrder.get(j).getOP(0) instanceof Arg)) {
                                    continue;
                                }
                                if (past.getOP(0).equals(instr.getOP(0))) {
                                    canreplace = false;
                                    break;
                                }
                            }
                            if (!canreplace) {
                                visitedLoad.put((Load) instr, i);
                                loadOrder.add((Load) instr);
                                continue;
                            }
                            for (int j = callOrder.size() - 1; j >= 0 && visitedCall.get(callOrder.get(j)) > order;
                                 j--) {
                                var past = callOrder.get(j);
                                if (past.getOperands().isEmpty()) {
                                    if (builtInMayChangeArray(past.getFunction())) {
                                        for (var ope : past.getOperands()) {
                                            if (ope instanceof GetElementPtrInst) {
                                                if (((GetElementPtrInst) ope).getOP(0).equals(instr.getOP(0))) {
                                                    canreplace = false;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    canreplace = false;
                                }

                                if (!canreplace) {
                                    break;
                                }
                            }
                            if (canreplace) {
                                needRepeat = true;
                                instr.replaceAllUsers(loadOrder.get(pos));
                            } else {
                                loadOrder.add((Load) instr);
                                visitedLoad.put((Load) instr, i);
                            }
                        } else {
                            visitedLoad.put((Load) instr, i);
                            loadOrder.add((Load) instr);
                        }
                    }

                } else if (instr instanceof Store) {
                    if (instr.getOP(0) instanceof GetElementPtrInst) {
                        int pos = -1;
                        int order = -1;
                        for (int j = storeOrder.size() - 1; j >= 0; j--) {
                            var past = storeOrder.get(j);
                            if (past.getOP(0).equals(instr.getOP(0))) {
                                if (past.getOP(1).equals(instr.getOP(1))) {
                                    pos = j;
                                    break;
                                }
                            }
                        }
                        if (pos == -1) {
                            storeOrder.add((Store) instr);
                            visitedStore.put((Store) instr, i);
                            continue;
                        }
                        order = visitedStore.get(storeOrder.get(pos));
                        var last = storeOrder.get(pos);
                        boolean canreplace = true;
                        for (int j = storeOrder.size() - 1;
                             j >= 0 && visitedStore.get(storeOrder.get(j)) > order; j--) {
                            var past = storeOrder.get(j);
                            if (!(storeOrder.get(j).getOP(0) instanceof GetElementPtrInst)) {
                                continue;
                            }
                            if (ArrayEliminate.mayConflict((GetElementPtrInst) past.getOP(0),
                                    (GetElementPtrInst) instr.getOP(0))) {
                                canreplace = false;
                                break;
                            }
                        }
                        if (!canreplace) {
                            visitedStore.put((Store) instr, i);
                            storeOrder.add((Store) instr);
                            continue;
                        }
                        for (int j = callOrder.size() - 1; j >= 0 && visitedCall.get(callOrder.get(j)) > order;
                             j--) {
                            var past = callOrder.get(j);
                            if (!past.getFunction().blocks.isEmpty()) {
                                canreplace = false;
                            } else {
                                if (builtInMayChangeArray(past.getFunction())) {
                                    for (var ope : past.getOperands()) {
                                        if (ope instanceof Arg ||
                                                ope instanceof GetElementPtrInst) {
                                            if (ArrayEliminate.ptrConflit(
                                                    (GetElementPtrInst) instr.getOP(0), ope)) {
                                                canreplace = false;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }

                            if (!canreplace) {
                                break;
                            }
                        }
                        if (canreplace) {
                            needRepeat = true;
                            block.getInsts().remove(instr);
                            i--;
                        } else {
                            storeOrder.add((Store) instr);
                            visitedStore.put((Store) instr, i);
                        }
                    } else if (instr.getOP(0) instanceof Arg) {
                        int pos = -1;
                        int order = -1;
                        for (int j = storeOrder.size() - 1; j >= 0; j--) {
                            var past = storeOrder.get(j);
                            if (past.getOP(0).equals(instr.getOP(0))) {
                                if (past.getOP(1).equals(instr.getOP(1))) {
                                    pos = j;
                                    break;
                                }
                            }
                        }
                        if (pos == -1) {
                            storeOrder.add((Store) instr);
                            visitedStore.put((Store) instr, i);
                            continue;
                        }
                        order = visitedStore.get(storeOrder.get(pos));
                        var last = storeOrder.get(pos);
                        boolean canreplace = true;
                        for (int j = storeOrder.size() - 1;
                             j >= 0 && visitedStore.get(storeOrder.get(j)) > order; j--) {
                            var past = storeOrder.get(j);
                            if (!(storeOrder.get(j).getOP(0) instanceof Arg)) {
                                continue;
                            }
                            if (past.getOP(0).equals(instr.getOP(0))) {
                                canreplace = false;
                                break;
                            }
                        }
                        if (!canreplace) {
                            visitedStore.put((Store) instr, i);
                            storeOrder.add((Store) instr);
                            continue;
                        }
                        for (int j = callOrder.size() - 1; j >= 0 && visitedCall.get(callOrder.get(j)) > order;
                             j--) {
                            var past = callOrder.get(j);
                            if (past.getOperands().isEmpty()) {
                                if (builtInMayChangeArray(past.getFunction())) {
                                    for (var ope : past.getOperands()) {
                                        if (ope instanceof GetElementPtrInst) {
                                            if (((GetElementPtrInst) ope).getOP(0).equals(instr.getOP(0))) {
                                                canreplace = false;
                                                break;
                                            }
                                        }
                                    }
                                }
                            } else {
                                canreplace = false;
                            }
                            if (!canreplace) {
                                break;
                            }
                        }
                        if (canreplace) {
                            needRepeat = true;
                            block.getInsts().remove(instr);
                            i--;
                        } else {
                            storeOrder.add((Store) instr);
                            visitedStore.put((Store) instr, i);
                        }
                    }
                } else if (instr instanceof Call) {
                    visitedCall.put((Call) instr, i);
                    callOrder.add((Call) instr);
                }
            }
        }
    }
}
