package mid.Optimizer.Memory;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.MainFunction;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.GlobalDecl;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Module;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;

public class GlobalDeclLocalization {
    private final Module module;

    public GlobalDeclLocalization() {
        module = IRManager.getModule();
    }

    public void optimize() {
        for (GlobalDecl globalDecl : module.getGlobalDecls()) {
            if (globalDecl.isArray()) {
                //如果是全局数组，做不做初始化其实都没啥区别了
                continue;
            }
            Function usedFunction = null;
            boolean canLocalize = true;
            for (User user : globalDecl.getUserList()) {
                BasicBlock block = user.getBlock();
                if (block != null) {
                    Function func = block.getFunction();
                    if (usedFunction == null) {
                        usedFunction = func;
                    } else if (!usedFunction.equals(func)) {
                        canLocalize = false;
                        break;
                    }
                }
            }

            //说明仅被一个函数使用，且该函数最多被调用一次，则可以局部化
            // TODO: 其他函数的局部化？
            if (canLocalize && usedFunction instanceof MainFunction) {
                BasicBlock firstBlock = usedFunction.getEntranceBlock();
                //局部化在最开始就可以做，不需要考虑phi等
                Alloca alloca = new Alloca(!globalDecl.isFloat());
                Value init = globalDecl.getInit();
                Store initStore = new Store(init, alloca);
                firstBlock.addInstructionAt(0, initStore);
                firstBlock.addInstructionAt(0, alloca);
                globalDecl.beReplacedBy(alloca);
                module.removeGlobalDecl(globalDecl);
                globalDecl.destroy();
            }
        }
    }
}
