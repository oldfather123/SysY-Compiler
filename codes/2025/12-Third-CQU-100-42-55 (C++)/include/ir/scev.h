#ifndef LLVM_IR_SCEV_H
#define LLVM_IR_SCEV_H

#include "llvm_ir.h"
#include "cfg.h"
#include "loop_analysis.h"
#include <map>
#include <set>
#include <memory>
#include <optional>
#include <vector>
#include <cstdint>

namespace llvm_ir {

// SCEV表达式类型枚举
enum class SCEVType {
    Constant,           // 常量表达式
    Unknown,            // 未知表达式
    Truncate,          // 截断表达式
    ZeroExtend,        // 零扩展表达式
    SignExtend,        // 符号扩展表达式
    AddExpr,           // 加法表达式
    MulExpr,           // 乘法表达式
    AddRecExpr,        // 加法递归表达式（循环变量）
    MulRecExpr,        // 乘法递归表达式（循环变量）
    CastExpr           // 类型转换表达式
};

// SCEV表达式基类
class SCEV {
public:
    SCEVType type;
    Type valueType;
    
    SCEV(SCEVType t, Type vt) : type(t), valueType(vt) {}
    virtual ~SCEV() = default;
    
    // 获取表达式的字符串表示
    virtual std::string toString() const = 0;
    
    // 检查是否为常量
    virtual bool isConstant() const { return type == SCEVType::Constant; }
    
    // 获取常量值（如果可能）
    virtual std::optional<int64_t> getConstantValue() const { return std::nullopt; }
    
    // 检查是否为循环不变量
    virtual bool isLoopInvariant(const Loop* loop) const = 0;
    
    // 克隆表达式
    virtual std::unique_ptr<SCEV> clone() const = 0;
    
    // 比较两个SCEV表达式是否相等
    virtual bool equals(const SCEV* other) const = 0;
};

// 常量SCEV表达式
class SCEVConstant : public SCEV {
public:
    int64_t value;
    
    SCEVConstant(int64_t val, Type vt = Type::I32) 
        : SCEV(SCEVType::Constant, vt), value(val) {}
    
    std::string toString() const override {
        return std::to_string(value);
    }
    
    std::optional<int64_t> getConstantValue() const override {
        return value;
    }
    
    bool isLoopInvariant(const Loop* loop) const override {
        return true; // 常量总是循环不变量
    }
    
    std::unique_ptr<SCEV> clone() const override {
        return std::make_unique<SCEVConstant>(value, valueType);
    }
    
    bool equals(const SCEV* other) const override {
        if (!other || other->type != SCEVType::Constant) return false;
        auto* otherConst = static_cast<const SCEVConstant*>(other);
        return value == otherConst->value && valueType == otherConst->valueType;
    }
};

// 未知SCEV表达式
class SCEVUnknown : public SCEV {
public:
    Value* value;
    
    SCEVUnknown(Value* val) 
        : SCEV(SCEVType::Unknown, val->type), value(val) {}
    
    std::string toString() const override {
        return value->name;
    }
    
    bool isLoopInvariant(const Loop* loop) const override {
        if (!loop) return true;
        // 检查值是否在循环外定义
        if (auto* inst = dynamic_cast<Instruction*>(value)) {
            return !loop->contains(inst);
        }
        return true; // 非指令值（如参数）认为是循环不变量
    }
    
    std::unique_ptr<SCEV> clone() const override {
        return std::make_unique<SCEVUnknown>(value);
    }
    
    bool equals(const SCEV* other) const override {
        if (!other || other->type != SCEVType::Unknown) return false;
        auto* otherUnknown = static_cast<const SCEVUnknown*>(other);
        return value == otherUnknown->value;
    }
};

// 加法SCEV表达式
class SCEVAddExpr : public SCEV {
public:
    std::vector<std::unique_ptr<SCEV>> operands;
    
    SCEVAddExpr(std::vector<std::unique_ptr<SCEV>> ops, Type vt = Type::I32)
        : SCEV(SCEVType::AddExpr, vt) {
        operands = std::move(ops);
    }
    
    std::string toString() const override {
        std::string result = "(";
        for (size_t i = 0; i < operands.size(); ++i) {
            if (i > 0) result += " + ";
            result += operands[i]->toString();
        }
        result += ")";
        return result;
    }
    
    bool isLoopInvariant(const Loop* loop) const override {
        for (const auto& op : operands) {
            if (!op->isLoopInvariant(loop)) {
                return false;
            }
        }
        return true;
    }
    
    std::unique_ptr<SCEV> clone() const override {
        std::vector<std::unique_ptr<SCEV>> cloned_ops;
        for (const auto& op : operands) {
            cloned_ops.push_back(op->clone());
        }
        return std::make_unique<SCEVAddExpr>(std::move(cloned_ops), valueType);
    }
    
    bool equals(const SCEV* other) const override {
        if (!other || other->type != SCEVType::AddExpr) return false;
        auto* otherAdd = static_cast<const SCEVAddExpr*>(other);
        if (operands.size() != otherAdd->operands.size()) return false;
        for (size_t i = 0; i < operands.size(); ++i) {
            if (!operands[i]->equals(otherAdd->operands[i].get())) {
                return false;
            }
        }
        return true;
    }
};

// 乘法SCEV表达式
class SCEVMulExpr : public SCEV {
public:
    std::vector<std::unique_ptr<SCEV>> operands;
    
    SCEVMulExpr(std::vector<std::unique_ptr<SCEV>> ops, Type vt = Type::I32)
        : SCEV(SCEVType::MulExpr, vt) {
        operands = std::move(ops);
    }
    
    std::string toString() const override {
        std::string result = "(";
        for (size_t i = 0; i < operands.size(); ++i) {
            if (i > 0) result += " * ";
            result += operands[i]->toString();
        }
        result += ")";
        return result;
    }
    
    bool isLoopInvariant(const Loop* loop) const override {
        for (const auto& op : operands) {
            if (!op->isLoopInvariant(loop)) {
                return false;
            }
        }
        return true;
    }
    
    std::unique_ptr<SCEV> clone() const override {
        std::vector<std::unique_ptr<SCEV>> cloned_ops;
        for (const auto& op : operands) {
            cloned_ops.push_back(op->clone());
        }
        return std::make_unique<SCEVMulExpr>(std::move(cloned_ops), valueType);
    }
    
    bool equals(const SCEV* other) const override {
        if (!other || other->type != SCEVType::MulExpr) return false;
        auto* otherMul = static_cast<const SCEVMulExpr*>(other);
        if (operands.size() != otherMul->operands.size()) return false;
        for (size_t i = 0; i < operands.size(); ++i) {
            if (!operands[i]->equals(otherMul->operands[i].get())) {
                return false;
            }
        }
        return true;
    }
};

// 加法递归SCEV表达式（循环变量）
class SCEVAddRecExpr : public SCEV {
public:
    std::unique_ptr<SCEV> start;      // 初始值
    std::unique_ptr<SCEV> step;       // 步长
    const Loop* loop;                 // 所属循环
    
    SCEVAddRecExpr(std::unique_ptr<SCEV> s, std::unique_ptr<SCEV> st, 
                   const Loop* l, Type vt = Type::I32)
        : SCEV(SCEVType::AddRecExpr, vt), start(std::move(s)), 
          step(std::move(st)), loop(l) {}
    
    std::string toString() const override {
        return "{" + start->toString() + ", +, " + step->toString() + "}";
    }
    
    bool isLoopInvariant(const Loop* loop) const override {
        return false; // 循环变量不是循环不变量
    }
    
    std::unique_ptr<SCEV> clone() const override {
        return std::make_unique<SCEVAddRecExpr>(
            start->clone(), step->clone(), loop, valueType);
    }
    
    bool equals(const SCEV* other) const override {
        if (!other || other->type != SCEVType::AddRecExpr) return false;
        auto* otherAddRec = static_cast<const SCEVAddRecExpr*>(other);
        return start->equals(otherAddRec->start.get()) && 
               step->equals(otherAddRec->step.get()) && 
               loop == otherAddRec->loop;
    }
};

// 乘法递归SCEV表达式（循环变量）
class SCEVMulRecExpr : public SCEV {
public:
    std::unique_ptr<SCEV> start;      // 初始值
    std::unique_ptr<SCEV> factor;     // 乘数因子
    const Loop* loop;                 // 所属循环
    
    SCEVMulRecExpr(std::unique_ptr<SCEV> s, std::unique_ptr<SCEV> f, 
                   const Loop* l, Type vt = Type::I32)
        : SCEV(SCEVType::MulRecExpr, vt), start(std::move(s)), 
          factor(std::move(f)), loop(l) {}
    
    std::string toString() const override {
        return "{" + start->toString() + ", *, " + factor->toString() + "}";
    }
    
    bool isLoopInvariant(const Loop* loop) const override {
        return false; // 循环变量不是循环不变量
    }
    
    std::unique_ptr<SCEV> clone() const override {
        return std::make_unique<SCEVMulRecExpr>(
            start->clone(), factor->clone(), loop, valueType);
    }
    
    bool equals(const SCEV* other) const override {
        if (!other || other->type != SCEVType::MulRecExpr) return false;
        auto* otherMulRec = static_cast<const SCEVMulRecExpr*>(other);
        return start->equals(otherMulRec->start.get()) && 
               factor->equals(otherMulRec->factor.get()) && 
               loop == otherMulRec->loop;
    }
};

struct SCEVLoopVariableInfo {
    Value* variable;           // 循环变量
    std::unique_ptr<SCEV> scev; // 对应的SCEV表达式
    Value* initialValue;       // 初始值
    Value* stepValue;          // 步长值
    Value* lowerBound;         // 下界
    Value* upperBound;         // 上界
    ICmpCond exitCondition;    // 循环退出条件
    bool isIncrement;          // 是否为递增
    bool isSimple;             // 是否为简单循环
    bool isPrimary;            // 是否为主循环变量
    std::optional<int64_t> constantTripCount; // 常量循环次数（若可解析）
    
    SCEVLoopVariableInfo() : variable(nullptr), initialValue(nullptr), 
                            stepValue(nullptr), lowerBound(nullptr), upperBound(nullptr),
                            exitCondition(ICmpCond::EQ), isIncrement(true), isSimple(false), isPrimary(false) {}

    // ========== 常量提取便捷方法 ==========
    // 将任意 Value*（若为常量）尝试转为 int32
    static std::optional<int32_t> tryConvertValueToInt32(Value* value) {
        if (!value) return std::nullopt;
        if (auto* c = dynamic_cast<ConstantInt*>(value)) {
            return static_cast<int32_t>(c->value);
        }
        if (auto* f = dynamic_cast<ConstantFloat*>(value)) {
            return static_cast<int32_t>(f->value);
        }
        return std::nullopt;
    }

    // 将任意 Value*（若为常量）尝试转为 float
    static std::optional<float> tryConvertValueToFloat(Value* value) {
        if (!value) return std::nullopt;
        if (auto* f = dynamic_cast<ConstantFloat*>(value)) {
            return f->value;
        }
        if (auto* c = dynamic_cast<ConstantInt*>(value)) {
            return static_cast<float>(c->value);
        }
        return std::nullopt;
    }

    // 对结构体内各字段的便捷访问（int32）
    std::optional<int32_t> getInitialValueAsInt32() const { return tryConvertValueToInt32(initialValue); }
    std::optional<int32_t> getStepValueAsInt32() const { return tryConvertValueToInt32(stepValue); }
    std::optional<int32_t> getLowerBoundAsInt32() const { return tryConvertValueToInt32(lowerBound); }
    std::optional<int32_t> getUpperBoundAsInt32() const { return tryConvertValueToInt32(upperBound); }

    // 对结构体内各字段的便捷访问（float）
    std::optional<float> getInitialValueAsFloat() const { return tryConvertValueToFloat(initialValue); }
    std::optional<float> getStepValueAsFloat() const { return tryConvertValueToFloat(stepValue); }
    std::optional<float> getLowerBoundAsFloat() const { return tryConvertValueToFloat(lowerBound); }
    std::optional<float> getUpperBoundAsFloat() const { return tryConvertValueToFloat(upperBound); }
};

// SCEV分析器
class SCEVAnalysis {
    public:
    Function* function;
    Module* module;
    Loop* currentLoop;
    
    // 缓存：值到SCEV表达式的映射
    std::map<Value*, std::unique_ptr<SCEV>> scevCache;
    
    // 每个循环的变量信息映射：Loop* -> map<Value*, SCEVLoopVariableInfo>
    std::map<Loop*, std::map<Value*, SCEVLoopVariableInfo>> allLoopInfo;
    
    // 循环不变量集合
    std::set<Value*> invariantValues;
    
    // 递归检测集合，用于避免getSCEV中的无限递归
    std::set<Value*> scevBeingComputed;
    
    SCEVAnalysis(Function* F = nullptr, Module* M = nullptr) : function(F), module(M), currentLoop(nullptr) {}
    
    // 设置当前分析的函数、模块和循环
    void setFunction(Function* F) { function = F; }
    void setModule(Module* M) { module = M; }
    void setLoop(Loop* L) { currentLoop = L; }
    
    // 递归分析循环嵌套结构
    void analyzeLoopRecursively(Loop* loop, int depth);

    // 单循环分析接口
    void analyzeLoop(Loop* loop);
    
    // 获取SCEV表达式
    std::unique_ptr<SCEV> getSCEV(Value* value);
    std::unique_ptr<SCEV> getSCEV(Instruction* inst);
    
    // 检查值是否为循环不变量
    bool isLoopInvariant(Value* value);
    
    // 获取所有循环的变量信息
    const std::map<Loop*, std::map<Value*, SCEVLoopVariableInfo>>& getAllLoopInfo() const {
        return allLoopInfo;
    }
    
    // 获取特定循环的变量信息
    const std::map<Value*, SCEVLoopVariableInfo>& getLoopVariables(Loop* loop) const {
        static std::map<Value*, SCEVLoopVariableInfo> empty;
        auto it = allLoopInfo.find(loop);
        return (it != allLoopInfo.end()) ? it->second : empty;
    }
    
    // 获取特定循环的主循环变量
    Value* getPrimaryLoopVariable(Loop* loop = nullptr) const {
        if (!loop) loop = currentLoop;
        if (!loop) {
            std::cout << "[scev][getPVar] loop is nullptr" << std::endl;
            return nullptr;
        }
        
        auto it = allLoopInfo.find(loop);
        if (it == allLoopInfo.end()) {
            std::cout << "[scev][getPVar] No loop info found for loop" << std::endl;
            return nullptr;
        }
        
        for (const auto& [var, info] : it->second) {
            if (info.isPrimary) {
                std::cout << "[scev][getPVar] Found primary loop variable: " << var->name << std::endl;
                return var;
            }
        }
        std::cout << "[scev][getPVar] No primary loop variable found for loop" << std::endl;
        return nullptr;
    }
    
    // 计算循环次数（为当前循环内每个可解析变量填充常量trip count）
    void computeLoopTripCounts();

    // 获取当前循环的常量循环次数（优先主变量，否则任一可解析变量）。
    std::optional<int64_t> getLoopTripCount(Loop* loop = nullptr) const;

    // 获取循环不变量集合
    const std::set<Value*>& getInvariantValues() const {
        return invariantValues;
    }
    
    // 打印分析结果
    void printAnalysisResults() const;
    
    // 打印函数级别的分析总结
    void printFunctionSummary() const;
    
    // 帮助函数：获取当前循环的变量信息映射
    std::map<Value*, SCEVLoopVariableInfo>& getCurrentLoopVariables();
    const std::map<Value*, SCEVLoopVariableInfo>& getCurrentLoopVariables() const;
    
private:
    // 内部分析方法
    void findLoopInvariants(); 
    void findLoopVariables();
    void analyzeRecurrences();
    
    // 辅助方法
    std::unique_ptr<SCEV> createSCEVForValue(Value* value);
    std::unique_ptr<SCEV> createSCEVForInstruction(Instruction* inst);
    std::unique_ptr<SCEV> createSCEVForBinaryOp(BinaryOperator* inst);
    std::unique_ptr<SCEV> createSCEVForPhi(PhiInst* inst);
    
    // 分析PHI指令作为循环变量
    bool analyzePhiAsLoopVariable(PhiInst* phi, Value*& initialValue, Value*& stepValue, bool& isMultiplication);
    
    // 检查分支条件是否为简单的比较
    bool isSimpleComparison(Instruction* inst, Value*& var, Value*& bound, 
                           bool& isUpperBound, ICmpCond& condition);
    
    // 分析循环边界
    void analyzeLoopBounds();
    
    // 查找循环退出条件
    void findLoopExitConditions();
    
    // 识别主循环变量
    void identifyPrimaryLoopVariable();
    
    // 检查值是否为循环变量
    bool isLoopVariable(Value* value);
    
    // 查找变量的初始值
    Value* findInitialValue(Value* var);
    
    // 简化SCEV表达式
    std::unique_ptr<SCEV> simplifySCEV(std::unique_ptr<SCEV> scev);
    
    // 合并SCEV表达式
    std::unique_ptr<SCEV> addSCEV(std::unique_ptr<SCEV> a, std::unique_ptr<SCEV> b);
    std::unique_ptr<SCEV> mulSCEV(std::unique_ptr<SCEV> a, std::unique_ptr<SCEV> b);
};

// 便捷函数：分析模块中的所有函数
SCEVAnalysis analyzeSCEV(Function& F, Module& M, const LoopAnalysis& LA);

} // namespace llvm_ir

#endif // LLVM_IR_SCEV_H 