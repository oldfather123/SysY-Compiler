#include "../../include/ir/function_analysis.h"
#include <iostream>
#include <algorithm>

namespace llvm_ir {
namespace function_analysis {

// 入口函数
void FunctionAttributeAnalyzer::analyzeModule(Module& M) {
    // std::cout << "[function_analysis] 开始分析模块函数" << M.name << "的属性（使用工作列表算法）" << std::endl;
    
    // 设置模块引用
    module = &M;
    
    // 初始化分析
    initializeAnalysis(M);
    
    // 执行工作列表迭代
    worklistIteration();
    
    // 提交最终结果到Function对象
    commitResults();
    
    // std::cout << "[function_analysis] 模块分析完成" << std::endl;
}

void FunctionAttributeAnalyzer::initializeAnalysis(Module& M) {
    // std::cout << "[function_analysis] 初始化分析..." << std::endl;
    
    // 清空之前的数据
    functionAttributes.clear();
    functionNameMap.clear();
    callGraph.clear();
    reverseCallGraph.clear();
    worklist = std::queue<Function*>();
    inWorklist.clear();
    
    // 构建函数查找表和调用图
    buildFunctionMaps(M);
    
    // 初始化所有函数为最乐观的属性（readnone）
    for (auto& func : M.functions) {
        if (!func->isDeclaration()) {
            // 乐观地假设所有函数都是readnone
            functionAttributes[func.get()].insert(FunctionAttribute::ReadNone);
            
            // 将所有函数加入工作列表
            addToWorklist(func.get());
        }
    }
    
    // std::cout << "[function_analysis] 初始化完成，工作列表大小: " << worklist.size() << std::endl;
}

void FunctionAttributeAnalyzer::buildFunctionMaps(Module& M) {
    // std::cout << "[function_analysis] 构建函数查找表和调用图..." << std::endl;
    
    // 构建函数名到Function*的映射
    for (auto& func : M.functions) {
        functionNameMap[func->name] = func.get();
    }
    
    // 构建调用图
    for (auto& caller : M.functions) {
        if (caller->isDeclaration()) continue;
        
        for (auto& bb : caller->basicBlocks) {
            for (auto& inst : bb->instructions) {
                if (inst->opcode == Opcode::Call) {
                    auto callInst = dynamic_cast<CallInst*>(inst.get());
                    if (callInst) {
                        // 查找被调用的函数
                        Function* calledFunc = findCalledFunction(callInst->functionName);
                        if (calledFunc) {
                            // 添加调用关系
                            callGraph[caller.get()].insert(calledFunc);
                            reverseCallGraph[calledFunc].insert(caller.get());
                            
                            // std::cout << "[function_analysis] 发现调用关系: " 
                                    //  << caller->name << " -> " << calledFunc->name << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    // std::cout << "[function_analysis] 函数查找表和调用图构建完成" << std::endl;
}

void FunctionAttributeAnalyzer::worklistIteration() {
    // std::cout << "[function_analysis] 开始工作列表迭代..." << std::endl;
    
    int iteration = 0;
    while (!worklist.empty()) {
        iteration++;
        // std::cout << "[function_analysis] 迭代 " << iteration << "，工作列表大小: " << worklist.size() << std::endl;
        
        Function* F = worklist.front();
        worklist.pop();
        inWorklist.erase(F);
        
        // 重新分析函数
        bool changed = reanalyzeFunction(*F);
        
        if (changed) {
            // std::cout << "[function_analysis] 函数 " << F->name << " 的属性发生变化，将调用者加入工作列表" << std::endl;
            // 如果函数属性发生变化，将所有调用者加入工作列表
            addCallersToWorklist(F);
        }
    }
    
    // std::cout << "[function_analysis] 工作列表迭代完成，共 " << iteration << " 次迭代" << std::endl;
}

bool FunctionAttributeAnalyzer::reanalyzeFunction(Function& F) {
    // 保存旧的属性
    std::set<FunctionAttribute> oldAttrs = functionAttributes[&F];
    
    // 清空当前属性
    functionAttributes[&F].clear();
    
    // 重新分析函数
    analyzeMemoryAccess(F);
    
    // 检查属性是否发生变化
    bool changed = attributesChanged(F, oldAttrs);
    
    // if (changed) {
    //     std::cout << "[function_analysis] 函数 " << F.name << " 属性变化: ";
    //     std::cout << "旧属性: ";
    //     for (auto attr : oldAttrs) {
    //         switch (attr) {
    //             case FunctionAttribute::ReadNone: std::cout << "readnone "; break;
    //             case FunctionAttribute::ReadOnly: std::cout << "readonly "; break;
    //             case FunctionAttribute::NoUnwind: std::cout << "nounwind "; break;
    //         }
    //     }
    //     std::cout << "新属性: ";
    //     for (auto attr : functionAttributes[&F]) {
    //         switch (attr) {
    //             case FunctionAttribute::ReadNone: std::cout << "readnone "; break;
    //             case FunctionAttribute::ReadOnly: std::cout << "readonly "; break;
    //             case FunctionAttribute::NoUnwind: std::cout << "nounwind "; break;
    //         }
    //     }
    //     std::cout << std::endl;
    // }
    
    return changed;
}

void FunctionAttributeAnalyzer::commitResults() {
    // std::cout << "[function_analysis] 提交最终结果到Function对象..." << std::endl;
    
    for (auto& func : module->functions) {
        // 清空Function对象的attributes
        func->attributes.clear();
        
        // 从分析器的单一数据源复制属性
        auto it = functionAttributes.find(func.get());
        if (it != functionAttributes.end()) {
            for (auto attr : it->second) {
                func->attributes.insert(attr);
            }
        }
        
        std::cout << "[function_analysis] 函数 " << func->name << " 最终属性: ";
        for (auto attr : func->attributes) {
            switch (attr) {
                case FunctionAttribute::ReadNone: std::cout << "readnone "; break;
                case FunctionAttribute::ReadOnly: std::cout << "readonly "; break;
                case FunctionAttribute::NoUnwind: std::cout << "nounwind "; break;
            }
        }
        std::cout << std::endl;
    }
}

void FunctionAttributeAnalyzer::addToWorklist(Function* F) {
    if (inWorklist.find(F) == inWorklist.end()) {
        worklist.push(F);
        inWorklist.insert(F);
    }
}

void FunctionAttributeAnalyzer::addCallersToWorklist(const Function* F) {
    auto it = reverseCallGraph.find(F);
    if (it != reverseCallGraph.end()) {
        for (auto caller : it->second) {
            addToWorklist(const_cast<Function*>(caller));
        }
    }
}

bool FunctionAttributeAnalyzer::attributesChanged(const Function& F, const std::set<FunctionAttribute>& oldAttrs) {
    const std::set<FunctionAttribute>& newAttrs = functionAttributes[&F];
    
    if (oldAttrs.size() != newAttrs.size()) {
        return true;
    }
    
    for (auto attr : oldAttrs) {
        if (newAttrs.find(attr) == newAttrs.end()) {
            return true;
        }
    }
    
    return false;
}

Function* FunctionAttributeAnalyzer::findCalledFunction(const std::string& functionName) {
    auto it = functionNameMap.find(functionName);
    if (it != functionNameMap.end()) {
        return it->second;
    }
    return nullptr;
}

std::set<FunctionAttribute> FunctionAttributeAnalyzer::getFunctionAttributes(const Function& F) const {
    // 从单一数据源获取属性
    auto it = functionAttributes.find(&F);
    if (it != functionAttributes.end()) {
        return it->second;
    }
    return std::set<FunctionAttribute>();
}

bool FunctionAttributeAnalyzer::hasAttribute(const Function& F, FunctionAttribute attr) const {
    // 从单一数据源检查属性
    auto it = functionAttributes.find(&F);
    if (it != functionAttributes.end()) {
        return it->second.count(attr) > 0;
    }
    return false;
}

bool FunctionAttributeAnalyzer::isCallSafe(const CallInst* call) const {
    if (!call) return false;
    
    // 查找被调用的函数
    Function* calledFunc = nullptr;
    if (module) {
        auto it = functionNameMap.find(call->functionName);
        if (it != functionNameMap.end()) {
            calledFunc = it->second;
        }
    }
    
    if (calledFunc) {
        return hasAttribute(*calledFunc, FunctionAttribute::ReadNone) ||
               hasAttribute(*calledFunc, FunctionAttribute::ReadOnly);
    }
    
    // 如果找不到函数定义，保守地假设不安全
    return false;
}

void FunctionAttributeAnalyzer::analyzeMemoryAccess(Function& F) {
    bool mayRead = false;
    bool mayWrite = false;
    
    // 遍历所有基本块和指令
    for (const auto& bb : F.basicBlocks) {
        for (const auto& inst : bb->instructions) {
            if (instructionMayReadMemory(inst.get(), &F)) {
                mayRead = true;
            }
            if (instructionMayWriteMemory(inst.get(), &F)) {
                mayWrite = true;
            }
        }
    }
    
    // 根据内存访问模式设置属性
    if (!mayRead && !mayWrite) {
        // 函数不访问任何内存
        functionAttributes[&F].insert(FunctionAttribute::ReadNone);
        // std::cout << "[function_analysis] 函数 " << F.name << " 被标记为 readnone" << std::endl;
    } else if (mayRead && !mayWrite) {
        // 函数只读取内存，不写入
        functionAttributes[&F].insert(FunctionAttribute::ReadOnly);
        // std::cout << "[function_analysis] 函数 " << F.name << " 被标记为 readonly" << std::endl;
    } else {
        // 函数可能写入内存，不设置特殊属性
        // std::cout << "[function_analysis] 函数 " << F.name << " 可能写入内存，不设置特殊属性" << std::endl;
    }
}

bool FunctionAttributeAnalyzer::isReadOnly(Function& F) {
    for (const auto& bb : F.basicBlocks) {
        for (const auto& inst : bb->instructions) {
            if (instructionMayWriteMemory(inst.get())) {
                return false;
            }
        }
    }
    return true;
}

bool FunctionAttributeAnalyzer::isReadNone(Function& F) {
    for (const auto& bb : F.basicBlocks) {
        for (const auto& inst : bb->instructions) {
            if (instructionMayReadMemory(inst.get()) || instructionMayWriteMemory(inst.get())) {
                return false;
            }
        }
    }
    return true;
}

bool FunctionAttributeAnalyzer::instructionMayWriteMemory(const Instruction* I, const Function* currentFunc) {
    if (!I) return false;
    
    switch (I->opcode) {
        // 终止指令 - 不写入内存
        case Opcode::Ret:
        case Opcode::Br:
        case Opcode::Switch:
            return false;
            
        // 内存指令
        case Opcode::Store:
            // Store指令明确写入内存
            return true;
            
        case Opcode::Alloca:
            // Alloca指令分配内存，但不写入调用者可见的内存
            return false;
            
        case Opcode::Load:
            // Load指令读取内存，不写入
            return false;
            
        case Opcode::GetElementPtr:
            // GEP指令计算地址，不写入内存
            return false;
            
        // 算术指令 - 纯计算，不写入内存
        case Opcode::Add:
        case Opcode::FAdd:
        case Opcode::Sub:
        case Opcode::FSub:
        case Opcode::Mul:
        case Opcode::FMul:
        case Opcode::UDiv:
        case Opcode::SDiv:
        case Opcode::FDiv:
        case Opcode::URem:
        case Opcode::SRem:
        case Opcode::FRem:
            return false;
            
        // 位运算指令 - 纯计算，不写入内存
        case Opcode::Shl:
        case Opcode::LShr:
        case Opcode::AShr:
        case Opcode::And:
        case Opcode::Or:
        case Opcode::Xor:
            return false;
            
        // 类型转换指令 - 纯计算，不写入内存
        case Opcode::Trunc:
        case Opcode::ZExt:
        case Opcode::SExt:
        case Opcode::FPToUI:
        case Opcode::FPToSI:
        case Opcode::UIToFP:
        case Opcode::SIToFP:
        case Opcode::FPTrunc:
        case Opcode::FPExt:
        case Opcode::PtrToInt:
        case Opcode::IntToPtr:
        case Opcode::BitCast:
            return false;
            
        // 比较指令 - 纯计算，不写入内存
        case Opcode::ICmp:
        case Opcode::FCmp:
            return false;
            
        // 其他指令
        case Opcode::PHI:
            // PHI指令不写入内存
            return false;
            
        case Opcode::Call:
            // 递归检查函数调用是否写入内存
            return isCallMayWriteMemory(I, currentFunc);
            
        case Opcode::Select:
            // Select指令不写入内存
            return false;
            
        case Opcode::Move:
            // Move指令不写入内存
            return false;
            
        default:
            // 其他指令通常不写入内存
            return false;
    }
}

bool FunctionAttributeAnalyzer::instructionMayReadMemory(const Instruction* I, const Function* currentFunc) {
    if (!I) return false;
    
    switch (I->opcode) {
        // 终止指令 - 不读取内存
        case Opcode::Ret:
        case Opcode::Br:
        case Opcode::Switch:
            return false;
            
        // 内存指令
        case Opcode::Load:
            // Load指令明确读取内存
            return true;
            
        case Opcode::Store:
            // Store指令写入内存，但可能读取操作数
            return false;
            
        case Opcode::Alloca:
            // Alloca指令分配内存，不读取内存
            return false;
            
        case Opcode::GetElementPtr:
            // GEP指令计算地址，但不读取内存内容
            return false;
            
        // 算术指令 - 纯计算，不读取内存
        case Opcode::Add:
        case Opcode::FAdd:
        case Opcode::Sub:
        case Opcode::FSub:
        case Opcode::Mul:
        case Opcode::FMul:
        case Opcode::UDiv:
        case Opcode::SDiv:
        case Opcode::FDiv:
        case Opcode::URem:
        case Opcode::SRem:
        case Opcode::FRem:
            return false;
            
        // 位运算指令 - 纯计算，不读取内存
        case Opcode::Shl:
        case Opcode::LShr:
        case Opcode::AShr:
        case Opcode::And:
        case Opcode::Or:
        case Opcode::Xor:
            return false;
            
        // 类型转换指令 - 纯计算，不读取内存
        case Opcode::Trunc:
        case Opcode::ZExt:
        case Opcode::SExt:
        case Opcode::FPToUI:
        case Opcode::FPToSI:
        case Opcode::UIToFP:
        case Opcode::SIToFP:
        case Opcode::FPTrunc:
        case Opcode::FPExt:
        case Opcode::PtrToInt:
        case Opcode::IntToPtr:
        case Opcode::BitCast:
            return false;
            
        // 比较指令 - 纯计算，不读取内存
        case Opcode::ICmp:
        case Opcode::FCmp:
            return false;
            
        // 其他指令
        case Opcode::PHI:
            // PHI指令不读取内存
            return false;
            
        case Opcode::Call:
            // 递归检查函数调用是否读取内存
            return isCallMayReadMemory(I, currentFunc);
            
        case Opcode::Select:
            // Select指令不读取内存
            return false;
            
        case Opcode::Move:
            // Move指令不读取内存
            return false;
            
        default:
            // 其他指令通常不读取内存
            return false;
    }
}

bool FunctionAttributeAnalyzer::isCallSafeForHoisting(const CallInst* call) {
    if (!call) return false;
    // 这个功能目前没用过
    // 这里需要根据被调用函数的属性来判断
    // 简化实现：假设所有函数调用都不安全
    return false;
}

// 递归检查函数调用是否写入内存
bool FunctionAttributeAnalyzer::isCallMayWriteMemory(const Instruction* I, const Function* currentFunc) {
    if (!I || I->opcode != Opcode::Call) return false;
    
    auto callInst = dynamic_cast<const CallInst*>(I);
    if (!callInst) return false;
    
    // 检查是否为内置函数
    if (isBuiltinFunction(callInst->functionName)) {
        // std::cout << "[function_analysis] 函数调用 " << callInst->functionName << " 是内置函数" << std::endl;
        
        // 获取内置函数的属性
        auto attrs = getBuiltinFunctionAttributes(callInst->functionName);
        
        // 如果内置函数有readnone属性，则不会写入内存
        if (attrs.count(FunctionAttribute::ReadNone) > 0) {
            // std::cout << "[function_analysis] 内置函数 " << callInst->functionName << " 是readnone，不会写入内存" << std::endl;
            return false;
        }
        
        // 如果内置函数有readonly属性，则不会写入内存
        if (attrs.count(FunctionAttribute::ReadOnly) > 0) {
            // std::cout << "[function_analysis] 内置函数 " << callInst->functionName << " 是readonly，不会写入内存" << std::endl;
            return false;
        }
        
        // 其他内置函数可能写入内存
        // std::cout << "[function_analysis] 内置函数 " << callInst->functionName << " 可能写入内存" << std::endl;
        return true;
    }
    
    // 查找被调用的函数
    Function* calledFunc = findCalledFunction(callInst->functionName);
    
    if (calledFunc) {
        // 如果函数有readnone属性，则不会写入内存
        if (hasAttribute(*calledFunc, FunctionAttribute::ReadNone)) {
            // std::cout << "[function_analysis] 函数调用 " << callInst->functionName << " 是readnone，不会写入内存" << std::endl;
            return false;
        }
        
        // 如果函数有readonly属性，则不会写入内存
        if (hasAttribute(*calledFunc, FunctionAttribute::ReadOnly)) {
            // std::cout << "[function_analysis] 函数调用 " << callInst->functionName << " 是readonly，不会写入内存" << std::endl;
            return false;
        }
        
        // 如果函数已经被分析过但没有特殊属性，则可能写入内存
        // std::cout << "[function_analysis] 函数调用 " << callInst->functionName << " 可能写入内存" << std::endl;
        return true;
    }
    
    // 如果找不到函数定义，保守地假设可能写入内存
    // std::cout << "[function_analysis] 找不到函数定义 " << callInst->functionName << "，假设可能写入内存" << std::endl;
    return true;
}

// 递归检查函数调用是否读取内存
bool FunctionAttributeAnalyzer::isCallMayReadMemory(const Instruction* I, const Function* currentFunc) {
    if (!I || I->opcode != Opcode::Call) return false;
    
    auto callInst = dynamic_cast<const CallInst*>(I);
    if (!callInst) return false;
    
    // 检查是否为内置函数
    if (isBuiltinFunction(callInst->functionName)) {
        // std::cout << "[function_analysis] 函数调用 " << callInst->functionName << " 是内置函数" << std::endl;
        
        // 获取内置函数的属性
        auto attrs = getBuiltinFunctionAttributes(callInst->functionName);
        
        // 如果内置函数有readnone属性，则不会读取内存
        if (attrs.count(FunctionAttribute::ReadNone) > 0) {
            // std::cout << "[function_analysis] 内置函数 " << callInst->functionName << " 是readnone，不会读取内存" << std::endl;
            return false;
        }
        
        // 其他内置函数可能读取内存
        // std::cout << "[function_analysis] 内置函数 " << callInst->functionName << " 可能读取内存" << std::endl;
        return true;
    }
    
    // 查找被调用的函数
    Function* calledFunc = findCalledFunction(callInst->functionName);
    
    if (calledFunc) {
        // 如果函数有readnone属性，则不会读取内存
        if (hasAttribute(*calledFunc, FunctionAttribute::ReadNone)) {
            // std::cout << "[function_analysis] 函数调用 " << callInst->functionName << " 是readnone，不会读取内存" << std::endl;
            return false;
        }
        
        // 如果函数已经被分析过但没有readnone属性，则可能读取内存
        // std::cout << "[function_analysis] 函数调用 " << callInst->functionName << " 可能读取内存" << std::endl;
        return true;
    }
    
    // 如果找不到函数定义，保守地假设可能读取内存
    // std::cout << "[function_analysis] 找不到函数定义 " << callInst->functionName << "，假设可能读取内存" << std::endl;
    return true;
}

bool FunctionAttributeAnalyzer::isBuiltinFunction(const std::string& functionName) const {
    return builtinFunctions.find(functionName) != builtinFunctions.end();
}

std::set<FunctionAttribute> FunctionAttributeAnalyzer::getBuiltinFunctionAttributes(const std::string& functionName) const {
    std::set<FunctionAttribute> attrs;
    
    // 根据函数名判断属性
    if (functionName == "getint" || functionName == "getch" || functionName == "getfloat") {
        // 输入函数：读取内存（从stdin），不写入用户可见的内存
        attrs.insert(FunctionAttribute::ReadOnly);
    }
    else if (functionName == "getarray" || functionName == "getfarray") {
        // getarray 和 getfarray 绝不是ReadOnly的。它们会写入内存，应该不赋予任何特殊属性
    }
    else if (functionName == "putint" || functionName == "putch" || functionName == "putarray" ||
             functionName == "putfloat" || functionName == "putfarray" || functionName == "putf") {
        // 输出函数：写入内存（到stdout），读取参数
        // 不设置特殊属性，因为会写入内存
    }
    else if (functionName == "before_main" || functionName == "after_main") {
        // 这两个函数都明确地访问（读和/或写）全局内存。它们应该不赋予任何特殊属性
    }
    else if (functionName == "_sysy_starttime" || functionName == "_sysy_stoptime") {
        // 写入了全局变量 _sysy_end，读取了 _sysy_start 和 _sysy_end
        // 并且写入了多个全局数组（_sysy_l2, _sysy_us等）和全局变量 _sysy_idx
    }
    else if (functionName == "scanf" || functionName == "printf" || functionName == "fprintf" ||
             functionName == "vfprintf") {
        // 标准I/O函数：会写入内存
        // 不设置特殊属性
    }
    else if (functionName == "gettimeofday") {
        // 系统调用：读取系统时间，写入参数
        // 不设置特殊属性
    }
    
    return attrs;
}

// 便捷函数实现
void analyzeFunctionAttributes(Module& M) {
    FunctionAttributeAnalyzer analyzer(&M);
    analyzer.analyzeModule(M);
}

} // namespace function_analysis
} // namespace llvm_ir 