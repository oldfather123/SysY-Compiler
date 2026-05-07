#pragma once
#include <Module.h>

struct callInfo final {
    std::unordered_map<FunctionPtr,int> callee = {};
    std::unordered_map<FunctionPtr,int> caller = {};
    std::vector<CallInstruction*> callInstList = {};
    bool isRecursive = false;
    bool allCalleeIsLib = true;
};

class callGraphAnalysisResult final {
    std::unordered_map<FunctionPtr, callInfo> mInfo;

public:
    std::unordered_map<FunctionPtr, callInfo>& callGraph() {
        return mInfo;
    }
    
    const std::unordered_map<FunctionPtr,int>& callee(FunctionPtr func) const;
    const std::unordered_map<FunctionPtr,int>& caller(FunctionPtr func) const;
    const std::vector<CallInstruction*>& callInstList(FunctionPtr func) const;

    void clear();
    void clear(FunctionPtr func);

    void printCallGraph() const;
};

callGraphAnalysisResult runCallGraphAnalysis(Module& mod);
callGraphAnalysisResult runCallGraphAnalysis(FunctionPtr func, callGraphAnalysisResult& cg);