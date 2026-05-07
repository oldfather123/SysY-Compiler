#pragma once
// #include "Block.h"
// #include "Block.h"
// #include "Function.h"
// #include "Function.h"
#include "Label.h"
#include "Type.h"
#include "Value.h"
#include <cassert>
#include <initializer_list>
#include <memory>
// #include <Variable.h>
#include <cstddef>  // For std::ptrdiff_t
#include <cstdlib>
#include <iostream>
#include <iterator>  // For std::forward_iterator_tag
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

struct Function;
struct BasicBlock;
struct Instruction;
struct LoopGEPInstruction;
using BasicBlockPtr = BasicBlock*;

enum class InsID {

    Return,
    Br,

    Alloca,

    Store,
    AtomicAdd,
    Load,

    Call,
    Bitcast,
    Ext,
    Sitofp,
    Fptosi,
    GEP,
    Binary,
    Fneg,
    Icmp,
    Fcmp,
    Phi,
    LoopGEP,
    Select,
};

enum class IcmpKind { ICmpEQ, ICmpNE, ICmpSLT, ICmpSGT, ICmpSLE, ICmpSGE };

struct Use final {
    Value* val;
    Instruction* user;
    Use* next = nullptr;
    Use* prev = nullptr;
    Use(Value* val, Instruction* user);
    Use(const Use& u) = delete;
    Use(Use&& u) = delete;
    Use& operator=(const Use& u) = delete;
    Use& operator=(Use&& u) = delete;
    void markDirty() {
        auto ptrVal = reinterpret_cast<uintptr_t>(val);
        ptrVal |= 1;  // 设置最低位
        val = reinterpret_cast<Value*>(ptrVal);
    }

    // 检查是否标记为“脏”
    bool isDirty() const {
        auto ptrVal = reinterpret_cast<uintptr_t>(val);
        return ptrVal & 1;  // 检查最低位
    }

    void replaceValue(Value* newValue);
    ~Use();
};
using UsePtr = Use*;

struct UseIterator final {

    using difference_type = std::ptrdiff_t;               // 定义差异类型
    using value_type = Instruction*;                      // 迭代器解引用的类型
    using pointer = Instruction**;                        // 指针类型
    using reference = Instruction*&;                      // 引用类型
    using iterator_category = std::forward_iterator_tag;  // 迭代器类别

    Use* use;
    explicit UseIterator(Use* use) : use{ use } {}
    bool operator!=(const UseIterator& other) const noexcept {
        return use != other.use;
    }

    bool operator==(const UseIterator& other) const {
        return use == other.use;
    }

    UseIterator& operator++() {
        use = use->next;
        return *this;
    }
    UseIterator operator++(int) {  // NOLINT
        UseIterator tmp = *this;
        use = use->next;
        return tmp;
    }
    Instruction* operator*() const {
        assert(use->user);
        return use->user;
    }

    // Use* mUse() {
    //     return use;
    // }
};

struct UseList final {
    Use useHead;
    int32_t useCount;

    UseList(Value* self);
    UseList(const UseList& u) = delete;
    UseList(UseList&& u) = delete;
    UseList& operator=(const UseList& u) = delete;
    UseList& operator=(UseList&& u) = delete;
    ~UseList() {
        while(useHead.next) {
            auto tmp = useHead.next;
            useHead.next = tmp->next;
            tmp->markDirty();
            // tmp->val = nullptr;
            // tmp->user = nullptr;
            // delete tmp;  // NOLINT
            useCount--;
        }
    }
    void dump(std::ostream& out = std::cout);
    void addUse(Use& use);
    void removeUse(Use& use);
    bool replaceWith(ValuePtr newValue);
    bool replaceWithInBlock(BasicBlockPtr bb, ValuePtr newValue);
    bool replaceWithInFunc(Function* func, ValuePtr newValue);
    [[nodiscard]] UseIterator begin() const;
    [[nodiscard]] UseIterator end() const;
};

struct User : public Value {
    UseList mUsers;
    User(TypePtr type, const string& name = "", bool isConst = false) : Value{ type, name, isConst }, mUsers{ this } {}

    bool replaceWith(ValuePtr value);
    bool replaceWithInBlock(BasicBlockPtr bb, ValuePtr value);
    bool replaceWithInFunc(Function* func, ValuePtr Value);
    [[nodiscard]] bool isUsed() const {
        return mUsers.useCount > 0;
    }
    [[nodiscard]] bool hasOnlyOneUse() const {
        return mUsers.useCount == 1;
    }
    UseList& users() {
        return mUsers;
    }
    [[nodiscard]] const UseList& users() const {
        return mUsers;
    }
    void cleanupUserResources() {
        mUsers.~UseList();
    }
};
using UserPtr = User*;

struct Operand final {
    Use* use;
    Operand() : use{ nullptr } {}
    Operand(Use* use) : use{ std::move(use) } {}  // NOLINT
    Operand(const Operand& o) = delete;
    Operand& operator=(const Operand& o) = delete;
    Operand(Operand&& o) noexcept : use{ o.use } {
        o.use = nullptr;
    }  // NOLINT

    Operand& operator=(Operand&& o) noexcept {
        Operand tmp(std::move(o));
        swap(tmp);

        return *this;
    }

    ~Operand() {
        if(use)
            std::destroy_at(use);
    }
    void swap(Operand& rhs) {
        std::swap(use, rhs.use);
    }

    Use& operator*() const {
        assert(use);
        return *use;
    }

    UsePtr operator->() const {
        assert(use);
        return use;
    }

    [[nodiscard]] UsePtr get() const {
        return use;
    }

    UsePtr release() {
        return std::exchange(use, nullptr);
    }
};

// TODO: 如果重构出现 问题 记得 检查现在的构造函数 与 原来的构造函数的 差别， 重构过程可能遗漏了某些操作
struct Instruction : public User {
    InsID insId;
    BasicBlock* basicblock;
    vector<Operand> mOperands;
    // bool isRemoved = false;
    Instruction(InsID insId, TypePtr type, BasicBlock* bb = nullptr, const string& name = "", bool isConst = false)
        : User{ type, name, isConst }, insId{ insId }, basicblock{ bb } {}

    // operand init
    Instruction(InsID insId, TypePtr type, std::initializer_list<ValuePtr> operands, BasicBlock* bb = nullptr,
                const string& name = "", bool isConst = false)
        : User{ type, name, isConst }, insId{ insId }, basicblock{ bb } {
        for(auto operand : operands) {  // NOLINT
            addOperand(operand);
        }
    }

    ~Instruction() override {
        cleanupUserResources();
        mOperands.clear();
        basicblock = nullptr;
    };
    void dump(std::ostream& out = std::cout) override {  // NOLINT
        assert(false);
    };

    [[nodiscard]] ValueID valueId() const override {
        return ValueID::Instruction;
    }
    [[nodiscard]] InsID getInsID() const {
        return insId;
    }
    BasicBlock* getBasicBlock() override {
        return basicblock;
    }

    void eraseFromParent();
    Instruction* releaseFromParend();

    auto& getOperandsRef() {
        return mOperands;
    }

    [[nodiscard]] const vector<Operand>& getOperands() const {
        return mOperands;
    }

    // call 指令 包括 callee
    [[nodiscard]] unsigned getNumOperands() const {
        return mOperands.size();
    }

    ValuePtr getOperand(unsigned i) const {
        return mOperands[i]->val;
    }

    UsePtr addOperand(ValuePtr val) {
        // 如果 val 是 user 那么 它的use list 将会加上 this inst
        auto operand = new Use(val, this);  // 构造一个Use对象 // NOLINT
        mOperands.emplace_back(operand);    // 直接在容器中构造对象, 感觉很关键的emplace_back
        return mOperands.back().get();      // 返回新添加对象的原始指针
    }

    void rmOperand(unsigned i) {
        mOperands.erase(mOperands.begin() + i);
    }

    void setOperand(int i, ValuePtr val) {
        auto& operand = mOperands[i];
        operand.use->replaceValue(val);
        //auto operand = new Use(val, this);
        //rmOperand(i);
        //mOperands.insert(mOperands.begin() + i, operand);
    }

    void setBasicBlock(BasicBlock* bb) {
        basicblock = bb;  // NOLINT
    }

    // 交换
    bool isCommutative = false;
    // 结合
    bool isAssociative = false;
    // 幂等
    bool isIdempotent = false;
    // 幂零
    bool isNilpotent = false;

    [[nodiscard]] bool canbeOperand() const;
    // 获得在basicblock的instructions列表中的迭代器
    vector<Instruction*>::iterator getIterator();
    vector<std::unique_ptr<Instruction>>::iterator getIteratorRef();

    [[nodiscard]] virtual Instruction* clone() const = 0;

    [[nodiscard]] bool isTerminator() const {
        return insId == InsID::Br || insId == InsID::Return;
    }

    [[nodiscard]] bool isBranch() const {
        return insId == InsID::Br;
    }

    [[nodiscard]] bool isContiditonBranch() const {
        return insId == InsID::Br && mOperands.size() == 1;
    }

    [[nodiscard]] bool isCompare() const {
        return insId == InsID::Icmp || insId == InsID::Fcmp;
    }
    // just compare insId and type and op
    [[nodiscard]] virtual bool isEqual(Instruction* rhs) const ;

};
using InstructionPtr = Instruction*;

struct ReturnInstruction final : public Instruction {

    ReturnInstruction() : Instruction{ InsID::Return, Type::getVoid(), {} } {}
    ReturnInstruction(ValuePtr retVal) : Instruction{ InsID::Return, Type::getVoid(), { retVal } } {}

    void dump(std::ostream& out = std::cout) override;

    ValuePtr getRetVal() {
        return mOperands.empty() ? Void::get() : getOperand(0);
    }
    [[nodiscard]] Instruction* clone() const override;
};

struct AllocaInstruction final : public Instruction {
    // FIXME: not improve to pointer type
    AllocaInstruction(TypePtr type, bool isConst) : Instruction{ InsID::Alloca, type, nullptr, "", isConst } {}
    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;
};

// TODO: to compute true type, and whether to improve to pointer type
//  llvm GEP inst example : %arrayidx = getelementptr inbounds i32, ptr %a, i32 %idxprom, i32 0
struct GetElementPtrInstruction final : public Instruction {
    bool isConstIdx = false;
    bool typeIsImproved = false;
    // FIXME: arrayidx arrayinit.element
    LoopGEPInstruction* loopGEP = nullptr;
    static const TypePtr getValueType(ValuePtr base, const std::vector<ValuePtr>& indices);

    GetElementPtrInstruction(ValuePtr base, const std::vector<ValuePtr>& indices)
        : Instruction{ InsID::GEP, getValueType(base, indices), {} } {
        addOperand(base);
        for(auto index : indices) {  // NOLINT
            addOperand(index);
        }
    }

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;
    ValuePtr getBase() {
        return getOperand(0);
    }
    void setBase(ValuePtr base) {
        setOperand(0, base);
    }
    void setIdx(unsigned i, ValuePtr idx) {
        setOperand(i + 1, idx);
    }
    // use from as operand 0
    vector<ValuePtr> getIdx() {
        vector<ValuePtr> idx;
        for(int i = 1; i < mOperands.size(); i++) {
            idx.push_back(getOperand(i));
        }
        return idx;
    }
    vector<ValuePtr> getArrIdx() {
        vector<ValuePtr> idx;
        for(int i = 2; i < mOperands.size(); i++) {
            idx.push_back(getOperand(i));
        }
        return idx;
    }
};

// TODO: deal with gep
struct StoreInstruction final : public Instruction {

    GetElementPtrInstruction* gep = nullptr;
    StoreInstruction(GetElementPtrInstruction* gep, ValuePtr value)
        : Instruction{ InsID::Store, Type::getVoid(), { gep, value } }, gep{ gep } {}

    StoreInstruction(ValuePtr des, ValuePtr value) : Instruction{ InsID::Store, Type::getVoid(), { des, value } } {}

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;

    ValuePtr getPtr() {
        return getOperand(0);
    }
    ValuePtr getValue() {
        return getOperand(1);
    }
};

// TODO: deal with dep
struct AtomicAddInstruction final : public Instruction {
    // shared_ptr<GetElementPtrInstruction> gep;
    // AtomicAddInstruction(const shared_ptr<GetElementPtrInstruction>& gep, const ValuePtr& value, shared_ptr<BasicBlock> bb)
    //     : Instruction{ InsID::AtomicAdd, std::move(bb) }, gep{ gep }, des{ gep->reg }, value{ value } {
    //     newUse(gep->reg.get(), this);
    //     // newUse(gep->from.get(), this);
    //     newUse(value.get(), this);
    // };

    AtomicAddInstruction(ValuePtr des, ValuePtr value) : Instruction{ InsID::AtomicAdd, Type::getVoid(), { des, value } } {}
    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;

    ValuePtr getPtr() {
        return mOperands[0]->val;
    }
    ValuePtr getValue() {
        return mOperands[1]->val;
    }
};

// TODO: deal with gep
struct LoadInstruction final : public Instruction {
    GetElementPtrInstruction* gep = nullptr;
    // FIXME: not improve to pointer
    LoadInstruction(ValuePtr from, TypePtr toType) : Instruction{ InsID::Load, toType, { from } } {}

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;

    ValuePtr getPtr() {
        return getOperand(0);
    }
};

struct BitCastInstruction final : public Instruction {
    BitCastInstruction(ValuePtr srcValue, TypePtr toType = new PtrType(Type::getInt8()))
        : Instruction{ InsID::Bitcast, toType, { srcValue } } {}
    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;
    ValuePtr getFrom() {
        return getOperand(0);
    }
    TypePtr getToType() {
        return type;
    }
};

struct ExtInstruction final : public Instruction {
    bool isign = true;
    // ExtInstruction(const ValuePtr& from, const TypePtr& to, bool isign, shared_ptr<BasicBlock> bb)
    //     : Instruction{ InsID::Ext, std::move(bb), getExtReg(to) }, from{ from }, to{ to }, isign{ isign } {
    //     newUse(from.get(), this, reg.get());
    //     // reg即这条指令本身
    //     reg->I = this;
    // }
    ExtInstruction(ValuePtr from, TypePtr to, bool isign) : Instruction{ InsID::Ext, to, { from } }, isign{ isign } {}
    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;
    ValuePtr getFrom() {
        // return shared_ptr<Value>(mOperands[0]->val);
        return getOperand(0);
    }
    TypePtr getToType() {
        return type;
    }
};

struct SitofpInstruction final : public Instruction {

    SitofpInstruction(ValuePtr from) : Instruction{ InsID::Sitofp, Type::getFloat(), { from } } {}

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;

    friend struct FptosiInstruction;
    ValuePtr getFrom() {
        // return shared_ptr<Value>(mOperands[0]->val);
        return getOperand(0);
    }
};

struct FptosiInstruction final : public Instruction {
    FptosiInstruction(ValuePtr from) : Instruction{ InsID::Fptosi, Type::getInt(), { from } } {}

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;
    ValuePtr getFrom() {
        // return shared_ptr<Value>(mOperands[0]->val);
        return getOperand(0);
    }
};

struct CallInstruction final : public Instruction {

    CallInstruction(ValuePtr callee, const vector<Value*>& args) : Instruction{ InsID::Call, callee->type, {} } {
        for(auto& arg : args) {
            addOperand(arg);
        }
        addOperand(callee);
    }

    ValuePtr getCallee() {
        // return shared_ptr<Value>(mOperands.back()->val);
        return getOperand(mOperands.size() - 1);
    }
    ValuePtr getArg(unsigned i) {
        // return shared_ptr<Value>(mOperands[i]->val);
        return getOperand(i);
    }

    int getNumArg() {
        return mOperands.size() - 1;  // NOLINT
    }

    vector<Value*> getArgs() {
        vector<Value*> args;
        args.reserve(mOperands.size() - 1);
        for(int i = 0; i < mOperands.size() - 1; i++) {
            args.push_back(getOperand(i));
        }
        return args;
    }

    // 函数 作为第一个操作数
    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;

    Function* getFunc();
};

// TODO: 没有合并 hq的更改

enum BinaryInstructionOps { Add, Mul, Sub, Div, Rem, Xor, Shl, AShr, FAdd, FSub, FMul, FDiv, Not };

struct BinaryInstruction final : public Instruction {
    bool isrv64 = false;
    BinaryInstructionOps op;
    BinaryInstruction(BinaryInstructionOps op, ValuePtr lhs, ValuePtr rhs)
        : Instruction{ InsID::Binary, lhs->type, { lhs, rhs } }, op{ op } {
        if(getOp() == "!")
            type = Type::getBool();
        if(lhs->type->isInt() && rhs->type->isInt() && (getOp() == "+" || getOp() == "*" || getOp() == "!")) {
            isCommutative = true;
            isAssociative = true;
        }
        if(lhs->type->isInt() && rhs->type->isInt() && getOp() == "!") {
            isNilpotent = true;
        }
    }

    [[nodiscard]] Instruction* clone() const override;
    void dump(std::ostream& out = std::cout) override;

    string getOp() const {
        switch(op) {
            case Add:
                return "+";
            case Mul:
                return "*";
            case Sub:
                return "-";
            case Div:
                return "/";
            case Rem:
                return "%";
            case Xor:
                return "!";
            case Shl:
                return ",";
            case AShr:
                return ".";
            case FAdd:
                return "+";
            case FSub:
                return "-";
            case FMul:
                return "*";
            case FDiv:
                return "/";
            case Not:
                return "!";
            default:
                assert(false);
        }
        assert(false);
        return "";
    }
    ValuePtr getLHS() {
        // return shared_ptr<Value>(mOperands[0]->val);
        return getOperand(0);
    }
    ValuePtr getRHS() {
        // return shared_ptr<Value>(mOperands[1]->val);
        return getOperand(1);
    }
    //// I为insertbefore
    // BinaryInstruction(ValuePtr a, ValuePtr b, char op, Instruction* I);  // NOLINT
};

struct FnegInstruction final : public Instruction {
    FnegInstruction(ValuePtr src) : Instruction{ InsID::Fneg, Type::getFloat(), { src } } {}

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;
    ValuePtr getSrc() {
        // return shared_ptr<Value>(mOperands[0]->val);
        return getOperand(0);
    }
};

struct IcmpInstruction final : public Instruction {
    IcmpKind kind;
    static std::map<string, IcmpKind> kindMap;
    IcmpKind getReverseKind();
    IcmpKind getInvertedKind();
    void setReverseKind();
    void setInvertedKind();

    IcmpInstruction(ValuePtr lhs, ValuePtr rhs = Const::getConst(Type::getInt(), 0), IcmpKind kind = IcmpKind::ICmpNE)
        : Instruction{ InsID::Icmp, Type::getBool(), { lhs, rhs } }, kind{ kind } {}

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;
    bool isEqual(Instruction* rhs) const override;
    [[nodiscard]] string getOp() const {
        switch(kind) {
            case IcmpKind::ICmpEQ:
                return "==";
            case IcmpKind::ICmpNE:
                return "!=";
            case IcmpKind::ICmpSGT:
                return ">";
            case IcmpKind::ICmpSGE:
                return ">=";
            case IcmpKind::ICmpSLT:
                return "<";
            case IcmpKind::ICmpSLE:
                return "<=";
        }
        assert(false);
        return "";
    }
    ValuePtr getLHS() {
        // return shared_ptr<Value>(mOperands[0]->val);
        return getOperand(0);
    }
    ValuePtr getRHS() {
        // return shared_ptr<Value>(mOperands[1]->val);
        return getOperand(1);
    }
    friend struct FcmpInstruction;
};

struct FcmpInstruction final : public Instruction {

    IcmpKind kind;
    FcmpInstruction(ValuePtr lhs, ValuePtr rhs = Const::getConst(Type::getFloat(), (float)0), IcmpKind kind = IcmpKind::ICmpNE)
        : Instruction{ InsID::Fcmp, Type::getBool(), { lhs, rhs } }, kind{ kind } {}

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;
    bool isEqual(Instruction* rhs) const override;
    [[nodiscard]] string getOp() const {
        switch(kind) {
            case IcmpKind::ICmpEQ:
                return "==";
            case IcmpKind::ICmpNE:
                return "!=";
            case IcmpKind::ICmpSGT:
                return ">";
            case IcmpKind::ICmpSGE:
                return ">=";
            case IcmpKind::ICmpSLT:
                return "<";
            case IcmpKind::ICmpSLE:
                return "<=";
        }
        assert(false);
        return "";
    }

    ValuePtr getLHS() {
        // return shared_ptr<Value>(mOperands[0]->val);
        return getOperand(0);
    }
    ValuePtr getRHS() {
        // return shared_ptr<Value>(mOperands[1]->val);
        return getOperand(1);
    }
};

struct BrInstruction final : public Instruction {
    static int ifThenNum;
    static int ifEndNum;
    static int ifElseNum;
    static int orNum;
    static int andNum;
    static int whileCondNum;
    static int whileBodyNum;
    static int whileEndNum;
    static string getifThenStr() {
        return "if.then" + to_string(ifThenNum++);
    }
    static string getifEndStr() {
        return "if.end" + to_string(ifEndNum++);
    }
    static string getifElseStr() {
        return "if.else" + to_string(ifElseNum++);
    }
    static string getOrStr() {
        return "lor.lhs.false" + to_string(orNum++);
    }
    static string getAndStr() {
        return "land.lhs.true" + to_string(andNum++);
    }
    static string getWhileCondStr() {
        return "while.cond" + to_string(whileCondNum++);
    }
    static string getwhileBodyStr() {
        return "while.body" + to_string(whileBodyNum++);
    }
    static string getwhileEndStr() {
        return "while.end" + to_string(whileEndNum++);
    }
    // 条件表达式
    // ValuePtr exp = nullptr;
    BasicBlockPtr trueTarget = nullptr;
    BasicBlockPtr falseTarget = nullptr;

    BrInstruction(BasicBlockPtr target)
        : Instruction{ InsID::Br, Type::getVoid(), {} }, trueTarget{ target }, falseTarget(nullptr) {}
    BrInstruction(ValuePtr condition, BasicBlockPtr trueTarget, BasicBlockPtr falseTarget)
        : Instruction{ InsID::Br, Type::getVoid(), { condition } }, trueTarget{ trueTarget }, falseTarget{ falseTarget } {}

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;
    bool isEqual(Instruction* rhs) const override;

    ValuePtr getCondition() {
        if(mOperands.empty()) {
            return nullptr;
        }
        // return shared_ptr<Value>(mOperands[0]->val);
        return getOperand(0);
    }
    [[nodiscard]] BasicBlockPtr getTrueTarget() const {
        return trueTarget;
    }
    [[nodiscard]] BasicBlockPtr getFalseTarget() const {
        return falseTarget;
    }
    void swapTargets() {
        std::swap(trueTarget, falseTarget);
    }

    void setTrueTarget(BasicBlockPtr target) {
        trueTarget = target;
    }
    void setFalseTarget(BasicBlockPtr target) {
        falseTarget = target;
    }
    void clearCondition() {
        rmOperand(0);
    }
};

// TODO: 没有理解这个mem2reg 用到 的 val 的作用
struct PhiInstruction final : public Instruction {
    ValuePtr val = nullptr;  // mem2reg中才会用到
    // vector<std::pair<ValuePtr, shared_ptr<BasicBlock>>> from;
    unordered_map<BasicBlockPtr, UsePtr> mIncomings = {};
    string op;
    // PhiInstruction(shared_ptr<BasicBlock> bb, const ValuePtr& val)
    //     : Instruction{ InsID::Phi, std::move(bb), getPhiReg(val->type) }, val{ val } {
    //     // newUse(val.get(), this, reg.get());
    //     reg->I = this;
    // };
    //// bug 无法print
    // PhiInstruction(shared_ptr<BasicBlock> bb, const TypePtr& type)
    //     : Instruction{ InsID::Phi, std::move(bb), getPhiReg(type) }, val{ nullptr } {
    //     // newUse(val.get(), this, reg.get());
    //     val = Const::getConst(type, 0);
    //     reg->I = this;
    // };

    PhiInstruction(ValuePtr val) : Instruction{ InsID::Phi, val->type, {}, "phi" }, val{ val } {}
    PhiInstruction(TypePtr type) : Instruction{ InsID::Phi, type, {}, "phi" } {}
    void addIncoming(BasicBlockPtr bb, ValuePtr val);

    auto& incomings() const {
        return mIncomings;
    }
    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;

    void clear();
    void removeSource(BasicBlockPtr block);
    void replaceSource(BasicBlockPtr oldBlock, BasicBlockPtr newBlock);
    void keepOneIncoming(BasicBlockPtr block);
};

// FIXME: 暂时还没有理解实际使用中的情况？, 这里暂时没有处理use
struct LoopGEPInstruction final : public Instruction {

    GetElementPtrInstruction* gep = nullptr;
    Value* index;
    int initVal, stepVal;

    // LoopGEPInstruction(shared_ptr<BasicBlock> bb, GetElementPtrInstruction* gep, const shared_ptr<Value>& index, int
    // initVal,
    //                    int stepVal)
    //     : Instruction{ InsID::LoopGEP, std::move(bb), getLoopGEPReg(index->type) }, gep(gep), index(index),
    //     initVal(initVal),
    //       stepVal(stepVal) {
    //     gep->loopGEP = this;
    // }
    //  TODO:
    LoopGEPInstruction(GetElementPtrInstruction* gep, ValuePtr index, int initVal, int stepVal)
        : Instruction{ InsID::LoopGEP, gep->type, {} }, gep{ gep }, index{ index }, initVal{ initVal }, stepVal{ stepVal } {
        gep->loopGEP = this;
    }

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;

    int getInitOffset();
    int getStepOffset();
};

struct SelectInstruction final : public Instruction {

    /// SelectInstruction(shared_ptr<BasicBlock> bb, ValuePtr exp, const ValuePtr& val1, ValuePtr val2)
    ///     : Instruction{ InsID::Select, std::move(bb), getSelReg(val1->type) }, exp{ std::move(exp) }, val1{ val1 },
    ///       val2{ std::move(val2) } {
    ///     // newUse(val.get(), this, reg.get());
    ///     reg->I = this;
    /// };

    SelectInstruction(ValuePtr exp, const ValuePtr& val1, ValuePtr val2)
        : Instruction{ InsID::Select, val1->type, { exp, val1, val2 } } {}

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] Instruction* clone() const override;

    ValuePtr getExp() {
        // return shared_ptr<Value>(mOperands[0]->val);
        return getOperand(0);
    }
    ValuePtr getVal1() {
        // return shared_ptr<Value>(mOperands[1]->val);
        return getOperand(1);
    }
    ValuePtr getVal2() {
        // return shared_ptr<Value>(mOperands[2]->val);
        return getOperand(2);
    }
};

void insertBefore(BasicBlock* bb, vector<unique_ptr<Instruction>>::iterator insertPoint, Instruction* self);
// O(n)的时间复杂度，后续编译时间不足可以优化
void moveBefore(BasicBlock* movedbb, vector<unique_ptr<Instruction>>::iterator insertPoint, Instruction* self);
// void replaceVarByVarForLCSSA(ValuePtr from, ValuePtr to, Use* use);