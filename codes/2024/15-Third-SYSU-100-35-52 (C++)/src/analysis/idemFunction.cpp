#include "idemFunction.h"

std::set<Function*> idemFunction(Module& mod) {
    std::set<Function*> result;
    std::set<Function*> nonIdem;
    bool done = false;
    while(!done) {
        done = true;
        for(auto func : mod.getGlobalFunctions()) {
            // 该函数是否是幂等函数
            bool isIdem = true;
            // 如果是函数声明, 保守认为非幂等
            if(func->isLib) {
                isIdem = false;
            }
            // 函数参数是指针, 非幂等
            for(auto arg : func->args()) {
                if(arg->getType()->isPtr()) {
                    isIdem = false;
                    break;
                }
            }
            // 遍历每条指令
            for(auto& bb : func->getBasicBlocks()) {
                for(auto inst : bb->instructions()) {
                    // 对全局变量修改, 该函数非幂等
                    if(auto storeInst = inst->as<StoreInstruction>()) {
                        auto ptrVal = storeInst->getPtr();
                        while(auto gepInst = ptrVal->as<GetElementPtrInstruction>()) {
                            ptrVal = gepInst->getBase();
                        }
                        if(std::find(mod.getGlobalVariables().begin(), mod.getGlobalVariables().end(), ptrVal) !=
                           mod.getGlobalVariables().end()) {
                            isIdem = false;
                            break;
                        }
                    }
                    // 调用非幂等函数, 该函数非幂等
                    else if(auto callInst = inst->as<CallInstruction>()) {
                        auto calledFunc = callInst->getFunc();
                        if(nonIdem.find(calledFunc) != nonIdem.end()) {
                            isIdem = false;
                            break;
                        }
                    }
                }
                if(!isIdem) {
                    break;
                }
            }
            if(!isIdem && nonIdem.find(func) == result.end()) {
                nonIdem.insert(func);
                done = false;
            }
        }
    }
    for(auto func : mod.getGlobalFunctions()) {
        if(nonIdem.find(func) == nonIdem.end()) {
            result.insert(func);
        }
    }
    return result;
}