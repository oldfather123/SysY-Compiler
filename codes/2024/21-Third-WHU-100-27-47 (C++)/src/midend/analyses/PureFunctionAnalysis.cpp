#include "PureFunctionAnalysis.h"

using namespace ir;

void PureFunctionAnalysisContext::analyzePureness(FuncPtr func, Map<FuncPtr, bool> &analyzed, Map<FuncPtr, bool> &inStack) {
    if (analyzed[func]) {
        return;
    }
    inStack[func] = true;

    isPureFunction[func] = true;

    // 如果引用了非局部变量，则不是纯函数
    // 获取形参中的内存变量的基变量集合
    Set<Variable> paramBaseMemVars{};
    for (auto param : func->paramList()) {
        if (param->dataType().isPointer()) {
            auto paramBaseMemVar = Variable::mem(param->findAliasGroup());
            paramBaseMemVars.insert(paramBaseMemVar);
        }
    }
    // 检查对非局部变量的引用
    for (auto bb : func->bbSet()) {
        for (auto inst : bb->getInstSet()) {
            for (auto mayUseVar : inst->mayUseVars()) {
                if ((mayUseVar.isMem() && mayUseVar.getMemPtr()->isGlobal()) ||
                    (mayUseVar.isMem() && paramBaseMemVars.count(Variable::mem(mayUseVar.getMemPtr()->findAliasGroup())))) {
                    isPureFunction[func] = false;
                    analyzed[func] = true;
                    inStack[func] = false;
                    return;
                }
            }
        }
    }

    if (!inStack[func]) {
        // 如果调用了非纯函数，则不是纯函数
        for (auto callee : callerToCallees[func]) {
            if (!analyzed[callee]) {
                analyzePureness(func, analyzed, inStack);
            }
            if (!isPureFunction[callee]) {
                isPureFunction[func] = false;
                analyzed[func] = true;
                inStack[func] = false;
                return;
            }
        }
    }

    analyzed[func] = true;
    inStack[func] = false;
}

PureFunctionAnalysisContext::PureFunctionAnalysisContext(Ptr<Module> module) {
    for (auto func : module->funcSet()) {
        for (auto bb : func->bbSet()) {
            for (auto inst : bb->getInstSet()) {
                if (inst->is<CallInst>()) {
                    auto caller = func;
                    auto callee = inst->as<CallInst>()->function();
                    callerToCallees[caller].insert(callee);
                    calleeToCallers[callee].insert(caller);
                }
            }
        }
    }

    // 分析纯函数
    {
        Map<FuncPtr, bool> analyzed{};
        Map<FuncPtr, bool> inStack{};
        for (auto func : module->funcSet()) {
            if (func->isPrototype()) {
                isPureFunction[func] = false;
                analyzed[func] = true;
            }
        }
        for (auto func : module->funcSet()) {
            analyzePureness(func, analyzed, inStack);
        }
        for (auto [func, isPure] : isPureFunction) {
            func->isPure() = isPure;
        }
    }

    dbgout << "├── PureFunctionDetectionContext constructed for module." << std::endl;
    printDebug();
}
