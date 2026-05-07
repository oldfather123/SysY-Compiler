#include "Common.h"
#include "Values.h"
#include "Scopes.h"

namespace ir_builder {
    /// @brief Variable, can be global or local, constant or non-constant
    class DeclVar {
    protected:
        DataType _dataType;
        String _name;
        bool _isFuncArg;
        bool _isGlobal;
        ir::RegPtr _reg;

    public: /* constructors */
        DeclVar(const DataType &dataType, const String &name, const bool &isFuncArg, const bool &isGlobal) : _dataType(dataType), _name(name), _isFuncArg(isFuncArg), _isGlobal(isGlobal) { }
        static Ptr<DeclVar> create(const DataType &dataType, const String &name, const bool &isFuncArg = false, const bool &isGlobal = false) {
            return makePtr<DeclVar>(dataType, name, isFuncArg, isGlobal);
        }

    public: /* properties */
        const DataType &dataType() const { return _dataType; }
        String &name() { return _name; }
        bool &isGlobal() { return _isGlobal; }
        bool &isFuncArg() { return _isFuncArg; }
        ir::RegPtr &reg() { return _reg; }

    public: /* methods */
    };

    class SymbolTable {
    private:
        std::unordered_map<String, Ptr<DeclVar>> _varMap;
        std::unordered_map<String, ir::FuncPtr> _funcMap;
        Ptr<SymbolTable> _outer;

    public: /* constructors */
        SymbolTable(const Ptr<SymbolTable> &outer) : _outer(outer) { }
        static Ptr<SymbolTable> create(const Ptr<SymbolTable> &outer) {
            return makePtr<SymbolTable>(outer);
        }

    public: /* properties */
        const Ptr<SymbolTable> &outer() const { return _outer; }

    public: /* methods */
        void declareVariable(const Ptr<DeclVar> &variable) {
            auto varName = variable->name();
            excassert(!_varMap[varName] && !_funcMap[varName], "Identifier `" + varName + "` already declared in this scope");
            _varMap[varName] = variable;
        }
        void declareFunction(const ir::FuncPtr &function) {
            auto funcName = function->name();
            excassert(!_funcMap[funcName] && !_varMap[funcName], "Identifier `" + funcName + "` already declared in this scope");
            _funcMap[funcName] = function;
        }
        const Ptr<DeclVar> lookupVariable(const String &varName) {
            auto variable = _varMap[varName];
            if (variable) {
                return variable;
            } else if (_funcMap[varName]) {
                excassert(false, "Identifier `" + varName + "` has been declared in this scope, but as a function");
            } else if (_outer) {
                return _outer->lookupVariable(varName);
            }
            excassert(variable != nullptr, "`" + varName + "` has not been declared in this scope");
            return variable;
        }
        const ir::FuncPtr lookupFunction(const String &funcName) {
            auto function = _funcMap[funcName];
            if (function) {
                return function;
            } else if (_varMap[funcName]) {
                excassert(false, "Identifier `" + funcName + "` has been declared in this scope, but as a variable");
            } else if (_outer) {
                return _outer->lookupFunction(funcName);
            }
            excassert(function != nullptr, "`" + funcName + "` has not been declared in this scope");
            return function;
        }
    };

    class Builder {
    private:
        class Loop {
        public:
            ir::BBPtr continueTarget = nullptr;
            ir::BBPtr breakTarget = nullptr;
            Ptr<Loop> outerLoop = nullptr;
            Loop(ir::BBPtr continueTarget, ir::BBPtr breakTarget, Ptr<Loop> outerLoop) : continueTarget(continueTarget), breakTarget(breakTarget), outerLoop(outerLoop) { }
        };
        Ptr<ir::Module> _currentModule = nullptr;
        ir::FuncPtr _currentFunc = nullptr;
        ir::RegPtr _currentRetReg = nullptr;
        ir::BBPtr _currentBB = nullptr;
        Ptr<SymbolTable> _symbolTable = nullptr;
        Ptr<Loop> _currentLoop = nullptr;

    public: /* constructors */
        Builder() { }
        static Ptr<Builder> create() {
            return makePtr<Builder>();
        }

    public: /* properties */
        const Ptr<ir::Module> &currentModule() const { return _currentModule; }
        const ir::FuncPtr &currentFunc() const { return _currentFunc; }
        const ir::BBPtr &currentBB() const { return _currentBB; }
        const ir::BBPtr &currentContinueTarget() const { return _currentLoop->continueTarget; }
        const ir::BBPtr &currentBreakTarget() const { return _currentLoop->breakTarget; }

    public: /* methods */
        void declareFunction(const ir::FuncPtr &function) {
            _symbolTable->declareFunction(function);
        }
        void declareVariable(const Ptr<DeclVar> &variable) {
            _symbolTable->declareVariable(variable);
            if (_currentFunc->isGlobalInitFunc()) {
                variable->isGlobal() = true;
            }
        }
        const Ptr<DeclVar> lookupVariable(const String &varName) {
            return _symbolTable->lookupVariable(varName);
        }
        const ir::FuncPtr lookupFunction(const String &funcName) {
            return _symbolTable->lookupFunction(funcName);
        }
        void setGlobalInitFunc(const ir::FuncPtr &globalInitFunc) {
            _currentModule->_globalInitFunc = globalInitFunc;
            globalInitFunc->_parentModule = _currentModule;
        }
        ir::BBPtr newBB(const String &label) {
            String _label = label;
            auto bb = ir::BasicBlock::create(_currentFunc, _label);
            return bb;
        }
        void enterBB(const ir::BBPtr &bb) {
            _currentBB = bb;
            bb->_printOrder = ir::BasicBlock::_printOrderCounter++;
            if (bb == _currentFunc->exitBB()) {
                bb->_printOrder = INT_MAX;
            }
        }
        /// @brief Drop the basic block when encountering `break`, `continue` or `return`
        void dropBB() {
            if (!_currentBB) {
                return;
            }
            _currentBB = nullptr;
        }
        void uncondBr(const ir::BBPtr &unconditionalTarget) {
            if (!_currentBB) {
                return;
            }
            ir::BasicBlock::addUnconditionalEdge(_currentBB, unconditionalTarget);
        }
        void condBr(const ir::Value &condition, const ir::BBPtr &trueTarget, const ir::BBPtr &falseTarget) {
            if (!_currentBB) {
                return;
            }
            ir::BasicBlock::addConditionalEdge(_currentBB, trueTarget, condition, true);
            ir::BasicBlock::addConditionalEdge(_currentBB, falseTarget, condition, false);
        }
        void ret(const ir::Value &retVal) {
            if (!_currentBB) {
                return;
            }
            dbgassert(_currentRetReg != nullptr, "Return value register not set");
            appendInst(ir::MoveInst::create(_currentBB, _currentRetReg, retVal));
            ir::BasicBlock::addUnconditionalEdge(_currentBB, _currentFunc->exitBB());
        }
        void enterFunction(const ir::FuncPtr &function) {
            dropBB();
            _currentRetReg = ir::Register::create(function, function->retDataType(), "retval");
            _currentRetReg->isRet() = true;
            _currentFunc = function;
            function->_printOrder = ir::Function::_printOrderCounter++;
        }
        void exitFunction() {
            dbgassert(_currentBB != nullptr, "Current basic block is null");
            appendInst(ir::RetInst::create(_currentBB, _currentRetReg));
            _currentFunc = nullptr;
            dropBB();
            _currentRetReg = nullptr;
        }
        void enterModule(const Ptr<ir::Module> &module) {
            _currentModule = module;
        }
        void enterScope() {
            _symbolTable = SymbolTable::create(_symbolTable);
        }
        void exitScope() {
            _symbolTable = _symbolTable->outer();
        }
        void appendInst(const ir::InstPtr &inst) {
            if (_currentBB) {
                ir::Instruction::insertBefore(_currentBB->_exitInst, inst);
            }
        }
        /// @brief Set current loop continue/break target basic blocks
        void enterLoop(const ir::BBPtr continueTarget, const ir::BBPtr breakTarget) {
            _currentLoop = makePtr<Loop>(continueTarget, breakTarget, _currentLoop);
        }
        void exitLoop() {
            _currentLoop = _currentLoop->outerLoop;
        }
    };
} // namespace irbuilder
