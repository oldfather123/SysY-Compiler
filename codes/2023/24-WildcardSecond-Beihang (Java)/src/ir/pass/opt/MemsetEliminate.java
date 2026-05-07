package ir.pass.opt;

import ir.Value;
import ir.instr.Alloca;
import ir.instr.Bitcast;
import ir.instr.Call;
import ir.instr.GetElementPtrInst;
import ir.instr.Instr;
import ir.instr.Store;
import ir.type.ArrayType;
import ir.type.IntType;
import ir.type.Pointer;
import ir.type.Type;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.Module;
import ir.value.user.Function;

import java.awt.Point;
import java.util.ArrayList;
import java.util.LinkedList;

//目标是消除所有大小小于50的memset，为LSGCM创造更多空间
public class MemsetEliminate {
    public static void run(Module module) {
        module.functions.values().forEach(MemsetEliminate::runForFunction);
    }

    private static void runForFunction(Function function) {
        function.blocks.forEach(MemsetEliminate::runForBlock);
    }

    private static void updatePos(ArrayList<Integer> pos, Alloca alloca) {
        ArrayType type = (ArrayType) ((Pointer) alloca.type).pointTo;
        int i = pos.size() - 1;
        pos.set(i, pos.get(i) + 1);
        while (!type.isValidPos(pos)) {
            pos.set(i, 0);
            i--;
            pos.set(i, pos.get(i) + 1);
        }
    }

    private static void runForBlock(BasicBlock block) {
        for (int i = 0; i < block.getInsts().size() - 1; i++) {
            Instr instr = (Instr) block.getInsts().get(i);
            if (!(instr instanceof Call)) {
                continue;
            }
            if (!((Call) instr).getFunction().name.equals("memset")) {
                continue;
            }
            Alloca target = (Alloca) ((Bitcast) instr.getOP(0)).getOP(0);
            int size = (Integer) ((ConstNumber) instr.getOP(2)).value;
            if (size <= 200) {
                ArrayList<Value> instrs = new ArrayList<>();
                for (int j = 0; j < i; j++) {
                    instrs.add(block.getInsts().get(j));
                }
                Type tmp = (ArrayType) ((Pointer) target.type).pointTo;
                ArrayList<Integer> pos = new ArrayList<Integer>();
                while (tmp instanceof ArrayType) {
                    pos.add(0);
                    tmp = ((ArrayType) tmp).t;
                }
                pos.set(pos.size() - 1, -1);
                for (int j = 0; 4 * j < size; j++) {
                    updatePos(pos, target);
                    LinkedList<Value> uses = new LinkedList<>();
                    uses.add(target);
                    uses.add(new ConstNumber(0));
                    for (Integer po : pos) {
                        uses.add(new ConstNumber(po));
                    }
                    var ptr = new GetElementPtrInst(uses,
                        new Pointer(((ArrayType) ((Pointer) target.type).pointTo).innerType()),
                        "tmp", block);
                    instrs.add(ptr);
                    var store = new Store(ptr,
                        ((ArrayType) ((Pointer) target.type).pointTo).innerType() instanceof IntType ?
                            new ConstNumber(0) : new ConstNumber(0.0), block);
                    instrs.add(store);
                }
                int isave = instrs.size() - 1;
                for (int j = i + 1; j < block.getInsts().size(); j++) {
                    instrs.add(block.getInsts().get(j));
                }
                i = isave;
                block.setInsts(instrs);
            }
        }
    }
}
