#include "callGraphAnalysis.h"
#include "utils.h"
#include <chrono>

// bool recursiveCheck(FunctionPtr func, const std::unordered_map<FunctionPtr, int>& callee, callGraphAnalysisResult& cg) {
//     if(callee.find(func) != callee.end()) {
//         return true;
//     }
//     for(const auto& [calleeFunc, count] : callee) {
//         if(recursiveCheck(func, cg.callee(calleeFunc), cg)) {
//             return true;
//         }
//     }
//     return false;
// }

callGraphAnalysisResult runCallGraphAnalysis(Module& mod) {
    callGraphAnalysisResult result;
    cout << YELLOW << "Running Call Graph Analysis..." << RESET << endl;
    auto start = chrono::high_resolution_clock::now();
    for(auto& func : mod.getGlobalFunctions()) {
        result.callGraph()[func] = {};
        if(func->isLib) {
            continue;
        }
        for(auto& bb : func->getBasicBlocks()) {
            for(auto& inst : bb->instructions()) {
                if(inst->getInsID() == InsID::Call) {
                    CallInstruction* callInst = dynamic_cast<CallInstruction*>(inst);
                    FunctionPtr calleeFunc = callInst->getFunc();
                    auto& cg = result.callGraph();
                    cg[func].callInstList.push_back(callInst);

                    // check if the callee is a library function
                    if(!calleeFunc->isLib) {
                        cg[func].allCalleeIsLib = false;
                    }

                    // add callee info to caller
                    if(cg[func].callee.find(calleeFunc) == cg[func].callee.end()) {
                        cg[func].callee[calleeFunc] = 1;
                    } else {
                        cg[func].callee[calleeFunc]++;
                    }

                    // add caller info to callee
                    if(cg[calleeFunc].caller.find(func) == cg[calleeFunc].caller.end()) {
                        cg[calleeFunc].caller[func] = 1;
                    } else {
                        cg[calleeFunc].caller[func]++;
                    }

                    // check if the function is recursive
                    //// Recursively check if the function is recursive
                    if(func == calleeFunc) {
                        cg[func].isRecursive = true;
                    }
                }
            }
        }
    }
    // for(auto& [func, info] : result.callGraph()) {
    //     info.isRecursive = recursiveCheck(func, info.callee, result);
    // }
    auto end = chrono::high_resolution_clock::now();
    cout << GREEN << "Call Graph Analysis finished in " << RESET
         << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms" << endl;
    return result;
}

callGraphAnalysisResult runCallGraphAnalysis(FunctionPtr func, callGraphAnalysisResult& oldCG) {
    cout << YELLOW << "Running Call Graph Analysis for function " << func->getName() << "..." << RESET << endl;
    auto start = chrono::high_resolution_clock::now();
    oldCG.clear(func);
    auto& cg = oldCG.callGraph();
    for(auto& bb : func->getBasicBlocks()) {
        for(auto& inst : bb->instructions()) {
            if(inst->getInsID() == InsID::Call) {
                CallInstruction* callInst = dynamic_cast<CallInstruction*>(inst);
                FunctionPtr calleeFunc = callInst->getFunc();
                cg[func].callInstList.push_back(callInst);
                // check if the callee is a library function
                if(!calleeFunc->isLib) {
                    cg[func].allCalleeIsLib = false;
                }

                // add callee info to caller
                if(cg[func].callee.find(calleeFunc) == cg[func].callee.end()) {
                    cg[func].callee[calleeFunc] = 1;
                } else {
                    cg[func].callee[calleeFunc]++;
                }

                // // add caller info to callee
                // if(cg[calleeFunc].caller.find(func) == cg[calleeFunc].caller.end()) {
                //     cg[calleeFunc].caller[func] = 1;
                // } else {
                //     cg[calleeFunc].caller[func]++;
                // }

                // check if the function is recursive
                if(func == calleeFunc) {
                    cg[func].isRecursive = true;
                }
            }
        }
    }
    auto end = chrono::high_resolution_clock::now();
    cout << GREEN << "Call Graph Analysis for function " << func->getName() << " finished in " << RESET
         << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms" << endl;
    return oldCG;
}

void callGraphAnalysisResult::printCallGraph() const {
    for(const auto& [func, info] : mInfo) {
        if(func->isLib) {
            continue;
        }
        cout << "Function: " << YELLOW << func->getName() << RESET << endl;
        cout << "Callee: " << endl;
        for(const auto& [callee, count] : info.callee) {
            cout << "    " << callee->getName() << " : " << count << endl;
        }
        cout << "\nCaller: " << endl;
        for(const auto& [caller, count] : info.caller) {
            cout << "    " << caller->getName() << " : " << count << endl;
        }
        cout << "\nCall Instruction: " << endl;
        for(const auto& callInst : info.callInstList) {
            cout << "    ";
            callInst->dump();
        }
        cout << "\nIs Recursive: " << info.isRecursive << endl;
        cout << "All Callee is Lib: " << info.allCalleeIsLib << endl;
        cout << endl;
    }
}

void callGraphAnalysisResult::clear() {
    mInfo.clear();
}

void callGraphAnalysisResult::clear(FunctionPtr func) {
    mInfo.erase(func);
}

const std::unordered_map<FunctionPtr, int>& callGraphAnalysisResult::callee(FunctionPtr func) const {
    return mInfo.at(func).callee;
}

const std::unordered_map<FunctionPtr, int>& callGraphAnalysisResult::caller(FunctionPtr func) const {
    return mInfo.at(func).caller;
}

const std::vector<CallInstruction*>& callGraphAnalysisResult::callInstList(FunctionPtr func) const {
    return mInfo.at(func).callInstList;
}