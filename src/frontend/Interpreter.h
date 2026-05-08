#pragma once

#include "frontend/AST.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Interpreter {
public:
    explicit Interpreter(std::istream &input);
    ~Interpreter();

    int run(const ast::TranslationUnit &unit);
    std::string takeOutput();

private:
    enum class ScalarType {
        Int,
        Float,
    };

    struct ArrayObject {
        ScalarType type = ScalarType::Int;
        std::vector<int> intElements;
        std::vector<float> floatElements;
    };

    struct ArrayRef {
        std::shared_ptr<ArrayObject> object;
        std::vector<int> dims;
        std::size_t offset = 0;
        bool isConst = false;
    };

    struct Value {
        enum class Kind {
            Int,
            Float,
            Array,
            String,
        };

        Kind kind = Kind::Int;
        int intValue = 0;
        float floatValue = 0.0f;
        ArrayRef array;
        std::string stringValue;

        static Value fromInt(int value);
        static Value fromFloat(float value);
        static Value fromArray(ArrayRef ref);
        static Value fromString(std::string value);
    };

    struct Variable {
        ScalarType type = ScalarType::Int;
        int value = 0;
        float floatValue = 0.0f;
        std::shared_ptr<ArrayObject> array;
        std::vector<int> dims;
        std::vector<std::size_t> strides;
        std::size_t offset = 0;
        bool isConst = false;

        bool isArray() const;
    };

    struct ReturnSignal {
        Value value;
    };

    struct BreakSignal {};
    struct ContinueSignal {};

    struct ScopeFrame {
        std::vector<const ast::Decl *> declarations;
    };

    enum class MemoState {
        Unknown,
        Visiting,
        Memoizable,
        NonMemoizable,
    };

    std::istream &input_;
    std::ostringstream output_;
    std::vector<ScopeFrame> scopes_;
    std::unordered_map<const ast::Decl *, std::vector<Variable>> bindings_;
    std::unordered_map<std::string, const ast::FunctionDecl *> astFunctions_;
    std::unordered_set<const ast::Decl *> globalDecls_;
    std::unordered_map<const ast::FunctionDecl *, MemoState> memoStates_;
    std::unordered_map<const ast::FunctionDecl *, std::unordered_map<std::string, Value>>
        memoizedCalls_;
    std::string runtimeError_;
    std::size_t runtimeArrayCapacity_ = 0;
    static constexpr std::size_t kSysyTimerCount = 1024;
    std::array<int, kSysyTimerCount> timerL1_{};
    std::array<int, kSysyTimerCount> timerL2_{};
    std::array<int, kSysyTimerCount> timerH_{};
    std::array<int, kSysyTimerCount> timerM_{};
    std::array<int, kSysyTimerCount> timerS_{};
    std::array<int, kSysyTimerCount> timerUs_{};
    int timerIdx_ = 1;
    std::chrono::steady_clock::time_point timerStart_{};
    bool timerRunning_ = false;

    void reset();
    void pushScope();
    void popScope();
    void declareScalar(const ast::Decl &decl, int value, bool isConst);
    void declareScalar(const ast::Decl &decl, ScalarType type, const Value &value,
                       bool isConst);
    void declareArray(const ast::Decl &decl, std::vector<int> dims,
                      std::vector<Value> values, bool isConst, ScalarType type);
    void declareArrayRef(const ast::Decl &decl, const ArrayRef &ref);
    Variable &resolveVariable(const ast::Decl &decl);
    Variable &resolveVariable(const ast::DeclRefExpr &expr);

    void throwRuntimeErrorIfAny();
    void setRuntimeError(std::string message);
    static int runtimeReadInt(void *ctx, int *value);
    static int runtimeReadChar(void *ctx, int *value);
    static int runtimeReadFloat(void *ctx, float *value);
    static int runtimeGetArray(void *ctx, int values[]);
    static int runtimeGetFArray(void *ctx, float values[]);
    void appendFloatHex(float value);
    void appendFloatDecimal(float value);
    void appendPutf(const std::string &format, const std::vector<Value> &args);
    void resetTimers();
    void normalizeTimerSlot(std::size_t index);
    void appendTimerSummary();
    void startTimer(int lineno);
    void stopTimer(int lineno);
    bool isScalarDecl(const ast::Decl &decl) const;
    bool isMemoizableFunction(const ast::FunctionDecl &function);
    void collectLocalDecls(const ast::Stmt &stmt,
                           std::unordered_set<const ast::Decl *> &locals) const;
    bool isMemoizableStmt(const ast::Stmt &stmt,
                          const std::unordered_set<const ast::Decl *> &locals);
    bool isMemoizableExpr(const ast::Expr &expr,
                          const std::unordered_set<const ast::Decl *> &locals);
    std::string buildMemoKey(const std::vector<Value> &args) const;

    Value callFunction(const std::string &name, const std::vector<Value> &args);
    Value callAstFunction(const ast::FunctionDecl &function,
                          const std::vector<Value> &args);
    std::size_t aggregateSize(const std::vector<int> &dims, std::size_t level) const;
    std::vector<std::size_t> stridesOf(const std::vector<int> &dims) const;
    Value arrayElementValue(const ArrayRef &ref, std::size_t linearIndex) const;
    void setArrayElement(const ArrayRef &ref, std::size_t linearIndex, const Value &value);
    int &intArrayElement(const ArrayRef &ref, std::size_t linearIndex);
    float &floatArrayElement(const ArrayRef &ref, std::size_t linearIndex);
    int requireInt(const Value &value) const;
    float requireFloat(const Value &value) const;
    bool isTruthy(const Value &value) const;
    ArrayRef requireArray(const Value &value) const;
    Value convertValue(const Value &value, ScalarType type) const;
    std::size_t arrayStorageSize(const ArrayRef &ref) const;

    void executeDecl(const ast::Decl &decl);
    void executeVarDecl(const ast::VarDecl &decl);
    void executeStmt(const ast::Stmt &stmt);
    bool isIntScalarExpr(const ast::Expr &expr) const;
    Value evalExpr(const ast::Expr &expr);
    int evalInt(const ast::Expr &expr);
    ArrayRef evalLValRef(const ast::Expr &expr);
    std::vector<int> evalDimensions(
        const std::vector<std::unique_ptr<ast::Expr>> &dims);
    std::vector<int> evalParamTailDimensions(const ast::ParamDecl &param);
    std::vector<int> dimensionsOf(ast::Type type) const;
    std::vector<Value> buildInitValues(const std::vector<int> &dims,
                                       const ast::Expr *initializer);
    std::size_t fillInitValue(const ast::Expr &initializer,
                              const std::vector<int> &dims, std::size_t level,
                              std::size_t start, std::vector<Value> &values);
    std::size_t initializerChildLevel(const std::vector<int> &dims,
                                      std::size_t parentLevel,
                                      std::size_t parentStart,
                                      std::size_t pos) const;
    ScalarType scalarTypeOf(ast::BuiltinType type) const;

    static int parseIntLiteral(const std::string &text);
    static float parseFloatLiteral(const std::string &text);
    static std::string parseStringLiteral(const std::string &text);
};
