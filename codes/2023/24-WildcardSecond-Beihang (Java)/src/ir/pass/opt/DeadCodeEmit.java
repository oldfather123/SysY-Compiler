package ir.pass.opt;
import ir.Value;
import ir.type.FloatType;
import ir.type.IntType;
import ir.type.VoidType;
import ir.value.Module;
import ir.value.*;
import ir.instr.*;
import ir.value.user.Function;
import ir.pass.analyze.*;
import ir.pass.analyze.RemoveUselessUser;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.LinkedList;

/*
    store call br
*/
public class DeadCodeEmit implements Pass{
    LinkedHashSet<Function> usefulFunctionClosure = new LinkedHashSet<>();
    LinkedHashSet<Function>  uselessFunction = new LinkedHashSet<>();
    LinkedHashSet<Value> usefulInstrClosure = new LinkedHashSet<>();
    LinkedList<User> queue = new LinkedList<>();

    public void runWeak(Module module){
        SideEffectAnalyzer.calSideEffect(module,true);
        deadCodeEmitInstrLevel(module,false);
//        deadCodeEmitLocalLevel(module);
        removeRet(module);
        deadCodeEmitFunctionLevel(module);
        deadCodeEmitParamLevel(module);

        for(var function:module.functions.values()){
            if(function.blocks.size()==0) continue;
            System.out.println(function.getFullName()+":");
            function.printCallees();
        }
    }
    @Override
    public void run(Module module){
//        if(module != null){
//            module = null;
//        }
//        module.functions = null;
        SideEffectAnalyzer.calSideEffect(module,false);
        deadCodeEmitInstrLevel(module,false);
        removeRet(module);
        deadCodeEmitFunctionLevel(module);
        deadCodeEmitParamLevel(module);
    }
    public void runStrong(Module module){
        //我真测了，大错特错
        SideEffectAnalyzer.calSideEffect(module,true);
        deadCodeEmitInstrLevel(module,false);
        removeRet(module);
        deadCodeEmitInstrLevel(module,false);
        deadCodeEmitFunctionLevel(module);
        deadCodeEmitVariableLevel(module);
        deadCodeEmitInstrLevel(module,false);
        deadCodeEmitParamLevel(module);
    }

    public void removeRet(Module module){
        for(var function:module.functions.values()){
            if(function.getFullName().equals("@main")) continue;
            if(function.blocks.size()==0) continue;
            if(function.getRetType() instanceof VoidType) continue;
            if(!retCanRemove(function)) continue;
            for(var BB:function.blocks){
                for(var instr:BB.getInsts()){
                    if(instr instanceof Ret){
                        Ret ret = (Ret)instr;
                        if(function.getRetType() instanceof IntType){
                            ret.replaceOp(ret.getOP(0),new ConstNumber(0));
                        }
                        if(function.getRetType() instanceof FloatType){
                            ret.replaceOp(ret.getOP(0),new ConstNumber(0.0));
                        }
                    }
                }
            }
        }
    }


    public boolean retCanRemove(Function function){
        for(var caller:function.getCallers()){
            for(var BB:caller.blocks){
                for(var instr:BB.getInsts()){
                    if(instr instanceof Call && ((Call)instr).getFunction().equals(function)){
                        for(var user:instr.getUsers()){
                            if (!(user instanceof Ret && ((Instr)user).getParent().getFatherFunction().equals(function))){
                                return false;
                            }
                        }
                    }
                }
            }
        }
        return true;
    }
    public void deadCodeEmitParamLevel(Module module){
        //TODO DeadParamEmit 2023/7/26
    }
    public void deadCodeEmitLocalLevel(Module module){
        LinkedHashSet<Value> toDelete = new LinkedHashSet<>();
        for(var function:module.functions.values()){
            for(var BB:function.blocks){
                for(var instr:BB.getInsts()){
                    if(instr instanceof Alloca && (((Alloca)instr).getAllocType() instanceof IntType || ((Alloca)instr).getAllocType() instanceof FloatType )){
                        boolean flag = true;
                        for(var user:instr.getUsers()){
                            if(user instanceof Load) {
                                flag = false;
                                break;
                            }
                        }
                        if(flag){
                            toDelete.add(instr);
                            toDelete.addAll(instr.getUsers());
                        }
                    }
                }
            }
        }
        for(var function: module.functions.values()){
            for(var basicBlock:function.blocks){
                Iterator<Value> it = basicBlock.getInsts().iterator();
                while (it.hasNext()) {
                    Instr instr = (Instr) it.next();
                    if (toDelete.contains(instr))
                    {
                        instr.delete();
                        it.remove();
                    }
                }
            }
        }
    }
    public void deadCodeEmitVariableLevel(Module module){
        LinkedHashSet<Value> toDelete = new LinkedHashSet<>();
        for(int i=0;i<module.init.getInsts().size();++i){
            User instr = (User) module.init.getInsts().get(i);
            if(instr.checkUsers()) {
                toDelete.addAll(instr.users);
                for(var User:instr.users){
                    if(User instanceof GetElementPtrInst) toDelete.addAll(User.getUsers());
                }
                instr.delete();
            }
        }
        for(var function: module.functions.values()){
            for(var basicBlock:function.blocks){
                Iterator<Value> it = basicBlock.getInsts().iterator();
                while (it.hasNext()) {
                    Instr instr = (Instr) it.next();
                    if (toDelete.contains(instr))
                    {
                        instr.delete();
                        it.remove();
                    }
                }
            }
        }
        module.init.getInsts().removeIf(Value::checkUsers);
    }
    public void deadCodeEmitInstrLevel(Module module,boolean opt){
        usefulInstrClosure.clear();
        queue.clear();
        for(var function: module.functions.values()){
            for(var basicBlock:function.blocks){
                getUseFulInstr(basicBlock,opt);
            }
        }
        for(var function: module.functions.values()){
            for(var basicBlock:function.blocks){
                Iterator<Value> it = basicBlock.getInsts().iterator();
                while (it.hasNext()) {
                    Instr instr = (Instr) it.next();
                    if (!usefulInstrClosure.contains(instr))
                    {
                        instr.delete();
                        it.remove();
                    }
                }
            }
        }
    }

    public boolean isUseFulInstr(Value instr, boolean opt){
        //store可以加强修改局部变量和改变数组指针
        //TODO:加强捏吗,卷错方向了.....
        if (!opt) {
            return  instr instanceof Store ||
                    instr instanceof Br ||
                    instr instanceof Ret ||
                    (instr instanceof Call && ((Call) instr).getFunction().hasSideEffect);
        } else {
//            return  (instr instanceof Store && (((Store) instr).checkGlobalStore()||((Store) instr).checkArgPointerStore())) ||
//                    instr instanceof Br    ||
//                    instr instanceof Ret   ||
//                    (instr instanceof Call && ((Call) instr).getFunction().hasSideEffect);
            System.exit(90);
            return true;
        }
    }
    public void getUseFulInstrClosure(Value instr){
        if(usefulInstrClosure.contains(instr)) return;
        usefulInstrClosure.add(instr);
        for(var operand:((Instr) instr).getOperands()){
            if(operand instanceof Instr) queue.add((Instr)operand);
        }
    }
    public void getUseFulInstr(BasicBlock basicBlock,boolean opt){
        for(var instr:basicBlock.getInsts()){
            if(isUseFulInstr(instr,opt)) {
                queue.addLast((User)instr);
            }

        }
        while(!queue.isEmpty()){
            getUseFulInstrClosure(queue.pop());
        }
    }

    public void deadCodeEmitFunctionLevel(Module module){
        var main = module.getFunction("main");
        getUsefulFunctionClosure(main);
        for(var key: module.functions.keySet()){
            if(!usefulFunctionClosure.contains(module.functions.get(key))&&module.functions.get(key).blocks.size()!=0){
                uselessFunction.add(module.functions.get(key));
                System.out.println("shuai"+key);
            }
        }
        for(var function: uselessFunction){
            module.functions.remove(function.name);
        }
    }

    public void getUsefulFunctionClosure(Function function) {
        if(usefulFunctionClosure.contains(function)) return;
        usefulFunctionClosure.add(function);
        for(var callee:function.getCallees()){
            getUsefulFunctionClosure(callee);
        }
    }


}

