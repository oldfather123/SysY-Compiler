#include "frontend/Interpreter.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>
#include <stdexcept>
#include <utility>

Interpreter::Value Interpreter::Value::fromInt(int value) {
    Value result;
    result.kind = Kind::Int;
    result.intValue = value;
    return result;
}

Interpreter::Value Interpreter::Value::fromFloat(float value) {
    Value result;
    result.kind = Kind::Float;
    result.floatValue = value;
    return result;
}

Interpreter::Value Interpreter::Value::fromArray(ArrayRef ref) {
    Value result;
    result.kind = Kind::Array;
    result.array = std::move(ref);
    return result;
}

Interpreter::Value Interpreter::Value::fromString(std::string value) {
    Value result;
    result.kind = Kind::String;
    result.stringValue = std::move(value);
    return result;
}

bool Interpreter::Variable::isArray() const {
    return array != nullptr;
}

Interpreter::Interpreter(std::istream &input) : input_(input) { reset(); }

Interpreter::~Interpreter() = default;

int Interpreter::run(const ast::TranslationUnit &unit) {
    reset();

    for (const std::unique_ptr<ast::Decl> &decl : unit.declarations) {
        if (const auto *function = dynamic_cast<const ast::FunctionDecl *>(decl.get())) {
            astFunctions_[function->name] = function;
        } else {
            globalDecls_.insert(decl.get());
        }
    }

    for (const std::unique_ptr<ast::Decl> &decl : unit.declarations) {
        if (dynamic_cast<const ast::FunctionDecl *>(decl.get()) == nullptr) {
            executeDecl(*decl);
        }
    }

    const int result = requireInt(callFunction("main", {}));
    appendTimerSummary();
    return result;
}

std::string Interpreter::takeOutput() {
    return output_.str();
}

std::string Interpreter::takeTimerOutput() {
    return timerOutput_.str();
}

void Interpreter::reset() {
    output_.str("");
    output_.clear();
    timerOutput_.str("");
    timerOutput_.clear();
    scopes_.clear();
    scopes_.emplace_back();
    bindings_.clear();
    astFunctions_.clear();
    globalDecls_.clear();
    memoStates_.clear();
    memoizedCalls_.clear();
    runtimeError_.clear();
    runtimeArrayCapacity_ = 0;
    resetTimers();
}

void Interpreter::pushScope() {
    scopes_.emplace_back();
}

void Interpreter::popScope() {
    ScopeFrame &scope = scopes_.back();
    for (auto it = scope.declarations.rbegin(); it != scope.declarations.rend(); ++it) {
        auto found = bindings_.find(*it);
        if (found == bindings_.end() || found->second.empty()) {
            throw std::runtime_error("runtime scope stack corrupted");
        }
        found->second.pop_back();
        if (found->second.empty()) {
            bindings_.erase(found);
        }
    }
    scopes_.pop_back();
}

void Interpreter::declareScalar(const ast::Decl &decl, ScalarType type, const Value &value,
                                bool isConst) {
    auto &scope = scopes_.back();
    if (std::find(scope.declarations.begin(), scope.declarations.end(), &decl) !=
        scope.declarations.end()) {
        throw std::runtime_error("duplicate declaration in runtime scope");
    }

    const Value converted = convertValue(value, type);
    Variable variable;
    variable.type = type;
    variable.isConst = isConst;
    if (type == ScalarType::Int) {
        variable.value = converted.intValue;
    } else {
        variable.floatValue = converted.floatValue;
    }
    scope.declarations.push_back(&decl);
    bindings_[&decl].push_back(std::move(variable));
}

void Interpreter::declareArray(const ast::Decl &decl, std::vector<int> dims,
                               std::vector<Value> values, bool isConst,
                               ScalarType type) {
    auto &scope = scopes_.back();
    if (std::find(scope.declarations.begin(), scope.declarations.end(), &decl) !=
        scope.declarations.end()) {
        throw std::runtime_error("duplicate declaration in runtime scope");
    }

    const std::size_t total = aggregateSize(dims, 0);
    values.resize(total, Value::fromInt(0));

    Variable variable;
    variable.type = type;
    variable.array = std::make_shared<ArrayObject>();
    variable.array->type = type;
    variable.dims = std::move(dims);
    variable.strides = stridesOf(variable.dims);
    variable.offset = 0;
    variable.isConst = isConst;
    if (type == ScalarType::Int) {
        variable.array->intElements.resize(total);
        for (std::size_t i = 0; i < total; ++i) {
            variable.array->intElements[i] = requireInt(values[i]);
        }
    } else {
        variable.array->floatElements.resize(total);
        for (std::size_t i = 0; i < total; ++i) {
            variable.array->floatElements[i] = requireFloat(values[i]);
        }
    }
    scope.declarations.push_back(&decl);
    bindings_[&decl].push_back(std::move(variable));
}

void Interpreter::declareArrayRef(const ast::Decl &decl, const ArrayRef &ref) {
    auto &scope = scopes_.back();
    if (std::find(scope.declarations.begin(), scope.declarations.end(), &decl) !=
        scope.declarations.end()) {
        throw std::runtime_error("duplicate declaration in runtime scope");
    }

    Variable variable;
    variable.type = ref.object->type;
    variable.array = ref.object;
    variable.dims = ref.dims;
    variable.strides = stridesOf(variable.dims);
    variable.offset = ref.offset;
    variable.isConst = ref.isConst;
    scope.declarations.push_back(&decl);
    bindings_[&decl].push_back(std::move(variable));
}

Interpreter::Variable &Interpreter::resolveVariable(const ast::Decl &decl) {
    auto found = bindings_.find(&decl);
    if (found != bindings_.end() && !found->second.empty()) {
        return found->second.back();
    }
    throw std::runtime_error("undefined variable");
}

Interpreter::Variable &Interpreter::resolveVariable(const ast::DeclRefExpr &expr) {
    if (expr.referencedDecl == nullptr) {
        throw std::runtime_error("unbound variable reference: " + expr.name);
    }
    return resolveVariable(*expr.referencedDecl);
}

void Interpreter::throwRuntimeErrorIfAny() {
    if (runtimeError_.empty()) {
        return;
    }
    std::string message = std::move(runtimeError_);
    runtimeError_.clear();
    throw std::runtime_error(message);
}

void Interpreter::setRuntimeError(std::string message) {
    if (runtimeError_.empty()) {
        runtimeError_ = std::move(message);
    }
}

int Interpreter::runtimeReadInt(void *ctx, int *value) {
    auto *self = static_cast<Interpreter *>(ctx);
    std::string token;
    if (!(self->input_ >> token)) {
        *value = 0;
        self->setRuntimeError("getint input exhausted");
        return 0;
    }
    try {
        *value = parseIntLiteral(token);
    } catch (const std::exception &) {
        *value = 0;
        self->setRuntimeError("invalid integer input");
        return 0;
    }
    return 1;
}

int Interpreter::runtimeReadChar(void *ctx, int *value) {
    auto *self = static_cast<Interpreter *>(ctx);
    char ch = '\0';
    if (!self->input_.get(ch)) {
        *value = 0;
        self->setRuntimeError("getch input exhausted");
        return 0;
    }
    *value = static_cast<unsigned char>(ch);
    return 1;
}

int Interpreter::runtimeReadFloat(void *ctx, float *value) {
    auto *self = static_cast<Interpreter *>(ctx);
    std::string token;
    if (!(self->input_ >> token)) {
        *value = 0.0f;
        self->setRuntimeError("getfloat input exhausted");
        return 0;
    }
    *value = parseFloatLiteral(token);
    return 1;
}

int Interpreter::runtimeGetArray(void *ctx, int values[]) {
    auto *self = static_cast<Interpreter *>(ctx);
    int n = 0;
    if (!runtimeReadInt(ctx, &n)) {
        return 0;
    }
    for (int i = 0; i < n; ++i) {
        int value = 0;
        if (!runtimeReadInt(ctx, &value)) {
            return n;
        }
        if (i < 0 || static_cast<std::size_t>(i) >= self->runtimeArrayCapacity_) {
            self->setRuntimeError("getarray input exceeds array storage");
            continue;
        }
        values[i] = value;
    }
    return n;
}

int Interpreter::runtimeGetFArray(void *ctx, float values[]) {
    auto *self = static_cast<Interpreter *>(ctx);
    int n = 0;
    if (!runtimeReadInt(ctx, &n)) {
        return 0;
    }
    for (int i = 0; i < n; ++i) {
        float value = 0.0f;
        if (!runtimeReadFloat(ctx, &value)) {
            return n;
        }
        if (i < 0 || static_cast<std::size_t>(i) >= self->runtimeArrayCapacity_) {
            self->setRuntimeError("getfarray input exceeds array storage");
            continue;
        }
        values[i] = value;
    }
    return n;
}

void Interpreter::appendFloatHex(float value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%a", value);
    output_ << buffer;
}

void Interpreter::appendFloatDecimal(float value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%f", value);
    output_ << buffer;
}

void Interpreter::appendPutf(const std::string &format, const std::vector<Value> &args) {
    std::size_t argIndex = 0;
    for (std::size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '%' || i + 1 >= format.size()) {
            output_.put(format[i]);
            continue;
        }

        const char spec = format[++i];
        if (spec == '%') {
            output_.put('%');
            continue;
        }
        if (argIndex >= args.size()) {
            throw std::runtime_error("putf argument count mismatch");
        }

        const Value &value = args[argIndex++];
        switch (spec) {
        case 'd':
            output_ << requireInt(value);
            break;
        case 'c':
            output_.put(static_cast<char>(requireInt(value)));
            break;
        case 'f':
            appendFloatDecimal(requireFloat(value));
            break;
        default:
            throw std::runtime_error("unsupported putf format specifier");
        }
    }
}

void Interpreter::resetTimers() {
    timerL1_.fill(0);
    timerL2_.fill(0);
    timerH_.fill(0);
    timerM_.fill(0);
    timerS_.fill(0);
    timerUs_.fill(0);
    timerIdx_ = 1;
    timerRunning_ = false;
}

void Interpreter::normalizeTimerSlot(std::size_t index) {
    timerS_[index] += timerUs_[index] / 1000000;
    timerUs_[index] %= 1000000;
    timerM_[index] += timerS_[index] / 60;
    timerS_[index] %= 60;
    timerH_[index] += timerM_[index] / 60;
    timerM_[index] %= 60;
}

void Interpreter::appendTimerSummary() {
    if (timerIdx_ <= 1) {
        return;
    }

    for (int i = 1; i < timerIdx_; ++i) {
        timerUs_[0] += timerUs_[i];
        timerS_[0] += timerS_[i];
        timerM_[0] += timerM_[i];
        timerH_[0] += timerH_[i];
        normalizeTimerSlot(0);
        char lineBuffer[64];
        std::snprintf(lineBuffer, sizeof(lineBuffer),
                      "     Timer@%04d-%04d: %dH-%dM-%dS-%dus\n", timerL1_[i],
                      timerL2_[i], timerH_[i], timerM_[i], timerS_[i], timerUs_[i]);
        timerOutput_ << lineBuffer;
    }
    timerOutput_ << "       TOTAL: " << timerH_[0] << "H-" << timerM_[0] << "M-"
                 << timerS_[0] << "S-" << timerUs_[0] << "us\n";
}

void Interpreter::startTimer(int lineno) {
    if (timerIdx_ >= static_cast<int>(kSysyTimerCount)) {
        return;
    }
    timerL1_[timerIdx_] = lineno;
    timerStart_ = std::chrono::steady_clock::now();
    timerRunning_ = true;
}

void Interpreter::stopTimer(int lineno) {
    if (!timerRunning_ || timerIdx_ >= static_cast<int>(kSysyTimerCount)) {
        return;
    }

    const auto end = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::microseconds>(end - timerStart_).count();
    timerL2_[timerIdx_] = lineno;
    timerUs_[timerIdx_] += static_cast<int>(elapsed);
    normalizeTimerSlot(static_cast<std::size_t>(timerIdx_));
    ++timerIdx_;
    timerRunning_ = false;
}

bool Interpreter::isScalarDecl(const ast::Decl &decl) const {
    if (const auto *param = dynamic_cast<const ast::ParamDecl *>(&decl)) {
        return !param->type.isPointer && param->type.isScalar();
    }
    if (const auto *var = dynamic_cast<const ast::VarDecl *>(&decl)) {
        return !var->type.isArray();
    }
    return false;
}

void Interpreter::collectLocalDecls(const ast::Stmt &stmt,
                                    std::unordered_set<const ast::Decl *> &locals) const {
    if (const auto *compound = dynamic_cast<const ast::CompoundStmt *>(&stmt)) {
        for (const std::unique_ptr<ast::Stmt> &child : compound->statements) {
            collectLocalDecls(*child, locals);
        }
        return;
    }

    if (const auto *declStmt = dynamic_cast<const ast::DeclStmt *>(&stmt)) {
        for (const std::unique_ptr<ast::VarDecl> &decl : declStmt->declarations) {
            locals.insert(decl.get());
        }
        return;
    }

    if (const auto *ifStmt = dynamic_cast<const ast::IfStmt *>(&stmt)) {
        if (ifStmt->thenBranch != nullptr) {
            collectLocalDecls(*ifStmt->thenBranch, locals);
        }
        if (ifStmt->elseBranch != nullptr) {
            collectLocalDecls(*ifStmt->elseBranch, locals);
        }
        return;
    }

    if (const auto *whileStmt = dynamic_cast<const ast::WhileStmt *>(&stmt)) {
        if (whileStmt->body != nullptr) {
            collectLocalDecls(*whileStmt->body, locals);
        }
    }
}

bool Interpreter::isMemoizableExpr(
    const ast::Expr &expr, const std::unordered_set<const ast::Decl *> &locals) {
    if (dynamic_cast<const ast::IntegerLiteral *>(&expr) != nullptr ||
        dynamic_cast<const ast::FloatLiteral *>(&expr) != nullptr ||
        dynamic_cast<const ast::StringLiteral *>(&expr) != nullptr) {
        return true;
    }

    if (const auto *cast = dynamic_cast<const ast::ImplicitCastExpr *>(&expr)) {
        return isMemoizableExpr(*cast->subExpr, locals);
    }

    if (const auto *ref = dynamic_cast<const ast::DeclRefExpr *>(&expr)) {
        if (ref->referencedDecl == nullptr || !isScalarDecl(*ref->referencedDecl)) {
            return false;
        }
        if (locals.find(ref->referencedDecl) != locals.end()) {
            return true;
        }
        const auto *var = dynamic_cast<const ast::VarDecl *>(ref->referencedDecl);
        return var != nullptr && globalDecls_.find(var) != globalDecls_.end() &&
               var->type.isConst;
    }

    if (dynamic_cast<const ast::ArraySubscriptExpr *>(&expr) != nullptr ||
        dynamic_cast<const ast::InitListExpr *>(&expr) != nullptr) {
        return false;
    }

    if (const auto *unary = dynamic_cast<const ast::UnaryOperator *>(&expr)) {
        return isMemoizableExpr(*unary->operand, locals);
    }

    if (const auto *binary = dynamic_cast<const ast::BinaryOperator *>(&expr)) {
        return isMemoizableExpr(*binary->lhs, locals) &&
               isMemoizableExpr(*binary->rhs, locals);
    }

    if (const auto *call = dynamic_cast<const ast::CallExpr *>(&expr)) {
        for (const std::unique_ptr<ast::Expr> &argument : call->arguments) {
            if (!isMemoizableExpr(*argument, locals)) {
                return false;
            }
        }
        if (call->isBuiltin || call->calleeDecl == nullptr) {
            return false;
        }
        return isMemoizableFunction(*call->calleeDecl);
    }

    return false;
}

bool Interpreter::isMemoizableStmt(
    const ast::Stmt &stmt, const std::unordered_set<const ast::Decl *> &locals) {
    if (const auto *compound = dynamic_cast<const ast::CompoundStmt *>(&stmt)) {
        for (const std::unique_ptr<ast::Stmt> &child : compound->statements) {
            if (!isMemoizableStmt(*child, locals)) {
                return false;
            }
        }
        return true;
    }

    if (const auto *declStmt = dynamic_cast<const ast::DeclStmt *>(&stmt)) {
        for (const std::unique_ptr<ast::VarDecl> &decl : declStmt->declarations) {
            if (!isScalarDecl(*decl)) {
                return false;
            }
            if (decl->initializer != nullptr &&
                !isMemoizableExpr(*decl->initializer, locals)) {
                return false;
            }
        }
        return true;
    }

    if (dynamic_cast<const ast::NullStmt *>(&stmt) != nullptr) {
        return true;
    }

    if (const auto *ifStmt = dynamic_cast<const ast::IfStmt *>(&stmt)) {
        return ifStmt->condition != nullptr &&
               isMemoizableExpr(*ifStmt->condition, locals) &&
               (ifStmt->thenBranch == nullptr ||
                isMemoizableStmt(*ifStmt->thenBranch, locals)) &&
               (ifStmt->elseBranch == nullptr ||
                isMemoizableStmt(*ifStmt->elseBranch, locals));
    }

    if (const auto *whileStmt = dynamic_cast<const ast::WhileStmt *>(&stmt)) {
        return whileStmt->condition != nullptr &&
               isMemoizableExpr(*whileStmt->condition, locals) &&
               (whileStmt->body == nullptr ||
                isMemoizableStmt(*whileStmt->body, locals));
    }

    if (dynamic_cast<const ast::BreakStmt *>(&stmt) != nullptr ||
        dynamic_cast<const ast::ContinueStmt *>(&stmt) != nullptr) {
        return true;
    }

    if (const auto *returnStmt = dynamic_cast<const ast::ReturnStmt *>(&stmt)) {
        return returnStmt->value == nullptr ||
               isMemoizableExpr(*returnStmt->value, locals);
    }

    if (const auto *expr = dynamic_cast<const ast::Expr *>(&stmt)) {
        return isMemoizableExpr(*expr, locals);
    }

    return false;
}

bool Interpreter::isMemoizableFunction(const ast::FunctionDecl &function) {
    auto found = memoStates_.find(&function);
    if (found != memoStates_.end()) {
        if (found->second == MemoState::Visiting || found->second == MemoState::Memoizable) {
            return true;
        }
        if (found->second == MemoState::NonMemoizable) {
            return false;
        }
    }

    memoStates_[&function] = MemoState::Visiting;
    bool memoizable = function.returnType.isScalar() && function.returnType.base != ast::BuiltinType::Void &&
                      function.body != nullptr;

    std::unordered_set<const ast::Decl *> locals;
    if (memoizable) {
        for (const std::unique_ptr<ast::ParamDecl> &param : function.params) {
            if (!isScalarDecl(*param)) {
                memoizable = false;
                break;
            }
            locals.insert(param.get());
        }
    }

    if (memoizable) {
        collectLocalDecls(*function.body, locals);
        memoizable = isMemoizableStmt(*function.body, locals);
    }

    memoStates_[&function] =
        memoizable ? MemoState::Memoizable : MemoState::NonMemoizable;
    return memoizable;
}

std::string Interpreter::buildMemoKey(const std::vector<Value> &args) const {
    std::string key;
    key.reserve(args.size() * 8);
    for (const Value &arg : args) {
        if (arg.kind == Value::Kind::Int) {
            key.push_back('i');
            const int value = arg.intValue;
            key.append(reinterpret_cast<const char *>(&value), sizeof(value));
            continue;
        }
        if (arg.kind == Value::Kind::Float) {
            key.push_back('f');
            const float value = arg.floatValue;
            key.append(reinterpret_cast<const char *>(&value), sizeof(value));
            continue;
        }
        throw std::runtime_error("memoization only supports scalar arguments");
    }
    return key;
}

Interpreter::Value Interpreter::callFunction(const std::string &name,
                                             const std::vector<Value> &args) {
    if (name == "getint" || name == "getch" || name == "getfloat") {
        if (!args.empty()) {
            throw std::runtime_error(name + " expects 0 arguments");
        }

        if (name == "getfloat") {
            float value = 0.0f;
            runtimeReadFloat(this, &value);
            throwRuntimeErrorIfAny();
            return Value::fromFloat(value);
        }

        int value = 0;
        if (name == "getint") {
            runtimeReadInt(this, &value);
        } else {
            runtimeReadChar(this, &value);
        }
        throwRuntimeErrorIfAny();
        return Value::fromInt(value);
    }

    if (name == "getarray" || name == "getfarray") {
        if (args.size() != 1) {
            throw std::runtime_error(name + " expects 1 argument");
        }
        ArrayRef ref = requireArray(args[0]);
        const bool wantsFloat = name == "getfarray";
        if ((ref.object->type == ScalarType::Float) != wantsFloat) {
            throw std::runtime_error(name + " argument type mismatch");
        }
        runtimeArrayCapacity_ = arrayStorageSize(ref) - ref.offset;
        float *floatData = ref.object->floatElements.empty()
                               ? nullptr
                               : ref.object->floatElements.data() + ref.offset;
        int *intData = ref.object->intElements.empty()
                           ? nullptr
                           : ref.object->intElements.data() + ref.offset;
        const int n = wantsFloat ? runtimeGetFArray(this, floatData)
                                 : runtimeGetArray(this, intData);
        runtimeArrayCapacity_ = 0;
        throwRuntimeErrorIfAny();
        return Value::fromInt(n);
    }

    if (name == "putint" || name == "putch" || name == "putfloat") {
        if (args.size() != 1) {
            throw std::runtime_error(name + " expects 1 argument");
        }
        if (name == "putint") {
            output_ << requireInt(args[0]);
        } else if (name == "putch") {
            output_.put(static_cast<char>(requireInt(args[0])));
        } else {
            appendFloatHex(requireFloat(args[0]));
        }
        return Value::fromInt(0);
    }

    if (name == "putarray" || name == "putfarray") {
        if (args.size() != 2) {
            throw std::runtime_error(name + " expects 2 arguments");
        }
        const int n = requireInt(args[0]);
        ArrayRef ref = requireArray(args[1]);
        const bool wantsFloat = name == "putfarray";
        if ((ref.object->type == ScalarType::Float) != wantsFloat) {
            throw std::runtime_error(name + " argument type mismatch");
        }
        if (n < 0 ||
            static_cast<std::size_t>(n) > arrayStorageSize(ref) - ref.offset) {
            throw std::runtime_error(name + " length exceeds array storage");
        }
        output_ << n << ":";
        if (wantsFloat) {
            const float *data = ref.object->floatElements.empty()
                                    ? nullptr
                                    : ref.object->floatElements.data() + ref.offset;
            for (int i = 0; i < n; ++i) {
                output_ << " ";
                appendFloatHex(data[i]);
            }
        } else {
            const int *data = ref.object->intElements.empty()
                                  ? nullptr
                                  : ref.object->intElements.data() + ref.offset;
            for (int i = 0; i < n; ++i) {
                output_ << " " << data[i];
            }
        }
        output_ << '\n';
        return Value::fromInt(0);
    }

    if (name == "putf") {
        if (args.empty() || args[0].kind != Value::Kind::String) {
            throw std::runtime_error("putf expects a format string");
        }
        appendPutf(args[0].stringValue,
                   std::vector<Value>(args.begin() + 1, args.end()));
        return Value::fromInt(0);
    }

    if (name == "starttime" || name == "stoptime" || name == "_sysy_starttime" ||
        name == "_sysy_stoptime") {
        const bool takesLine = name[0] == '_';
        if (takesLine ? args.size() != 1 : !args.empty()) {
            throw std::runtime_error(name +
                                     (takesLine ? " expects 1 argument"
                                                : " expects 0 arguments"));
        }

        const int lineno = takesLine ? requireInt(args[0]) : 0;
        if (name == "starttime" || name == "_sysy_starttime") {
            startTimer(lineno);
        } else {
            stopTimer(lineno);
        }
        return Value::fromInt(0);
    }

    auto astFound = astFunctions_.find(name);
    if (astFound != astFunctions_.end()) {
        return callAstFunction(*astFound->second, args);
    }

    throw std::runtime_error("undefined function: " + name);
}

Interpreter::Value Interpreter::callAstFunction(const ast::FunctionDecl &function,
                                                const std::vector<Value> &args) {
    if (function.params.size() != args.size()) {
        throw std::runtime_error("argument count mismatch when calling: " + function.name);
    }

    const bool memoizable = isMemoizableFunction(function);
    std::string memoKey;
    if (memoizable) {
        memoKey = buildMemoKey(args);
        auto &cache = memoizedCalls_[&function];
        auto found = cache.find(memoKey);
        if (found != cache.end()) {
            return found->second;
        }
    }

    pushScope();
    try {
        for (std::size_t i = 0; i < function.params.size(); ++i) {
            const ast::ParamDecl &param = *function.params[i];
            const ScalarType paramType = scalarTypeOf(param.type.base);
            if (!param.type.isPointer) {
                declareScalar(param, paramType, args[i], false);
                continue;
            }

            ArrayRef ref = requireArray(args[i]);
            if (ref.object->type != paramType) {
                throw std::runtime_error("array parameter element type mismatch: " +
                                         param.name);
            }

            std::vector<int> tailDims = dimensionsOf(param.type);
            const std::size_t available = arrayStorageSize(ref) - ref.offset;
            if (tailDims.empty()) {
                ref.dims = {static_cast<int>(available)};
            } else {
                const std::size_t tailSize = aggregateSize(tailDims, 0);
                if (tailSize == 0 || available % tailSize != 0) {
                    throw std::runtime_error("array parameter dimension mismatch: " +
                                             param.name);
                }
                ref.dims.clear();
                ref.dims.push_back(static_cast<int>(available / tailSize));
                ref.dims.insert(ref.dims.end(), tailDims.begin(), tailDims.end());
            }
            declareArrayRef(param, ref);
        }

        if (function.body != nullptr) {
            executeStmt(*function.body);
        }
        popScope();
        Value result = Value::fromInt(0);
        if (memoizable) {
            memoizedCalls_[&function].emplace(std::move(memoKey), result);
        }
        return result;
    } catch (const ReturnSignal &signal) {
        popScope();
        Value result = function.returnType.base == ast::BuiltinType::Void
                           ? Value::fromInt(0)
                           : convertValue(signal.value, scalarTypeOf(function.returnType.base));
        if (memoizable) {
            memoizedCalls_[&function].emplace(std::move(memoKey), result);
        }
        return result;
    } catch (...) {
        popScope();
        throw;
    }
}

std::size_t Interpreter::aggregateSize(const std::vector<int> &dims, std::size_t level) const {
    std::size_t result = 1;
    for (std::size_t i = level; i < dims.size(); ++i) {
        result *= static_cast<std::size_t>(dims[i]);
    }
    return result;
}

std::vector<std::size_t> Interpreter::stridesOf(const std::vector<int> &dims) const {
    std::vector<std::size_t> strides(dims.size(), 1);
    std::size_t stride = 1;
    for (std::size_t i = dims.size(); i > 0; --i) {
        strides[i - 1] = stride;
        stride *= static_cast<std::size_t>(dims[i - 1]);
    }
    return strides;
}

Interpreter::Value Interpreter::arrayElementValue(const ArrayRef &ref,
                                                  std::size_t linearIndex) const {
    if (!ref.object) {
        throw std::runtime_error("value is not an array");
    }
    const std::size_t index = ref.offset + linearIndex;
    if (ref.object->type == ScalarType::Int) {
        if (index >= ref.object->intElements.size()) {
            throw std::runtime_error("array access out of range");
        }
        return Value::fromInt(ref.object->intElements[index]);
    }
    if (index >= ref.object->floatElements.size()) {
        throw std::runtime_error("array access out of range");
    }
    return Value::fromFloat(ref.object->floatElements[index]);
}

void Interpreter::setArrayElement(const ArrayRef &ref, std::size_t linearIndex,
                                  const Value &value) {
    if (ref.isConst) {
        throw std::runtime_error("cannot assign to const array");
    }
    if (ref.object->type == ScalarType::Int) {
        intArrayElement(ref, linearIndex) = requireInt(value);
    } else {
        floatArrayElement(ref, linearIndex) = requireFloat(value);
    }
}

int &Interpreter::intArrayElement(const ArrayRef &ref, std::size_t linearIndex) {
    if (!ref.object || ref.object->type != ScalarType::Int) {
        throw std::runtime_error("int array required");
    }
    const std::size_t index = ref.offset + linearIndex;
    if (index >= ref.object->intElements.size()) {
        throw std::runtime_error("array access out of range");
    }
    return ref.object->intElements[index];
}

float &Interpreter::floatArrayElement(const ArrayRef &ref, std::size_t linearIndex) {
    if (!ref.object || ref.object->type != ScalarType::Float) {
        throw std::runtime_error("float array required");
    }
    const std::size_t index = ref.offset + linearIndex;
    if (index >= ref.object->floatElements.size()) {
        throw std::runtime_error("array access out of range");
    }
    return ref.object->floatElements[index];
}

int Interpreter::requireInt(const Value &value) const {
    if (value.kind == Value::Kind::Int) {
        return value.intValue;
    }
    if (value.kind == Value::Kind::Float) {
        return static_cast<int>(value.floatValue);
    }
    throw std::runtime_error("integer value required");
}

float Interpreter::requireFloat(const Value &value) const {
    if (value.kind == Value::Kind::Float) {
        return value.floatValue;
    }
    if (value.kind == Value::Kind::Int) {
        return static_cast<float>(value.intValue);
    }
    throw std::runtime_error("float value required");
}

bool Interpreter::isTruthy(const Value &value) const {
    if (value.kind == Value::Kind::Float) {
        return value.floatValue != 0.0f;
    }
    return requireInt(value) != 0;
}

Interpreter::ArrayRef Interpreter::requireArray(const Value &value) const {
    if (value.kind != Value::Kind::Array) {
        throw std::runtime_error("array value required");
    }
    return value.array;
}

Interpreter::Value Interpreter::convertValue(const Value &value, ScalarType type) const {
    if (type == ScalarType::Int) {
        return Value::fromInt(requireInt(value));
    }
    return Value::fromFloat(requireFloat(value));
}

std::size_t Interpreter::arrayStorageSize(const ArrayRef &ref) const {
    if (!ref.object) {
        throw std::runtime_error("value is not an array");
    }
    if (ref.object->type == ScalarType::Int) {
        return ref.object->intElements.size();
    }
    return ref.object->floatElements.size();
}

void Interpreter::executeDecl(const ast::Decl &decl) {
    if (const auto *var = dynamic_cast<const ast::VarDecl *>(&decl)) {
        executeVarDecl(*var);
    }
}

void Interpreter::executeVarDecl(const ast::VarDecl &decl) {
    const ScalarType declType = scalarTypeOf(decl.type.base);
    if (decl.type.isArray()) {
        const std::vector<int> dims = dimensionsOf(decl.type);
        declareArray(decl, dims, buildInitValues(dims, decl.initializer.get()),
                     decl.type.isConst, declType);
        return;
    }

    Value value = Value::fromInt(0);
    if (decl.initializer != nullptr) {
        value = evalExpr(*decl.initializer);
    }
    declareScalar(decl, declType, value, decl.type.isConst);
}

void Interpreter::executeStmt(const ast::Stmt &stmt) {
    if (const auto *compound = dynamic_cast<const ast::CompoundStmt *>(&stmt)) {
        pushScope();
        try {
            for (const std::unique_ptr<ast::Stmt> &child : compound->statements) {
                executeStmt(*child);
            }
            popScope();
        } catch (...) {
            popScope();
            throw;
        }
        return;
    }

    if (const auto *declStmt = dynamic_cast<const ast::DeclStmt *>(&stmt)) {
        for (const std::unique_ptr<ast::VarDecl> &decl : declStmt->declarations) {
            executeVarDecl(*decl);
        }
        return;
    }

    if (dynamic_cast<const ast::NullStmt *>(&stmt) != nullptr) {
        return;
    }

    if (const auto *ifStmt = dynamic_cast<const ast::IfStmt *>(&stmt)) {
        const bool condition =
            isIntScalarExpr(*ifStmt->condition) ? evalInt(*ifStmt->condition) != 0
                                                : isTruthy(evalExpr(*ifStmt->condition));
        if (condition) {
            if (ifStmt->thenBranch != nullptr) {
                executeStmt(*ifStmt->thenBranch);
            }
        } else if (ifStmt->elseBranch != nullptr) {
            executeStmt(*ifStmt->elseBranch);
        }
        return;
    }

    if (const auto *whileStmt = dynamic_cast<const ast::WhileStmt *>(&stmt)) {
        while (isIntScalarExpr(*whileStmt->condition)
                   ? evalInt(*whileStmt->condition) != 0
                   : isTruthy(evalExpr(*whileStmt->condition))) {
            try {
                if (whileStmt->body != nullptr) {
                    executeStmt(*whileStmt->body);
                }
            } catch (const ContinueSignal &) {
                continue;
            } catch (const BreakSignal &) {
                break;
            }
        }
        return;
    }

    if (dynamic_cast<const ast::BreakStmt *>(&stmt) != nullptr) {
        throw BreakSignal{};
    }

    if (dynamic_cast<const ast::ContinueStmt *>(&stmt) != nullptr) {
        throw ContinueSignal{};
    }

    if (const auto *returnStmt = dynamic_cast<const ast::ReturnStmt *>(&stmt)) {
        Value value = Value::fromInt(0);
        if (returnStmt->value != nullptr) {
            value = evalExpr(*returnStmt->value);
        }
        throw ReturnSignal{value};
    }

    if (const auto *expr = dynamic_cast<const ast::Expr *>(&stmt)) {
        evalExpr(*expr);
        return;
    }

    throw std::runtime_error("unsupported AST statement");
}

Interpreter::Value Interpreter::evalExpr(const ast::Expr &expr) {
    if (const auto *literal = dynamic_cast<const ast::IntegerLiteral *>(&expr)) {
        return Value::fromInt(parseIntLiteral(literal->text));
    }

    if (const auto *literal = dynamic_cast<const ast::FloatLiteral *>(&expr)) {
        return Value::fromFloat(parseFloatLiteral(literal->text));
    }

    if (const auto *literal = dynamic_cast<const ast::StringLiteral *>(&expr)) {
        return Value::fromString(parseStringLiteral(literal->text));
    }

    if (const auto *cast = dynamic_cast<const ast::ImplicitCastExpr *>(&expr)) {
        switch (cast->kind) {
        case ast::CastKind::LValueToRValue:
        case ast::CastKind::ArrayToPointerDecay:
            return evalExpr(*cast->subExpr);
        case ast::CastKind::IntToFloat:
        case ast::CastKind::FloatToInt:
            return convertValue(evalExpr(*cast->subExpr),
                                scalarTypeOf(cast->targetType.base));
        case ast::CastKind::IntToBool:
        case ast::CastKind::FloatToBool:
            return Value::fromInt(isTruthy(evalExpr(*cast->subExpr)) ? 1 : 0);
        }
    }

    if (const auto *paren = dynamic_cast<const ast::ParenExpr *>(&expr)) {
        return evalExpr(*paren->subExpr);
    }

    if (const auto *ref = dynamic_cast<const ast::DeclRefExpr *>(&expr)) {
        Variable &variable = resolveVariable(*ref);
        if (!variable.isArray()) {
            if (variable.type == ScalarType::Int) {
                return Value::fromInt(variable.value);
            }
            return Value::fromFloat(variable.floatValue);
        }
        return Value::fromArray(evalLValRef(*ref));
    }

    if (dynamic_cast<const ast::ArraySubscriptExpr *>(&expr) != nullptr) {
        ArrayRef ref = evalLValRef(expr);
        if (ref.dims.empty()) {
            return arrayElementValue(ref, 0);
        }
        return Value::fromArray(std::move(ref));
    }

    if (const auto *unary = dynamic_cast<const ast::UnaryOperator *>(&expr)) {
        const Value value = evalExpr(*unary->operand);
        switch (unary->opcode) {
        case ast::UnaryOpcode::Plus:
            return value;
        case ast::UnaryOpcode::Minus:
            if (value.kind == Value::Kind::Float) {
                return Value::fromFloat(-value.floatValue);
            }
            return Value::fromInt(-requireInt(value));
        case ast::UnaryOpcode::LogicalNot:
            return Value::fromInt(isTruthy(value) ? 0 : 1);
        }
    }

    if (const auto *binary = dynamic_cast<const ast::BinaryOperator *>(&expr)) {
        if (binary->opcode == ast::BinaryOpcode::Assign) {
            const Value rhs = evalExpr(*binary->rhs);

            if (const auto *ref = dynamic_cast<const ast::DeclRefExpr *>(binary->lhs.get())) {
                Variable &variable = resolveVariable(*ref);
                if (!variable.isArray()) {
                    if (variable.isConst) {
                        throw std::runtime_error("cannot assign to const: " + ref->name);
                    }
                    const ScalarType variableType = variable.type;
                    const Value value = convertValue(rhs, variableType);
                    if (variableType == ScalarType::Int) {
                        variable.value = value.intValue;
                    } else {
                        variable.floatValue = value.floatValue;
                    }
                    return value;
                }
            }

            ArrayRef ref = evalLValRef(*binary->lhs);
            if (!ref.dims.empty()) {
                throw std::runtime_error("cannot assign to array slice");
            }
            const ScalarType elementType = ref.object->type;
            const Value value = convertValue(rhs, elementType);
            setArrayElement(ref, 0, value);
            return value;
        }

        switch (binary->opcode) {
        case ast::BinaryOpcode::LogicalAnd:
            if (!isTruthy(evalExpr(*binary->lhs))) {
                return Value::fromInt(0);
            }
            return Value::fromInt(isTruthy(evalExpr(*binary->rhs)) ? 1 : 0);
        case ast::BinaryOpcode::LogicalOr:
            if (isTruthy(evalExpr(*binary->lhs))) {
                return Value::fromInt(1);
            }
            return Value::fromInt(isTruthy(evalExpr(*binary->rhs)) ? 1 : 0);
        default:
            break;
        }

        const Value lhs = evalExpr(*binary->lhs);
        const Value rhs = evalExpr(*binary->rhs);
        const bool useFloat = lhs.kind == Value::Kind::Float ||
                              rhs.kind == Value::Kind::Float;

        switch (binary->opcode) {
        case ast::BinaryOpcode::Mul:
        case ast::BinaryOpcode::Div:
        case ast::BinaryOpcode::Mod:
        case ast::BinaryOpcode::Add:
        case ast::BinaryOpcode::Sub: {
            if (useFloat) {
                if (binary->opcode == ast::BinaryOpcode::Mod) {
                    throw std::runtime_error("float modulo is invalid");
                }
                const float l = requireFloat(lhs);
                const float r = requireFloat(rhs);
                switch (binary->opcode) {
                case ast::BinaryOpcode::Mul:
                    return Value::fromFloat(l * r);
                case ast::BinaryOpcode::Div:
                    return Value::fromFloat(l / r);
                case ast::BinaryOpcode::Add:
                    return Value::fromFloat(l + r);
                case ast::BinaryOpcode::Sub:
                    return Value::fromFloat(l - r);
                default:
                    break;
                }
            }

            const int l = requireInt(lhs);
            const int r = requireInt(rhs);
            switch (binary->opcode) {
            case ast::BinaryOpcode::Mul:
                return Value::fromInt(l * r);
            case ast::BinaryOpcode::Div:
                return Value::fromInt(l / r);
            case ast::BinaryOpcode::Mod:
                return Value::fromInt(l % r);
            case ast::BinaryOpcode::Add:
                return Value::fromInt(l + r);
            case ast::BinaryOpcode::Sub:
                return Value::fromInt(l - r);
            default:
                break;
            }
            break;
        }
        default:
            break;
        }

        if (useFloat) {
            const float l = requireFloat(lhs);
            const float r = requireFloat(rhs);
            switch (binary->opcode) {
            case ast::BinaryOpcode::Less:
                return Value::fromInt(l < r ? 1 : 0);
            case ast::BinaryOpcode::Greater:
                return Value::fromInt(l > r ? 1 : 0);
            case ast::BinaryOpcode::LessEqual:
                return Value::fromInt(l <= r ? 1 : 0);
            case ast::BinaryOpcode::GreaterEqual:
                return Value::fromInt(l >= r ? 1 : 0);
            case ast::BinaryOpcode::Equal:
                return Value::fromInt(l == r ? 1 : 0);
            case ast::BinaryOpcode::NotEqual:
                return Value::fromInt(l != r ? 1 : 0);
            default:
                break;
            }
        } else {
            const int l = requireInt(lhs);
            const int r = requireInt(rhs);
            switch (binary->opcode) {
            case ast::BinaryOpcode::Less:
                return Value::fromInt(l < r ? 1 : 0);
            case ast::BinaryOpcode::Greater:
                return Value::fromInt(l > r ? 1 : 0);
            case ast::BinaryOpcode::LessEqual:
                return Value::fromInt(l <= r ? 1 : 0);
            case ast::BinaryOpcode::GreaterEqual:
                return Value::fromInt(l >= r ? 1 : 0);
            case ast::BinaryOpcode::Equal:
                return Value::fromInt(l == r ? 1 : 0);
            case ast::BinaryOpcode::NotEqual:
                return Value::fromInt(l != r ? 1 : 0);
            default:
                break;
            }
        }
    }

    if (const auto *call = dynamic_cast<const ast::CallExpr *>(&expr)) {
        std::vector<Value> args;
        args.reserve(call->arguments.size());
        for (const std::unique_ptr<ast::Expr> &argument : call->arguments) {
            args.push_back(evalExpr(*argument));
        }
        if (call->calleeDecl != nullptr) {
            return callAstFunction(*call->calleeDecl, args);
        }
        return callFunction(call->callee, args);
    }

    if (dynamic_cast<const ast::InitListExpr *>(&expr) != nullptr) {
        throw std::runtime_error(
            "unsupported SysY feature in interpreter: brace initializer for scalar");
    }

    throw std::runtime_error("unsupported AST expression");
}

int Interpreter::evalInt(const ast::Expr &expr) {
    if (!isIntScalarExpr(expr)) {
        return requireInt(evalExpr(expr));
    }

    if (const auto *literal = dynamic_cast<const ast::IntegerLiteral *>(&expr)) {
        return parseIntLiteral(literal->text);
    }

    if (const auto *paren = dynamic_cast<const ast::ParenExpr *>(&expr)) {
        return evalInt(*paren->subExpr);
    }

    if (const auto *ref = dynamic_cast<const ast::DeclRefExpr *>(&expr)) {
        Variable &variable = resolveVariable(*ref);
        if (variable.isArray() || variable.type != ScalarType::Int) {
            return requireInt(evalExpr(expr));
        }
        return variable.value;
    }

    if (dynamic_cast<const ast::ArraySubscriptExpr *>(&expr) != nullptr) {
        ArrayRef ref = evalLValRef(expr);
        if (!ref.dims.empty() || ref.object == nullptr || ref.object->type != ScalarType::Int) {
            return requireInt(evalExpr(expr));
        }
        return intArrayElement(ref, 0);
    }

    if (const auto *unary = dynamic_cast<const ast::UnaryOperator *>(&expr)) {
        if (!isIntScalarExpr(*unary->operand)) {
            return requireInt(evalExpr(expr));
        }
        const int value = evalInt(*unary->operand);
        switch (unary->opcode) {
        case ast::UnaryOpcode::Plus:
            return value;
        case ast::UnaryOpcode::Minus:
            return -value;
        case ast::UnaryOpcode::LogicalNot:
            return value == 0 ? 1 : 0;
        }
    }

    if (const auto *binary = dynamic_cast<const ast::BinaryOperator *>(&expr)) {
        if (binary->opcode == ast::BinaryOpcode::Assign) {
            return requireInt(evalExpr(expr));
        }
        if (!isIntScalarExpr(*binary->lhs) || !isIntScalarExpr(*binary->rhs)) {
            return requireInt(evalExpr(expr));
        }
        switch (binary->opcode) {
        case ast::BinaryOpcode::LogicalAnd:
            return evalInt(*binary->lhs) != 0 && evalInt(*binary->rhs) != 0 ? 1 : 0;
        case ast::BinaryOpcode::LogicalOr:
            return evalInt(*binary->lhs) != 0 || evalInt(*binary->rhs) != 0 ? 1 : 0;
        default:
            break;
        }

        const int l = evalInt(*binary->lhs);
        const int r = evalInt(*binary->rhs);
        switch (binary->opcode) {
        case ast::BinaryOpcode::Mul:
            return l * r;
        case ast::BinaryOpcode::Div:
            return l / r;
        case ast::BinaryOpcode::Mod:
            return l % r;
        case ast::BinaryOpcode::Add:
            return l + r;
        case ast::BinaryOpcode::Sub:
            return l - r;
        case ast::BinaryOpcode::Less:
            return l < r ? 1 : 0;
        case ast::BinaryOpcode::Greater:
            return l > r ? 1 : 0;
        case ast::BinaryOpcode::LessEqual:
            return l <= r ? 1 : 0;
        case ast::BinaryOpcode::GreaterEqual:
            return l >= r ? 1 : 0;
        case ast::BinaryOpcode::Equal:
            return l == r ? 1 : 0;
        case ast::BinaryOpcode::NotEqual:
            return l != r ? 1 : 0;
        case ast::BinaryOpcode::LogicalAnd:
        case ast::BinaryOpcode::LogicalOr:
            break;
        default:
            break;
        }
    }

    if (const auto *call = dynamic_cast<const ast::CallExpr *>(&expr)) {
        return requireInt(evalExpr(*call));
    }

    return requireInt(evalExpr(expr));
}

bool Interpreter::isIntScalarExpr(const ast::Expr &expr) const {
    return expr.hasType && expr.inferredType.base == ast::BuiltinType::Int &&
           !expr.inferredType.isPointer && expr.inferredType.shape.empty();
}

Interpreter::ArrayRef Interpreter::evalLValRef(const ast::Expr &expr) {
    const ast::Expr *current = &expr;
    while (const auto *paren = dynamic_cast<const ast::ParenExpr *>(current)) {
        current = paren->subExpr.get();
    }
    while (const auto *cast = dynamic_cast<const ast::ImplicitCastExpr *>(current)) {
        if (cast->kind != ast::CastKind::ArrayToPointerDecay &&
            cast->kind != ast::CastKind::LValueToRValue) {
            break;
        }
        current = cast->subExpr.get();
        while (const auto *paren = dynamic_cast<const ast::ParenExpr *>(current)) {
            current = paren->subExpr.get();
        }
    }
    std::array<const ast::Expr *, 64> indices{};
    std::size_t indexCount = 0;
    while (const auto *subscript =
               dynamic_cast<const ast::ArraySubscriptExpr *>(current)) {
        if (indexCount >= indices.size()) {
            throw std::runtime_error("too many indices");
        }
        indices[indexCount++] = subscript->index.get();
        current = subscript->base.get();
        while (const auto *paren = dynamic_cast<const ast::ParenExpr *>(current)) {
            current = paren->subExpr.get();
        }
        while (const auto *cast = dynamic_cast<const ast::ImplicitCastExpr *>(current)) {
            if (cast->kind != ast::CastKind::ArrayToPointerDecay &&
                cast->kind != ast::CastKind::LValueToRValue) {
                break;
            }
            current = cast->subExpr.get();
            while (const auto *paren = dynamic_cast<const ast::ParenExpr *>(current)) {
                current = paren->subExpr.get();
            }
        }
    }

    const auto *root = dynamic_cast<const ast::DeclRefExpr *>(current);
    if (root == nullptr) {
        throw std::runtime_error("expression is not an lvalue");
    }

    Variable &variable = resolveVariable(*root);
    if (!variable.isArray()) {
        if (indexCount != 0) {
            throw std::runtime_error("scalar indexed as array: " + root->name);
        }
        return ArrayRef{};
    }

    if (indexCount > variable.dims.size()) {
        throw std::runtime_error("too many indices: " + root->name);
    }

    std::size_t offset = variable.offset;
    for (std::size_t i = 0; i < indexCount; ++i) {
        const ast::Expr *indexExpr = indices[indexCount - i - 1];
        const int index = evalInt(*indexExpr);
        if (index < 0 || index >= variable.dims[i]) {
            throw std::runtime_error("array index out of range: " + root->name);
        }
        offset += static_cast<std::size_t>(index) * variable.strides[i];
    }

    ArrayRef result;
    result.object = variable.array;
    result.offset = offset;
    result.isConst = variable.isConst;
    result.dims.assign(variable.dims.begin() + static_cast<std::ptrdiff_t>(indexCount),
                       variable.dims.end());
    return result;
}

std::vector<int> Interpreter::dimensionsOf(ast::Type type) const {
    std::vector<int> dims;
    dims.reserve(type.shape.size());
    for (long long dimension : type.shape) {
        if (dimension <= 0) {
            throw std::runtime_error("array dimension must be positive");
        }
        dims.push_back(static_cast<int>(dimension));
    }
    return dims;
}

std::vector<Interpreter::Value> Interpreter::buildInitValues(
    const std::vector<int> &dims, const ast::Expr *initializer) {
    std::vector<Value> values(aggregateSize(dims, 0), Value::fromInt(0));
    if (initializer != nullptr) {
        fillInitValue(*initializer, dims, 0, 0, values);
    }
    return values;
}

std::size_t Interpreter::fillInitValue(const ast::Expr &initializer,
                                       const std::vector<int> &dims,
                                       std::size_t level, std::size_t start,
                                       std::vector<Value> &values) {
    const auto *list = dynamic_cast<const ast::InitListExpr *>(&initializer);
    if (list == nullptr) {
        if (start < values.size()) {
            values[start] = evalExpr(initializer);
        }
        return start + 1;
    }

    if (level >= dims.size()) {
        return start;
    }

    const std::size_t total = aggregateSize(dims, 0);
    const std::size_t currentSize = aggregateSize(dims, level);
    const std::size_t aggregateEnd = std::min(total, start + currentSize);
    std::size_t pos = start;
    for (std::size_t i = 0; i < list->values.size(); ++i) {
        const std::unique_ptr<ast::Expr> &value = list->values[i];
        const bool hasOffset = list->valueOffsets.size() == list->values.size();
        const std::size_t valuePos =
            hasOffset ? start + list->valueOffsets[i] : pos;
        if (valuePos >= aggregateEnd) {
            break;
        }

        if (dynamic_cast<const ast::InitListExpr *>(value.get()) == nullptr) {
            pos = fillInitValue(*value, dims, level + 1, valuePos, values);
            continue;
        }

        const std::size_t childLevel =
            initializerChildLevel(dims, level, start, valuePos);
        if (childLevel >= dims.size()) {
            continue;
        }
        pos = fillInitValue(*value, dims, childLevel, valuePos, values);
    }
    return start + currentSize;
}

std::size_t Interpreter::initializerChildLevel(const std::vector<int> &dims,
                                               std::size_t parentLevel,
                                               std::size_t parentStart,
                                               std::size_t pos) const {
    if (parentLevel + 1 >= dims.size()) {
        return dims.size();
    }

    const std::size_t relative = pos - parentStart;
    for (std::size_t level = parentLevel + 1; level < dims.size(); ++level) {
        const std::size_t size = aggregateSize(dims, level);
        if (size != 0 && relative % size == 0) {
            return level;
        }
    }
    return dims.size();
}

Interpreter::ScalarType Interpreter::scalarTypeOf(ast::BuiltinType type) const {
    if (type == ast::BuiltinType::Float) {
        return ScalarType::Float;
    }
    if (type == ast::BuiltinType::Int) {
        return ScalarType::Int;
    }
    throw std::runtime_error("int or float type required");
}

int Interpreter::parseIntLiteral(const std::string &text) {
    return static_cast<int>(std::stoll(text, nullptr, 0));
}

float Interpreter::parseFloatLiteral(const std::string &text) {
    return std::strtof(text.c_str(), nullptr);
}

std::string Interpreter::parseStringLiteral(const std::string &text) {
    std::string result;
    for (std::size_t i = 1; i + 1 < text.size(); ++i) {
        char ch = text[i];
        if (ch != '\\' || i + 1 >= text.size() - 1) {
            result.push_back(ch);
            continue;
        }

        char escaped = text[++i];
        switch (escaped) {
        case 'n':
            result.push_back('\n');
            break;
        case 't':
            result.push_back('\t');
            break;
        case 'r':
            result.push_back('\r');
            break;
        case 'a':
            result.push_back('\a');
            break;
        case 'b':
            result.push_back('\b');
            break;
        case 'f':
            result.push_back('\f');
            break;
        case 'v':
            result.push_back('\v');
            break;
        case '\\':
        case '\'':
        case '"':
        case '?':
            result.push_back(escaped);
            break;
        default:
            if (escaped >= '0' && escaped <= '7') {
                int value = escaped - '0';
                int consumed = 1;
                while (consumed < 3 && i + 1 < text.size() - 1 &&
                       text[i + 1] >= '0' && text[i + 1] <= '7') {
                    value = value * 8 + (text[++i] - '0');
                    ++consumed;
                }
                result.push_back(static_cast<char>(value));
            } else if (escaped == 'x') {
                int value = 0;
                while (i + 1 < text.size() - 1 &&
                       std::isxdigit(static_cast<unsigned char>(text[i + 1]))) {
                    char hex = text[++i];
                    value *= 16;
                    if (hex >= '0' && hex <= '9') {
                        value += hex - '0';
                    } else {
                        value += std::tolower(static_cast<unsigned char>(hex)) - 'a' + 10;
                    }
                }
                result.push_back(static_cast<char>(value));
            } else {
                result.push_back(escaped);
            }
            break;
        }
    }
    return result;
}
