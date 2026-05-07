package midend.optimism;

import config.Config;
import midend.*;
import midend.LLVMType.UndefinedType;
import midend.LLVMType.VoidType;
import midend.Module;
import midend.analysis.CFGBuilder;
import midend.instr.*;

import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedList;

public class FuncInline {
    private Module module;
    private boolean changed = true;
    private IRBuilder builder = new IRBuilder();

    public FuncInline(Module module) {
        this.module = module;
    }

    public void run() throws IOException {
        ArrayList<Function> inLineFunc = new ArrayList<>();
        //找到能内联的函数
        while(changed) {
            module.buildCallRelation();
            changed = false;
            for (Function function : module.getFunctions()) {
                if (function.isInLineable()) {
                    inLineFunc.add(function);
                }
            }
            for (Function function : inLineFunc) {
                inLineFunc(function);
            }
            inLineFunc.clear();
            module.outputLLVM(Config.llvmoptFile);
            new CFGBuilder(module).run();
        }
        module.buildCallRelation();
        module.removeUseLessFunc();
    }

    public void inLineFunc(Function function) {
        if(function.getBeCalledList().isEmpty()) {
            return;
        }
        changed = true;
        //找到对应的callInstr
        ArrayList<Instruction> callInstr = new ArrayList<>();
        for (Function function1 : function.getBeCalledList()) {
            if (function1.equals(function)) {
                continue;
            }
            for (BasicBlock block : function1.getBlockList()) {
                for (Instruction instruction : block.getInstrList()) {
                    if (instruction instanceof CallInstr) {
                        if (((CallInstr) instruction).getFunction().getName().equals(function.getName())) {
                            callInstr.add(instruction);
                        }
                    }
                }
            }
        }
        //处理callInstr进行替换
        for (Instruction instruction : callInstr) {
            dealCall((CallInstr) instruction);
        }
        //删除该函数调用表
        function.getBeCalledList().clear();
    }

    //假设函数中块关系为preBlock-InlineBlock-nextBlock
    //InlineBlock分为beforeFuncBlock（InlineBlock）-funcBlock-afterBlock
    //1.构建afterBlock
    //2.删除callInstr及后继指令
    //3.修正基本块的前驱后继关系（pre,nextb）
    //4.替换形参为实参
    //5.处理ret\call以及储存内联函数的块
    //6.ret只有一个可以直接将call替换为相应的Value跳转到nextBlock
    //7.多个ret可使用phi指令
    //8.加入内联函数块（即funcBlock）
    //9.函数中的call构建新的函数关系
    public void dealCall(CallInstr callInstr) {
        BasicBlock InlineBlock = callInstr.getBasicBlock();
        Function InlineFunc = callInstr.getFunction();
        Function nowFunc = InlineBlock.getParentFunc();

        //1.构建afterBlock inlineblock-funcblock-afterblock
        BasicBlock afterBlock = new BasicBlock(UndefinedType.undefined, nowFunc);
        LinkedList<BasicBlock> basicBlocks = nowFunc.getBlockList();
        int index = basicBlocks.indexOf(InlineBlock);
        basicBlocks.add(index + 1, afterBlock);

        //2.在afterBlock中加入指令
        int instrIndex = InlineBlock.getInstrList().indexOf(callInstr);
        LinkedList<Instruction> instructions = new LinkedList<>(InlineBlock.getInstrList());
        for (int count = instrIndex + 1; count < instructions.size(); count++) {
            Instruction instruction = instructions.get(count);
            afterBlock.addInstr(instruction);
            instruction.setBasicBlock(afterBlock);
            InlineBlock.getInstrList().remove(instruction);
        }
        //InlineBlock.getInstrList().remove(callInstr);
        callInstr.remove();
        nowFunc.getCalledList().remove(InlineFunc);
        InlineFunc.getBeCalledList().remove(nowFunc);

        //3.维持前驱后续
        for (BasicBlock block : InlineBlock.getNextBlock()) {
            afterBlock.setNextBlock(block);
            for (int count = 0; count < block.getPreBlock().size(); count++) {
                BasicBlock basicBlock = block.getPreBlock().get(count);
                if (basicBlock == InlineBlock) {
                    block.getPreBlock().set(count, afterBlock);
                }
            }
        }

        //4.在InlineBlock和afterBlock之间构造FuncBlock
        Function copyFunc = InlineFunc.clone();
        BasicBlock FuncBlock = copyFunc.getBlockList().getFirst();
        builder.buildBrInstr(FuncBlock, InlineBlock);
        InlineBlock.setNextBlock(FuncBlock);
        FuncBlock.setPreBlock(InlineBlock);

        //5.将形参替换为实参
        ArrayList<Param> formalParams = copyFunc.getParams();
        ArrayList<Value> actualParams = callInstr.getValues();
        for (int count = 0; count < formalParams.size(); count++) {
            Param formalParam = formalParams.get(count);
            Value actualParam = actualParams.get(count);
            if (actualParam.getType().isFloat() || actualParam.getType().isInteger()) {
                formalParam.replaceUse(actualParam);
            } else {
                ArrayList<Instruction> delete = new ArrayList<>();
                for (User user : formalParam.getUserList()) {
                    if (!(user instanceof Instruction)) {
                        continue;
                    }
                    Instruction instruction = (Instruction) user;
                    if (instruction.getInstrType() == InstrType.STORE) {
                        Value pointer = ((StoreInstr) instruction).getPointer();
                        delete.add((AllocaInstr) pointer);
                        delete.add(instruction);
                        for (User user1 : pointer.getUserList()) {
                            if (user1 instanceof LoadInstr) {
                                user1.replaceUse(actualParam);
                                delete.add((LoadInstr) user1);
                            }
                        }
                    }
                }
                for (Instruction instruction1 : delete) {
                    instruction1.remove();
                }
            }
        }
        //6.统计ret和call指令
        ArrayList<BasicBlock> funcBlock = new ArrayList<>();
        ArrayList<Instruction> ret = new ArrayList<>();
        ArrayList<Instruction> call = new ArrayList<>();
        for (BasicBlock block : copyFunc.getBlockList()) {
            funcBlock.add(block);
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof RetInstr) {
                    ret.add(instruction);
                    continue;
                }
                if (instruction instanceof CallInstr) {
                    call.add(instruction);
                    continue;
                }
            }
        }
        //7.处理ret指令（只有一个则直接跳到afterBlock）,多个需要利用phi指令
        if (copyFunc.getRetType().isInteger() || copyFunc.getRetType().isFloat()) {
            if (ret.size() == 1) {
                RetInstr retInstr = (RetInstr) ret.get(0);
                BasicBlock block = retInstr.getBasicBlock();
                callInstr.replaceUse(retInstr.getValue());
                retInstr.remove();
                builder.buildBrInstr(afterBlock, block);
                block.setNextBlock(afterBlock);
                afterBlock.setPreBlock(block);
            } else {
                //TODO:多个ret,使用phi指令
                PhiInstr phiInstr = new PhiInstr(InlineFunc.getRetType());
                phiInstr.setBasicBlock(afterBlock);
                LinkedList<Instruction> instructions1 = afterBlock.getInstrList();
                for (Instruction instruction : ret) {
                    if (((RetInstr) instruction).getType().isVoid()) {
                        continue;
                    }
                    BasicBlock nowBlock = instruction.getBasicBlock();
                    Value addValue = ((RetInstr) instruction).getValue();
                    if (Function.cloneMap.containsKey(((RetInstr) instruction).getValue())) {
                        addValue = Function.cloneMap.get(((RetInstr) instruction).getValue());
                    }
                    phiInstr.addOp(addValue, instruction.getBasicBlock());
                    instruction.remove();
                    builder.buildBrInstr(afterBlock, nowBlock);
                    nowBlock.setNextBlock(afterBlock);
                    afterBlock.setPreBlock(nowBlock);
                }
                callInstr.replaceUse(phiInstr);
                instructions1.addFirst(phiInstr);
            }
        } else if (copyFunc.getRetType().isVoid()) {
            for (Instruction instruction : ret) {
                BasicBlock block = instruction.getBasicBlock();
                instruction.remove();
                builder.buildBrInstr(afterBlock, block);
                block.setNextBlock(afterBlock);
                afterBlock.setPreBlock(block);
            }
        }
        //8.加入其他FuncBlock
        for (BasicBlock block : funcBlock) {
            block.setParentFunc(nowFunc);
            int index0 = basicBlocks.indexOf(afterBlock);
            basicBlocks.add(index0, block);
        }
        //9.保持内联之后新出现的call指令的函数关系
        for (Instruction instruction : call) {
            CallInstr callInstr1 = ((CallInstr) instruction);
            Function callFunc = callInstr1.getFunction();
            if (callFunc.isLib()) {
                continue;
            }
            callFunc.getBeCalledList().add(nowFunc);
            nowFunc.getCalledList().add(callFunc);
        }

        module.getFunctions().remove(copyFunc);
    }


}
