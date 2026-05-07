package Pass.IR;

import IR.IRBuildFactory;
import IR.IRModule;
import IR.Type.ArrayType;
import IR.Type.PointerType;
import IR.Value.*;
import IR.Value.Instructions.*;
import Pass.IR.Utils.DomAnalysis;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;

public class GlobalArrayInit implements Pass.IRPass {

    private final IRBuildFactory f = IRBuildFactory.getInstance();

    @Override
    public void run(IRModule module) {
        for(Function function : module.getFunctions()){
            DomAnalysis.run(function);
        }
        for(GlobalVar globalVar : module.getGlobalVars()){
            if(canInit(globalVar)) {
                initGlobalArray(globalVar);
            }
        }
    }

    private void initGlobalArray(GlobalVar globalVar){
        ArrayType type = (ArrayType) ((PointerType) globalVar.getType()).getEleType();
        if(!type.getEleType().isIntegerTy()){
            return ;
        }

        if(globalVar.getValues().size() == 0){
            int num = type.getTotalSize();
            for(int i = 0; i < num; i++){
                globalVar.getValues().add(f.buildNumber(0));
            }
        }

        ArrayList<StoreInst> storeInsts = getStoresForGlobalArray(globalVar);
        for(StoreInst storeInst : storeInsts){
            GepInst gepInst = (GepInst) storeInst.getPointer();

            ConstInteger idxValue = (ConstInteger) gepInst.getIndexs().get(1);
            int idx = idxValue.getValue();
            ConstInteger value = (ConstInteger) storeInst.getValue();

            globalVar.setInitValue(idx, value);
        }

        //  删除store
        for(StoreInst storeInst : storeInsts){
            storeInst.removeSelf();
        }

    }

    private boolean canInit(GlobalVar globalVar){
        if(!globalVar.isArray()){
            return false;
        }
        ArrayList<StoreInst> storeInsts = getStoresForGlobalArray(globalVar);

        /*
        * 1. 所有store在一个main中的一个基本块
        * 2. store的值是常数
        * 3. store的pointer是gep，且gep的偏移是常数
        * 4. storeBb支配所有load的基本块
        *
        * */

        if(storeInsts.size() == 0){
            return false;
        }
        BasicBlock storeBb = storeInsts.get(0).getParentbb();
        if(!storeBb.getParentFunc().getName().equals("@main")){
            return false;
        }
        for(StoreInst storeInst : storeInsts){
            BasicBlock nowStoreBb = storeInst.getParentbb();
            if(!nowStoreBb.equals(storeBb)){
                return false;
            }
            if(!(storeInst.getValue() instanceof Const)) {
                return false;
            }
            if(!(storeInst.getPointer() instanceof GepInst gepInst)){
                return false;
            }
            //  目前只处理一维数组
            if(gepInst.getIndexs().size() != 2){
                return false;
            }
            for(Value idxValue : gepInst.getIndexs()){
                if(!(idxValue instanceof Const)){
                    return false;
                }
            }
        }

        ArrayList<LoadInst> loadInsts = getLoadsForGlobalArray(globalVar);
        for(LoadInst loadInst : loadInsts){
            BasicBlock loadBb = loadInst.getParentbb();
            boolean isDominatedByStore = false;
            while (loadBb != null){
                if(loadBb.equals(storeBb)){
                    isDominatedByStore = true;
                    break;
                }
                loadBb = loadBb.getIdominator();
            }
            if(!isDominatedByStore){
                return false;
            }
        }

        return true;
    }

    private ArrayList<StoreInst> getStoresForGlobalArray(GlobalVar globalVar){
        ArrayList<GepInst> gepInsts = new ArrayList<>();
        ArrayList<StoreInst> storeInsts = new ArrayList<>();
        for(User user : globalVar.getUserList()){
            Instruction userInst = (Instruction) user;
            if(userInst instanceof GepInst gepInst){
                gepInsts.add(gepInst);
            }
        }
        for(GepInst gepInst : gepInsts){
            for(User user : gepInst.getUserList()){
                Instruction userInst = (Instruction) user;
                if(userInst instanceof StoreInst storeInst){
                    storeInsts.add(storeInst);
                }
            }
        }
        return storeInsts;
    }

    private ArrayList<LoadInst> getLoadsForGlobalArray(GlobalVar globalVar){
        ArrayList<GepInst> gepInsts = new ArrayList<>();
        ArrayList<LoadInst> loadInsts = new ArrayList<>();
        for(User user : globalVar.getUserList()){
            Instruction userInst = (Instruction) user;
            if(userInst instanceof GepInst gepInst){
                gepInsts.add(gepInst);
            }
        }
        for(GepInst gepInst : gepInsts){
            for(User user : gepInst.getUserList()){
                Instruction userInst = (Instruction) user;
                if(userInst instanceof LoadInst loadInst){
                    loadInsts.add(loadInst);
                }
            }
        }
        return loadInsts;
    }

    @Override
    public String getName() {
        return null;
    }
}
