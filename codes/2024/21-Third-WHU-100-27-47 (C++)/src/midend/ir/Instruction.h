#pragma once

#include "Common.h"
#include "Values.h"
#include "Graph.h"
#include "Scopes.h"
#include "CFG.h"
#include "DAG.h"

namespace ir {
    class BasicBlock;
    class Function;
    class DefInst;

    /// @brief Base class of instructions; abstract
    class Instruction : public GraphNode<Instruction, DAGEdge>, public IPrintOrderable<Instruction> {
        friend class Builder;
        friend class BasicBlock;

    protected:
        InstPtr _self = nullptr;

    public:
        enum Type {
            PseudoEntry,
            PseudoPlaceholder,
            Global,
            Move,
            Oper,
            Call,
            Load,
            Store,
            Alloc,
            Cast,
            GEP,
            Phi,
            Ret,
            Exit,
        };
        template <typename T>
        Ptr<T> as() { return castPtr<T>(_self); }
        template <typename T>
        Ptr<T> as() const { return castPtr<T>(_self); }
        template <typename T>
        bool is() const { return castPtr<const T>(_self) != nullptr; }

    protected:
        BBPtr _parentBB; // CAUSES LOOP REFERENCE
        Instruction::Type _instType;

        virtual void _init(const InstPtr &self, const BBPtr &parentBB);

    public: /* constructors */
        Instruction(const Instruction::Type &instType) : GraphNode(), _instType(instType) { }
        ~Instruction() {
            _parentBB.reset(); // BREAK THE LOOP REFERENCE
        }

    public: /* properties */
        const Instruction::Type &instType() const { return _instType; }
        const BBPtr &parentBB() const { return _parentBB; }

        virtual Set<RegPtrRef> regsWrittenRefs() = 0;
        virtual Set<RegPtrRef> regsReadRefs() = 0;

        Set<RegPtr> regsWritten() {
            Set<RegPtr> ret{};
            for (auto regWrittenRef : this->regsWrittenRefs()) {
                ret.insert(regWrittenRef.get());
            }
            return ret;
        }
        Set<RegPtr> regsRead() {
            Set<RegPtr> ret{};
            for (auto regReadRef : this->regsReadRefs()) {
                ret.insert(regReadRef.get());
            }
            return ret;
        }

        Set<Variable> mustDefVars();
        Set<Variable> mustUseVars();
        Set<Variable> mayDefVars();
        Set<Variable> mayUseVars();

    public: /* methods */
        static void insertAfter(const InstPtr &inst, const InstPtr &newInst);
        static void insertBefore(const InstPtr &inst, const InstPtr &newInst);
        static void replace(const InstPtr &inst, const InstPtr &newInst);
        static void remove(const InstPtr &inst);
        static void addTo(const InstPtr &inst, const BBPtr &bb);
        static InstPtr clone(const InstPtr &inst, const BBPtr &newBB);

        void replaceReg(RegPtrRef regRef, const Value &value);
        void recompute();

        virtual String toString(bool exprOnly) const = 0;
        String toString() const { return toString(false); }
    };

    // @brief Pseudo instruction, for entry or exit of a basic block
    class PseudoInst : public Instruction {
    private:

    public: /* constructors */
        PseudoInst(const Instruction::Type &instType) : Instruction(instType) {
            dbgassert(instType == Type::PseudoEntry || instType == Type::PseudoPlaceholder, "Invalid pseudo instruction type");
        }
        static Ptr<PseudoInst> create(const BBPtr &parentBB, const Instruction::Type &instType) {
            auto ret = makePtr<PseudoInst>(instType);
            ret->_init(ret, parentBB);
            return ret;
        }
        static Ptr<PseudoInst> createEntry(const BBPtr &parentBB) {
            return create(parentBB, Type::PseudoEntry);
        }
        static Ptr<PseudoInst> createPlaceholder(const BBPtr &parentBB) {
            return create(parentBB, Type::PseudoPlaceholder);
        }

    public: /* properties */
        Set<RegPtrRef> regsWrittenRefs() override {
            return Set<RegPtrRef>{};
        }
        Set<RegPtrRef> regsReadRefs() override {
            return Set<RegPtrRef>{};
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };

    /// @brief Instuction that defines a register; abstract
    class DefInst : public Instruction {
    protected:
        RegPtr _destReg;

    public: /* constructors */
        DefInst(const Instruction::Type &instType, const RegPtr &destReg) : Instruction(instType), _destReg(destReg) { }

    public: /* properties */
        /// @brief Destination register
        RegPtr &destReg() { return _destReg; }

        virtual Set<RegPtrRef> regsWrittenRefs() override {
            return Set<RegPtrRef>{&_destReg};
        }

    public: /* methods */
    };

    /// @brief Global variable declaration instruction
    class GlobalInst : public DefInst {
    private:
        Value _initConstVal;

    public: /* constructors */
        GlobalInst(const RegPtr &destReg, const Value &initConstVal) : DefInst(Type::Global, destReg), _initConstVal(initConstVal) {
            // dbgassert(_initConstVal.dataType().isConst(), "Global variable initialization value must be constant");
        }
        static Ptr<GlobalInst> create(const BBPtr &parentBB, const RegPtr &destReg, const Value &initVal) {
            auto ret = std::make_shared<GlobalInst>(destReg, initVal);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        Value &initConstVal() { return _initConstVal; }

        Set<RegPtrRef> regsReadRefs() override {
            return Set<RegPtrRef>{};
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };

    /// @brief Move instruction
    class MoveInst : public DefInst {
    private:
        Value _srcVal;

    public: /* constructors */
        MoveInst(const RegPtr &destReg, const Value &srcVal) : DefInst(Type::Move, destReg), _srcVal(srcVal) { }
        static Ptr<MoveInst> create(const BBPtr &parentBB, const RegPtr &destReg, const Value &srcVal) {
            auto ret = std::make_shared<MoveInst>(destReg, srcVal);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        Value &srcVal() { return _srcVal; }

        Set<RegPtrRef> regsReadRefs() override {
            return _srcVal.isRegister() ? Set<RegPtrRef>{&_srcVal} : Set<RegPtrRef>{};
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };

    /// @brief Operation instruction, assigns the operation result to a register
    class OperInst : public DefInst {
    public:
        enum class Operator {
            Shl,        // `<<`
            Shr,        // `>>`
            BitwiseAnd, // `&`
            BitwiseOr,  // `|`
            BitwiseXor, // `^`
            Xor,        // `^`
            Add,        // `+` (binary)
            Sub,        // `-` (binary)
            Mul,        // `*`
            Div,        // `/`
            Mod,        // `%`
            Eq,         // `==`
            Ne,         // `!=`
            Lt,         // `<`
            Ge,         // `>=`
            Not,        // `!`
            And,        // `&&`
            Or,         // `||`
        };
        static bool isUnary(const Operator &op) { return op == Operator::Not; }
        static bool isBinary(const Operator &op) { return !isUnary(op); }

    private:
        Operator _op;
        /// @brief Left-hand-side operand, unspecified if unary
        Value _lhs;
        /// @brief Right-hand-side operand, used as operand if unary
        Value _rhs;

    public: /* constructors */
        OperInst(const RegPtr &destReg, const Operator &op, const Value &lhs, const Value &rhs) : DefInst(Type::Oper, destReg), _op(op), _lhs(lhs), _rhs(rhs) { }
        static Ptr<OperInst> createUnary(const BBPtr &parentBB, const RegPtr &destReg, const Operator &op, const Value &rhs) {
            auto ret = std::make_shared<OperInst>(destReg, op, Value{}, rhs);
            ret->_init(ret, parentBB);
            return ret;
        }
        static Ptr<OperInst> createBinary(const BBPtr &parentBB, const RegPtr &destReg, const Operator &op, const Value &lhs, const Value &rhs) {
            auto ret = std::make_shared<OperInst>(destReg, op, lhs, rhs);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        /// @brief Operator of the operation
        Operator &op() { return _op; }
        /// @brief Left-hand-side operand of binary operation
        Value &lhs() {
            dbgassert(!isUnary(), "LHS property is only available for binary operations");
            return _lhs;
        }
        /// @brief Right-hand-side operand of unary/binary operation
        Value &rhs() {
            return _rhs;
        }

        Set<RegPtrRef> regsReadRefs() override {
            Set<RegPtrRef> ret{};
            if (_lhs.isRegister()) {
                ret.insert(&_lhs);
            }
            if (_rhs.isRegister()) {
                ret.insert(&_rhs);
            }
            return ret;
        }

    public: /* methods */
        String opToString() const;
        bool isUnary() const { return isUnary(_op); }
        bool isBinary() const { return isBinary(_op); }

        String toString(bool exprOnly = false) const override;
    };
    using Op = OperInst::Operator;

    /// @brief Function call instruction, assigns the return value to a register
    class CallInst : public DefInst {
    private: /* fields */
        FuncPtr _function;
        Vector<Value> _argList;

    public: /* constructors */
        CallInst(const RegPtr &destReg, const FuncPtr &function, const Vector<Value> &argList) : DefInst(Type::Call, destReg), _function(function), _argList(argList) { }
        static Ptr<CallInst> create(const BBPtr &parentBB, const RegPtr &destReg, const FuncPtr &function, const Vector<Value> &argList) {
            auto ret = std::make_shared<CallInst>(destReg, function, argList);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        FuncPtr &function() { return _function; }
        Vector<Value> &argList() { return _argList; }
        bool isTailCall() const;

        Set<RegPtrRef> regsReadRefs() override {
            Set<RegPtrRef> ret{};
            for (auto &arg : _argList) {
                if (arg.isRegister()) {
                    ret.insert(&arg);
                }
            }
            return ret;
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };

    /// @brief Load instruction, loads a value from an address to a register
    class LoadInst : public DefInst {
    private:
        RegPtr _srcAddrReg;

    public: /* constructors */
        LoadInst(const RegPtr &destReg, const RegPtr &srcAddrReg) : DefInst(Type::Load, destReg), _srcAddrReg(srcAddrReg) { }
        static Ptr<LoadInst> create(const BBPtr &parentBB, const RegPtr &destReg, const RegPtr &srcAddrReg) {
            auto ret = std::make_shared<LoadInst>(destReg, srcAddrReg);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        /// @brief Source address register
        RegPtr &srcAddrReg() { return _srcAddrReg; }

        Set<RegPtrRef> regsReadRefs() override {
            return Set<RegPtrRef>{&_srcAddrReg};
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };

    /// @brief Store instruction, stores a value from a register to an address
    class StoreInst : public Instruction {
    private:
        Value _srcVal;
        RegPtr _destAddrReg;

    public: /* constructors */
        StoreInst(const Value &srcVal, const RegPtr &destAddrReg) : Instruction(Type::Store), _srcVal(srcVal), _destAddrReg(destAddrReg) { }
        static Ptr<StoreInst> create(const BBPtr &parentBB, const Value &srcVal, const RegPtr &destAddrReg) {
            auto ret = std::make_shared<StoreInst>(srcVal, destAddrReg);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        /// @brief Source value
        Value &srcVal() { return _srcVal; }
        /// @brief Destination address value
        RegPtr &destAddrReg() { return _destAddrReg; }

        Set<RegPtrRef> regsWrittenRefs() override {
            return Set<RegPtrRef>{};
        }
        Set<RegPtrRef> regsReadRefs() override {
            Set<RegPtrRef> ret{};
            if (_srcVal.isRegister()) {
                ret.insert(&_srcVal);
            }
            ret.insert(&_destAddrReg);
            return ret;
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };

    /// @brief Allocation instruction, allocates memory for an array (can be multi-dimensional)
    class AllocInst : public DefInst {
    private:

    public: /* constructors */
        AllocInst(const RegPtr &destReg) : DefInst(Type::Alloc, destReg) {
            dbgassert(destReg->dataType().isPointer(), "Data instType to allocate is not pointer");
        }
        static Ptr<AllocInst> create(const BBPtr &parentBB, const RegPtr &destReg) {
            auto ret = std::make_shared<AllocInst>(destReg);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        Set<RegPtrRef> regsReadRefs() override {
            return Set<RegPtrRef>{};
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };

    /// @brief Type cast instruction
    class CastInst : public DefInst {
    private:
        Value _srcVal;

    public: /* constructors */
        CastInst(const RegPtr &destReg, const Value &srcVal) : DefInst(Type::Cast, destReg), _srcVal(srcVal) { }
        static Ptr<CastInst> create(const BBPtr &parentBB, const RegPtr &destReg, const Value &srcVal) {
            auto ret = std::make_shared<CastInst>(destReg, srcVal);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        Value &srcVal() { return _srcVal; }

        Set<RegPtrRef> regsReadRefs() override {
            return _srcVal.isRegister() ? Set<RegPtrRef>{&_srcVal} : Set<RegPtrRef>{};
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };

    /// @brief Get element pointer instruction
    class GEPInst : public DefInst {
    private:
        RegPtr _arrPtrReg;
        Value _indexVal;

    public: /* constructors */
        GEPInst(const RegPtr &destReg, const RegPtr &arrPtrReg, const Value &indexVal) : DefInst(Type::GEP, destReg), _arrPtrReg(arrPtrReg), _indexVal(indexVal) { }
        static Ptr<GEPInst> create(const BBPtr &parentBB, const RegPtr &destReg, const RegPtr &arrPtrReg, const Value &indexVal) {
            auto ret = std::make_shared<GEPInst>(destReg, arrPtrReg, indexVal);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        /// @brief Array pointer register
        RegPtr &arrPtrReg() { return _arrPtrReg; }
        /// @brief Index value
        Value &indexVal() { return _indexVal; }

        Set<RegPtrRef> regsReadRefs() override {
            Set<RegPtrRef> ret{};
            ret.insert(&_arrPtrReg);
            if (_indexVal.isRegister()) {
                ret.insert(&_indexVal);
            }
            return ret;
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };

    /// @brief Phi function instruction, assigns the result to a register
    class PhiInst : public DefInst {
    private:

    public: /* constructors */
        PhiInst(const RegPtr &destReg) : DefInst(Type::Phi, destReg) { }
        static Ptr<PhiInst> create(const BBPtr &parentBB, const RegPtr &destReg) {
            auto ret = std::make_shared<PhiInst>(destReg);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        Set<PhiTuple> getTuples() const {
            Set<PhiTuple> ret{};
            for (auto inEdge : _parentBB->inEdges()) {
                if (inEdge->isFake()) {
                    continue;
                }
                ret.insert(PhiTuple{inEdge->src(), *inEdge->getCopySrcValRef(this->as<PhiInst>())});
            }
            return ret;
        }

        Set<RegPtrRef> regsReadRefs() override {
            Set<RegPtrRef> ret{};
            for (auto inEdge : _parentBB->inEdges()) {
                if (inEdge->isFake()) {
                    continue;
                }
                auto srcValRef = inEdge->getCopySrcValRef(this->as<PhiInst>());
                if (srcValRef->isRegister()) {
                    ret.insert(srcValRef);
                }
            }
            return ret;
        }

    public: /* methods */
        /// @return `true` if the tuple is actually added, `false` if it already exists
        bool addTuple(const BBPtr &srcBB, const Value &srcVal) {
            for (auto inEdge : _parentBB->inEdges()) {
                if (inEdge->src() == srcBB) {
                    return inEdge->addParallelCopy(this->as<PhiInst>(), srcVal);
                }
            }
            return false;
        }
        /// @return `true` if the tuple is actually updated, `false` if not
        bool updateTuple(const BBPtr &srcBB, const Value &srcVal) {
            for (auto inEdge : _parentBB->inEdges()) {
                auto succ = inEdge->src();
                if (succ == srcBB) {
                    return inEdge->updateParallelCopy(this->as<PhiInst>(), srcVal);
                }
            }
            return false;
        }

        String toString(bool exprOnly = false) const override;
    };

    class RetInst : public Instruction {
    private:
        Value _retVal;

    public: /* constructors */
        RetInst(const Value &retVal) : Instruction(Type::Ret), _retVal(retVal) { }
        static Ptr<RetInst> create(const BBPtr &parentBB, const Value &retVal) {
            auto ret = std::make_shared<RetInst>(retVal);
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        Value &retVal() { return _retVal; }

        Set<RegPtrRef> regsWrittenRefs() override {
            return Set<RegPtrRef>{};
        }
        Set<RegPtrRef> regsReadRefs() override {
            return _retVal.isRegister() ? Set<RegPtrRef>{&_retVal} : Set<RegPtrRef>{};
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };

    /// @brief Exit instruction, can be conditional (when `condition` is specified) or unconditional (when `condition` is unspecified) branch, or return from function (when `retVal` is specified)
    class ExitInst : public Instruction {
    private:

    public: /* constructors */
        ExitInst() : Instruction(Type::Exit) { }
        /// @brief Constructor for a default exit instruction
        static Ptr<ExitInst> create(const BBPtr &parentBB) {
            auto ret = makePtr<ExitInst>();
            ret->_init(ret, parentBB);
            return ret;
        }

    public: /* properties */
        bool isBranch() const {
            return !_parentBB->outEdges().empty();
        }
        bool isFuncExit() const {
            return !isBranch() && _parentBB->exitInst().get() == this;
        }
        Value &condition() {
            dbgassert(isBranch(), "Not a branch instruction");
            return _parentBB->firstOutEdge()->brCondition();
        }
        const Value &condition() const {
            dbgassert(isBranch(), "Not a branch instruction");
            return _parentBB->firstOutEdge()->brCondition();
        }
        bool isUncondBr() const {
            dbgassert(isBranch(), "Not a branch instruction");
            return _parentBB->outEdges().size() == 1;
        }
        bool isCondBr() const {
            dbgassert(isBranch(), "Not a branch instruction");
            return _parentBB->outEdges().size() > 1;
        }
        Ptr<CFGEdge> uncondBrEdge() const {
            dbgassert(isBranch(), "Not a branch instruction");
            dbgassert(isUncondBr(),
                      "Cannot get unconditional edge of conditional branch instruction");
            return _parentBB->firstOutEdge();
        }
        Ptr<CFGEdge> condBrTrueEdge() const {
            dbgassert(isBranch(), "Not a branch instruction");
            dbgassert(isCondBr(),
                      "Cannot get true edge of unconditional branch instruction");
            for (auto &e : _parentBB->outEdges()) {
                if (e->tag() == CFGEdge::BrTrue) {
                    return e;
                }
            }
            dbgassert(false, "True edge not found");
            return nullptr;
        }
        Ptr<CFGEdge> condBrFalseEdge() const {
            dbgassert(isBranch(), "Not a branch instruction");
            dbgassert(isCondBr(),
                      "Cannot get false edge of unconditional branch instruction");
            for (auto &e : _parentBB->outEdges()) {
                if (e->tag() == CFGEdge::BrFalse) {
                    return e;
                }
            }
            dbgassert(false, "False edge not found");
            return nullptr;
        }
        const BBPtr &unconditionalTarget() const {
            dbgassert(isBranch(), "Not a branch instruction");
            return uncondBrEdge()->dest();
        }
        const BBPtr &trueTarget() const {
            dbgassert(isBranch(), "Not a branch instruction");
            return condBrTrueEdge()->dest();
        }
        const BBPtr &falseTarget() const {
            dbgassert(isBranch(), "Not a branch instruction");
            return condBrFalseEdge()->dest();
        }

        Set<RegPtrRef> regsWrittenRefs() override {
            return Set<RegPtrRef>{};
        }
        Set<RegPtrRef> regsReadRefs() override {
            Set<RegPtrRef> ret{};
            if (isBranch() && isCondBr() && condition().isRegister()) {
                for (auto outEdge : _parentBB->outEdges()) {
                    ret.insert(&outEdge->brCondition());
                }
            }
            return ret;
        }

    public: /* methods */
        String toString(bool exprOnly = false) const override;
    };
}
