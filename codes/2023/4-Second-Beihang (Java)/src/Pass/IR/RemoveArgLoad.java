package Pass.IR;

import IR.IRModule;
import IR.Value.Argument;
import IR.Value.Function;
import IR.Value.Instructions.AllocInst;
import IR.Value.Instructions.LoadInst;
import IR.Value.Instructions.StoreInst;
import IR.Value.User;
import Pass.Pass;

import java.util.ArrayList;

public class RemoveArgLoad implements Pass.IRPass {

    @Override
    public void run(IRModule module) {
        for(Function function : module.getFunctions()){
            for(Argument arg : function.getArgs()){
                AllocInst allocInst = null;
                for(User user : arg.getUserList()){
                    if(user instanceof StoreInst _store && _store.getPointer() instanceof AllocInst _alloc){
                        allocInst = _alloc;
                    }
                }
                if(allocInst == null){
                    continue;
                }

                int storeCnt = 0;
                StoreInst storeInst = null;
                ArrayList<LoadInst> loadInsts = new ArrayList<>();

                for(User user : allocInst.getUserList()){
                    if(user instanceof StoreInst _store){
                        storeInst = _store;
                        storeCnt++;
                    }
                    else if(user instanceof LoadInst _load){
                        loadInsts.add(_load);
                    }
                }
                if(storeCnt != 1){
                    continue;
                }
                if(loadInsts.size() < 1){
                    continue;
                }

                LoadInst oriLoadInst = loadInsts.get(0);
                for(int i = 1; i < loadInsts.size(); i++) {
                    LoadInst loadInst = loadInsts.get(i);
                    loadInst.replaceUsedWith(oriLoadInst);
                    loadInst.removeSelf();
                }
            }
        }
    }

    @Override
    public String getName() {
        return "RemoveArgAlloc";
    }
}
