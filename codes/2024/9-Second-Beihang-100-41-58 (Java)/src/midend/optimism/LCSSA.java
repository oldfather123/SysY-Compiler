package midend.optimism;

import midend.*;
import midend.Module;
import midend.analysis.CFGBuilder;
import midend.analysis.Loop;
import midend.analysis.LoopInfo;
import midend.instr.Instruction;
import midend.instr.PhiInstr;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Stack;

public class LCSSA {
    private Module module;

    public LCSSA(Module module) {
        this.module = module;
    }

    //1.记录循环中变量在外面使用的
    //2.获取退出块
    //3.找出口块
    //4.插入phi
    //5.重命名
    public void run() {
        new CFGBuilder(module).run();
        for (Function function : module.getFunctions()) {
            LoopInfo.loopAnalysis(function);
            LoopInfo.getExitingBlocks();
            runForLCSSA(function);
        }
    }

    public void runForLCSSA(Function function) {
        HashMap<BasicBlock, PhiInstr> phiInstrHashMap = new HashMap<>();
        ArrayList<Loop> loops = function.getLoops();
        for (Loop loop : loops) {
            //加入phi到退出块
            for (BasicBlock block : loop.getBasicBlocks()) {
                for (Instruction instruction : block.getInstrList()) {
                    phiInstrHashMap = new HashMap<>();
                    boolean useOut = false;
                    for (User user : instruction.getUserList()) {
                        if (!loop.getBasicBlocks().contains(((Instruction) user).getBasicBlock())) {
                            useOut = true;
                            break;
                        }
                    }
                    if (useOut) {
                        ArrayList<User> users = new ArrayList<>(instruction.getUserList());
                        for (BasicBlock exit : loop.getExitBlock()) {
                            if (!phiInstrHashMap.containsKey(exit) && instruction.getBasicBlock().getDomBlock().contains(exit)) {
                                PhiInstr phiInstr = new PhiInstr(instruction.getType(), exit.getPreBlock());
                                phiInstr.setBasicBlock(exit);
                                for (BasicBlock basicBlock : exit.getPreBlock()) {
                                    phiInstr.addOption(instruction, basicBlock);
                                }
                                exit.getInstrList().addFirst(phiInstr);
                                phiInstrHashMap.put(exit, phiInstr);
                            }
                        }
                        //加完phi之后，还需要重命名
                        for (User user : users) {
                            BasicBlock basicBlock = ((Instruction) user).getBasicBlock();
//                            if (basicBlock.getName().equals("block43")) {
//                                System.out.println("");
//                            }
                            if (user instanceof PhiInstr) {
                                int index = ((PhiInstr) user).getValueList().indexOf(instruction);
//                                if (index == -1) {
//                                    System.out.println("");
//                                }
                                basicBlock = ((PhiInstr) user).getPreBlockList().get(index);
                            }
                            if (!loop.getBasicBlocks().contains(basicBlock) && basicBlock != instruction.getBasicBlock()) {
                                PhiInstr phiInstr = null;
                                Stack<BasicBlock> stack = new Stack<>();
                                stack.push(basicBlock);
                                //HashMap<BasicBlock, PhiInstr> phiMap = new HashMap<>();
                                while (!stack.isEmpty()) {
                                    BasicBlock currentBlock = stack.peek();

                                    if (phiInstrHashMap.containsKey(currentBlock)) {
                                        stack.pop();
                                        continue;
                                    }

                                    if (!loop.getBasicBlocks().contains(currentBlock.getParentBlock())) {
                                        if (!phiInstrHashMap.containsKey(currentBlock.getParentBlock())) {
                                            stack.push(currentBlock.getParentBlock());
                                        } else {
                                            phiInstrHashMap.put(currentBlock, phiInstrHashMap.get(currentBlock.getParentBlock()));
                                            stack.pop();
                                        }
                                        //phiInstrHashMap.put(currentBlock, )
                                    } else {
                                        PhiInstr phiInstr1 = new PhiInstr(instruction.getType(), currentBlock.getPreBlock());
                                        for (BasicBlock block1 : currentBlock.getPreBlock()) {
                                            if (!phiInstrHashMap.containsKey(block1)) {
                                                //if (stack.contains(block1)) {
                                                stack.push(block1);
                                                //}
                                            } else {
                                                phiInstr1.addOption(phiInstrHashMap.get(block1), block1);
                                            }
                                        }
                                        int count = 0;
                                        for (int i = 0; i < phiInstr1.getValueList().size(); i++) {
                                            if (phiInstr1.getValueList().get(i) != null) {
                                                count++;
                                            }
                                        }
                                        if (count == currentBlock.getPreBlock().size()) {
                                            phiInstr1.setBasicBlock(currentBlock);
                                            currentBlock.getInstrList().addFirst(phiInstr1);
                                            phiInstrHashMap.put(currentBlock, phiInstr1);
                                            stack.pop();
                                        }
                                    }
                                }
                                phiInstr = phiInstrHashMap.get(basicBlock);
                                for (int count = 0; count < user.getValueList().size(); count++) {
                                    Value value = user.getValue(count);
                                    if (value.equals(instruction)) {
                                        user.setValue(count, phiInstr);
                                    }
                                }
                            }
                        }
                    }
                }
            }

//                for (Instruction instruction : phiInstrHashMap.keySet()) {
//                    PhiInstr phiInstr = phiInstrHashMap.get(instruction);
//                    for (User user : instruction.getUserList()) {
//                        if (user instanceof PhiInstr) {
//                            continue;
//                        }
//                        if (!(((Instruction) user).getBasicBlock().equals(instruction.getBasicBlock())
//                                || loop.getBasicBlocks().contains(((Instruction) user).getBasicBlock()))) {
//                            for (int count = 0; count < user.getValueList().size(); count++) {
//                                Value value = user.getValue(count);
//                                if (value.equals(instruction)) {
//                                    user.setValue(count, phiInstr);
//                                    break;
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//

        }

    }
}
