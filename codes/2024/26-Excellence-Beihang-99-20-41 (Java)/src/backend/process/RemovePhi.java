package backend.process;

import backend.component.ObjBlock;
import backend.component.ObjFunction;
import backend.component.ObjInstr;
import backend.component.ObjModule;
import backend.instruction.ObjBr;
import backend.instruction.ObjMove;
import backend.instruction.ObjPhi;
import backend.instruction.ObjRet;
import backend.operand.ObjFVirReg;
import backend.operand.ObjOperand;
import backend.operand.ObjVirReg;
import utils.Pair;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;

public class RemovePhi {
    public void run(ObjModule objModule) {
        for (ObjFunction function : objModule.getFunctions()) {
//            ObjFVirReg.fVirRegIndex = function.getOperands().size();
//            ObjVirReg.virRegIndex = function.getOperands().size();
//            ObjBlock.blockIndex = function.getBlocks().size();
            ArrayList<Pair<ObjInstr, ObjBlock>> toInsert = new ArrayList<>();//准备将move插入对应的块中
            ArrayList<Pair<ObjBlock, ObjBlock>> toInsert2 = new ArrayList<>();//准备在基本块之间插入
            for (ObjBlock block : function.getBlocks()) {
                /// 将 HashMap.Key 中的 block 替换为 HashMap.Value，后者是新建的基本块
                /// HashMap.Value 的最后一条指令，是跳转到 HashMap.Key.Second 的跳转指令
                /// 这是为了应对 block 中诸如下面的情况：
                ///     phi [%1, b1], [...]
                ///     phi [%2, b1], [...]
                HashMap<ObjBlock, ObjBlock> toReplace = new HashMap<>();
                for (int i = 0; i < block.getInstrs().size(); i++) {
                    ObjInstr instr = block.getInstrs().get(i);
                    if (instr.getType().equals("phi")) {
                        //遍历phi指令的操作数：operand:blocknum
                        ObjPhi phi = (ObjPhi) instr;
                        for (Pair<ObjOperand, ObjBlock> op : phi.getOperands()) {
                            if (op.getSecond().getNextBlocks().size() == 1) {
                                //只有一个去向，直接在结尾处赋值即可
                                //基本块都以终止指令结束
                                ObjMove move = new ObjMove(op.getFirst(), phi.getDst());
                                toInsert.add(new Pair<>(move, op.getSecond()));
                            } else {
                                //b0:
                                //  br b1 b2
                                //b1:
                                //  phi
                                //插一个基本块，先跳转到此处，move之后再跳转到phi处
                                if (!toReplace.containsKey(op.getSecond())) {
                                    // 新建一个基本块
                                    ObjBlock tmpBlock = new ObjBlock(function);
                                    // 调整 phi 来源块的图上关系
                                    op.getSecond().removeNextBlock(block);
                                    op.getSecond().addNextBlock(tmpBlock);
                                    // 新插入的基本块的图上关系
                                    tmpBlock.addPreBlock(op.getSecond());
                                    tmpBlock.addNextBlock(block);
                                    // phi 所处块的图上关系
                                    block.removePreBlock(op.getSecond());
                                    block.addPreBlock(tmpBlock);
                                    // 变更 phi 来源块的跳转指令
                                    LinkedList<ObjInstr> instrs = op.getSecond().getInstrs();
                                    ObjBr br = (ObjBr) instrs.getLast();
                                    // instrs.removeLast();
                                    if (br.getI1() == null) {
                                        if (br.getThenBlock().equals(block)) {
                                            instrs.set(instrs.size() - 1, new ObjBr(tmpBlock));
                                        }
                                    } else {
                                        if (br.getThenBlock().equals(block)) {
                                            instrs.set(instrs.size() - 1, new ObjBr(br.getI1(), tmpBlock, br.getElseBlock()));
                                        } else {
                                            instrs.set(instrs.size() - 1, new ObjBr(br.getI1(), br.getThenBlock(), tmpBlock));
                                        }
                                    }
                                    toInsert2.add(new Pair<>(op.getSecond(), tmpBlock));
                                    toReplace.put(op.getSecond(), tmpBlock);
                                }
                                ObjBlock tmpBlock = toReplace.get(op.getSecond());
                                tmpBlock.addInstr(new ObjMove(op.getFirst(), phi.getDst()));
                                // 不再这里加跳转，防止多次插入
                                // tmpBlock.addInstr(new ObjBr(block));
                            }
                        }
                    }
                }
                // 插入跳转指令
                for (ObjBlock tmpBlock : toReplace.values()) {
                    tmpBlock.addInstr(new ObjBr(block));
                }
            }
            //插入指令与代码块
            for (Pair<ObjInstr, ObjBlock> pair : toInsert) {
                ObjInstr tmpInstr = pair.getSecond().getInstrs().removeLast();
                pair.getSecond().addInstr(pair.getFirst());
                pair.getSecond().addInstr(tmpInstr);
            }
            for (Pair<ObjBlock, ObjBlock> pair : toInsert2) {
                LinkedList<ObjBlock> blocks = function.getBlocks();
                blocks.add(blocks.indexOf(pair.getFirst()) + 1, pair.getSecond());
            }
            //删除phi指令
            for (ObjBlock block : function.getBlocks()) {
                LinkedList<ObjInstr> instrs = block.getInstrs();
                Iterator<ObjInstr> it = instrs.iterator();
                while (it.hasNext()) {
                    ObjInstr objInstr = it.next();
                    if (objInstr instanceof ObjPhi) {
                        it.remove();
                    }
                }
//                while (!instrs.isEmpty() && instrs.peek().getType().equals("phi")) {
//                    instrs.pop();
//                }
            }
        }
//        ObjVirReg.virRegIndex = 0;
//        ObjFVirReg.fVirRegIndex = 0;
//        ObjBlock.blockIndex = 0;

        for (ObjFunction function : objModule.getFunctions()) {
            boolean hasSet = false;
            for (ObjBlock block : function.getBlocks()) {
                for (ObjInstr instr : block.getInstrs()) {
                    if (instr instanceof ObjRet) {
                        function.setExits(block);
                        hasSet = true;
                        break;
                    }
                }
                if (hasSet) {
                    break;
                }
            }
            if (!hasSet) {
                function.setExits(function.getBlocks().getLast());
            }
        }
    }
}
