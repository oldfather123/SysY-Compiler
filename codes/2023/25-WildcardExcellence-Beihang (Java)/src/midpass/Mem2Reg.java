package midpass;

import midend.Type;
import midend.Use;
import midend.value.BasicBlock;
import midend.value.Constant;
import midend.value.Function;
import midend.value.Value;
import midend.value.instrs.*;
import util.nodelist.Node;
import util.nodelist.NodeList;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.Stack;

public class Mem2Reg {
    private ArrayList<Function> functions;

    public Mem2Reg(ArrayList<Function> functions) {
        this.functions = functions;
        removeAlloc();
    }

    public void removeAlloc() {
        for (Function function : functions) {
            funcRemoveAlloc(function);
        }
    }

    public void funcRemoveAlloc(Function function) {
        for (var node : function.getBasicBlocks()) {
            BasicBlock block = node.get();
            blockRemoveAlloc(block);
        }
    }

    public void blockRemoveAlloc(BasicBlock basicBlock) {
        for (var node: basicBlock.getInstrList()) {
            Instr instr = node.get();
            if (instr instanceof Alloc && !(instr.getType().getElementType() instanceof Type.ArrayType)) {
                instrRemoveAlloc(node);
            }
        }
    }

    public void instrRemoveAlloc(Node<Instr> NInstr) {
        Instr instr = NInstr.get();
        ArrayList<BasicBlock> defBlocks = new ArrayList<>();
        ArrayList<BasicBlock> useBlocks = new ArrayList<>();
        HashSet<Instr> defInstrs = new HashSet<>();
        HashSet<Instr> useInstrs = new HashSet<>();
        NodeList<Use> usedOpList = instr.getUsedOpList();
        for (var node : usedOpList) {
            Instr userInstr = (Instr) node.get().getUser();
            if (userInstr instanceof Load) {
                //使用点
                useBlocks.add(userInstr.getParent());
                useInstrs.add(userInstr);
            } else if (userInstr instanceof Store) {
                //定义点
                defBlocks.add(userInstr.getParent());
                defInstrs.add(userInstr);
            }
        }
        if (useBlocks.isEmpty()) {
            //该变量从来没被使用过，所有对该变量的定义都是无用的，故删去
            for (var defInstr : defInstrs) {
                defInstr.remove();
            }
        } else if (defBlocks.size() == 1) {
            //只有一个基本块定义了该变量
            BasicBlock defBlock = defBlocks.get(0);
            Instr nowDef = null; //维护当前最新达到使用点的定义点
            for (var node : defBlock.getInstrList()) {
                Instr blockInstr = node.get();
                if (defInstrs.contains(blockInstr)) { //更新当前定义点
                    nowDef = blockInstr;
                } else if (useInstrs.contains(blockInstr)) {
                    if (nowDef == null) { //表明变量未初始化便使用
                        blockInstr.changeAllUse2UseOther(Constant.ConstantInteger.Constant0);
                    } else {
                        blockInstr.changeAllUse2UseOther(((Store) nowDef).getOpValue());
                    }
                }
            }
            for (Instr useInstr : useInstrs) {
                //对于在其他基本块中使用的指令也需要进行替换
                if (!useInstr.getParent().equals(defBlock)) {
                    assert nowDef != null;
                    useInstr.changeAllUse2UseOther(((Store) nowDef).getOpValue());
                }
            }
        } else {
            //教程教授的插入phi函数的算法
            LinkedList<BasicBlock> F = new LinkedList<>();
            LinkedList<BasicBlock> W = new LinkedList<>(defBlocks);

            while (!W.isEmpty()) {
                BasicBlock X = W.getFirst();
                W.remove(X);
                for (BasicBlock Y : X.getDfBB()) {
                    if (!F.contains(Y)) {
                        F.add(Y);
                        if (!defBlocks.contains(Y)) {
                            W.add(Y);
                        }
                    }
                }
            }

            for (BasicBlock block : F) {
                ArrayList<Value> values = new ArrayList<>(); //支配边界有n个前驱基本块就有2*n个操作数
                for (BasicBlock prevBB : block.getPrevBB()) {
                    values.add(new Instr());
                }
                Instr phi = new Phi(values, block);
                useInstrs.add(phi);
                defInstrs.add(phi);
            }

            Stack<Value> S = new Stack<>();
            BasicBlock entry = instr.getParent().getParent().getFirstBlock();
            Rename(S, entry, useInstrs, defInstrs);
        }
        NInstr.remove();
        instr.removeUse();
        if (!useBlocks.isEmpty()) {
            for (Instr defInstr : defInstrs) {
                if (!(defInstr instanceof Phi)) {
                    defInstr.remove();
                }
            }
            for (Instr useInstr : useInstrs) {
                if (!(useInstr instanceof Phi)) {
                    useInstr.remove();
                }
            }
        }
    }

    public void Rename(Stack<Value> S, BasicBlock now, HashSet<Instr> useInstrs, HashSet<Instr> defInstrs) {
        int cnt = 0;
        for (var node : now.getInstrList()) { //注意变量可能未定义
            Instr instr = node.get();
            if (!(instr instanceof Phi) && useInstrs.contains(instr)) {
                instr.changeAllUse2UseOther(getTopOfStack(S));
            }
            if (defInstrs.contains(instr)) {
                if (instr instanceof Store) {
                    S.push(((Store) instr).getOpValue());
                } else if (instr instanceof Phi) {
                    S.push(instr);
                }
                cnt++;
            }
        }
        ArrayList<BasicBlock> sucBB = now.getSucBB();
        for (BasicBlock block : sucBB) {
            for (var node : block.getInstrList()) {
                Instr instr = node.get();
                if (!(instr instanceof Phi)) {
                    break;
                }
                if (useInstrs.contains(instr)) {
                    instr.changeUseIn(getTopOfStack(S), block.getPrevBB().indexOf(now));
                }
            }
        }

        for (BasicBlock nextBlock : now.getIdomBB()) {
            Rename(S, nextBlock, useInstrs, defInstrs);
        }

        for (int i = 0; i < cnt; i++) {
            S.pop();
        }
    }

    public Value getTopOfStack(Stack<Value> S) {
        if (S.isEmpty()) {
            return Constant.ConstantInteger.Constant0;
        } else{
            return S.peek();
        }
    }
}
