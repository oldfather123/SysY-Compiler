#include "Block.h"
#include "IRbuilder.h"
#include "Instruction.h"
#include "StringTable.h"
#include "Type.h"
#include "Value.h"
#include "utils.h"
#include <Function.h>
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

int BrInstruction::ifThenNum = 0;
int BrInstruction::ifElseNum = 0;
int BrInstruction::ifEndNum = 0;
int BrInstruction::orNum = 0;
int BrInstruction::andNum = 0;
int BrInstruction::whileCondNum = 0;
int BrInstruction::whileBodyNum = 0;
int BrInstruction::whileEndNum = 0;
// NOLINTBEGIN
map<string, IcmpKind> IcmpInstruction::kindMap{ { "==", IcmpKind::ICmpEQ }, { "!=", IcmpKind::ICmpNE },
                                                { "<", IcmpKind::ICmpSLT }, { "<=", IcmpKind::ICmpSLE },
                                                { ">", IcmpKind::ICmpSGT }, { ">=", IcmpKind::ICmpSGE } };

// >= -> <=  , > -> <,  == -> ==
IcmpKind IcmpInstruction::getReverseKind() {
    switch(kind) {
        case IcmpKind::ICmpEQ:
            return IcmpKind::ICmpEQ;
        case IcmpKind::ICmpNE:
            return IcmpKind::ICmpNE;
        case IcmpKind::ICmpSLT:
            return IcmpKind::ICmpSGT;
        case IcmpKind::ICmpSLE:
            return IcmpKind::ICmpSGE;
        case IcmpKind::ICmpSGT:
            return IcmpKind::ICmpSLT;
        case IcmpKind::ICmpSGE:
            return IcmpKind::ICmpSLE;
        default:
            assert(false);
    }
}

IcmpKind IcmpInstruction::getInvertedKind() {
    switch(kind) {
        case IcmpKind::ICmpEQ:
            return IcmpKind::ICmpNE;
        case IcmpKind::ICmpNE:
            return IcmpKind::ICmpEQ;
        case IcmpKind::ICmpSLT:
            return IcmpKind::ICmpSGE;
        case IcmpKind::ICmpSLE:
            return IcmpKind::ICmpSGT;
        case IcmpKind::ICmpSGT:
            return IcmpKind::ICmpSLE;
        case IcmpKind::ICmpSGE:
            return IcmpKind::ICmpSLT;
        default:
            assert(false);
    }
}

void IcmpInstruction::setInvertedKind() {
    kind = getInvertedKind();
}

void IcmpInstruction::setReverseKind() {
    kind = getReverseKind();
}

bool Instruction::isEqual(Instruction* rhs) const {
    if(insId != rhs->insId)
        return false;
    // FIXME: 指针 等
    if(getType()->id != rhs->getType()->id)
        return false;
    return true;
}
bool IcmpInstruction::isEqual(Instruction* rhs) const {
    if(!Instruction::isEqual(rhs))
        return false;
    const auto rhsCompare = rhs->as<IcmpInstruction>();
    return kind == rhsCompare->kind;
}

bool FcmpInstruction::isEqual(Instruction* rhs) const {
    if(!Instruction::isEqual(rhs))
        return false;
    const auto rhsCompare = rhs->as<FcmpInstruction>();
    return kind == rhsCompare->kind;
}

bool BrInstruction::isEqual(Instruction* rhs) const {
    if(!Instruction::isEqual(rhs))
        return false;
    const auto rhsBranch = rhs->as<BrInstruction>();
    return trueTarget == rhsBranch->trueTarget && falseTarget == rhsBranch->falseTarget;
}

// TODO: 没有测试
//! NOTICE: iter->get() -> Instriuction*
vector<std::unique_ptr<Instruction>>::iterator Instruction::getIteratorRef() {
    auto bb = this->getBasicBlock();
    return std::find_if(bb->instructionsRef().begin(), bb->instructionsRef().end(),
                        [this](const std::unique_ptr<Instruction>& ptr) -> bool { return ptr.get() == this; });
}

// TODO: 没有测试
// TODO: may remove this func
vector<Instruction*>::iterator Instruction::getIterator() {
    auto bb = this->getBasicBlock();
    auto insts = bb->instructions();
    return std::find_if(insts.begin(), insts.end(), [this](Instruction* ptr) -> bool { return ptr == this; });
}

// TODO: 没有测试
void moveBefore(BasicBlock* movedbb, vector<unique_ptr<Instruction>>::iterator insertPoint, Instruction* self) {
    assert(movedbb);

    auto nowbb = self->getBasicBlock();
    auto iter = std::find_if(nowbb->instructionsRef().begin(), nowbb->instructionsRef().end(),
                             [self](const std::unique_ptr<Instruction>& ptr) -> bool { return ptr.get() == self; });

    assert(iter != nowbb->instructionsRef().end());
    auto inst = std::move(*iter);
    inst->basicblock = movedbb;
    nowbb->instructionsRef().erase(iter);
    IRbuilder builder(movedbb);
    builder.setInsertPoint(movedbb, insertPoint);
    builder.pushInstruction(std::move(inst));
}

// 这里只能insert 新的指令！！！
void insertBefore(BasicBlock* bb, vector<unique_ptr<Instruction>>::iterator insertPoint, Instruction* self) {
    assert(bb);
    IRbuilder builder(bb);
    builder.setInsertPoint(bb, insertPoint);
    builder.pushInstruction(std::unique_ptr<Instruction>(self));
}

Function* CallInstruction::getFunc() {
    return dynamic_cast<Function*>(mOperands.back()->val);
}

Use::Use(Value* value, Instruction* user) : val{ value }, user{ user } {
    assert(value);
    if(auto self = dynamic_cast<User*>(value)) {
        self->users().addUse(*this);
    }
}

// TODO: 显然如果知道指令的迭代器 删除会很方便，所以迭代器的优化是必要的
void Instruction::eraseFromParent() {

    auto bb = this->getBasicBlock();
    auto iter = std::find_if(bb->instructionsRef().begin(), bb->instructionsRef().end(),
                             [this](const std::unique_ptr<Instruction>& ptr) -> bool { return ptr.get() == this; });

    assert(iter != bb->instructionsRef().end());
    // FIXME: 如果是endInstruction，需要特殊处理?
    bb->instructionsRef().erase(iter);
}

void Use::replaceValue(Value* newValue) {
    assert(newValue);

    if(auto self = dynamic_cast<User*>(val)) {
        self->users().removeUse(*this);
    }
    val = newValue;
    if(auto self = dynamic_cast<User*>(val)) {
        self->users().addUse(*this);
    }
}

Use::~Use() {
    if(isDirty())
        return;
    if(val == nullptr)
        return;
    if(auto self = dynamic_cast<User*>(val)) {
        self->users().removeUse(*this);
    }
}

// FIXME: 也许有问题
// Operand::~Operand() {
//    if(use) {
//        use.reset();
//    }
//}
UseList::UseList(Value* self) : useHead{ self, nullptr }, useCount{ 0 } {}

void UseList::addUse(Use& U) {
    if(&U == &useHead)
        return;
    assert(U.val == useHead.val);
    U.prev = &useHead;
    U.next = useHead.next;
    if(useHead.next) {
        useHead.next->prev = &U;
    }
    useHead.next = &U;
    ++useCount;
}

void UseList::removeUse(Use& U) {
    if(&U == &useHead)
        return;

    assert(U.val == useHead.val);

    if(U.prev) {
        U.prev->next = U.next;
    }
    if(U.next) {
        U.next->prev = U.prev;
    }
    --useCount;
    assert(useCount >= 0);
}

[[nodiscard]] UseIterator UseList::begin() const {
    return UseIterator(useHead.next);
}

[[nodiscard]] UseIterator UseList::end() const {
    return UseIterator(nullptr);
}

bool Instruction::canbeOperand() const {
    if(insId == InsID::Call) {
        return !getType()->isVoid();
    }
    // TODO: may more conditions
    return !isTerminator() && insId != InsID::Store;
}

bool UseList::replaceWith(ValuePtr value) {
    auto cur = useHead.next;
    if(!cur)
        return false;
    while(cur) {
        auto next = cur->next;
        cur->replaceValue(value);
        cur = next;
    }
    return true;
}

bool UseList::replaceWithInBlock(BasicBlockPtr block, ValuePtr value) {
    auto cur = useHead.next;
    if(!cur)
        return false;
    bool modified = false;
    while(cur) {
        auto next = cur->next;
        if(cur->user->basicblock == block) {
            cur->replaceValue(value);
            modified = true;
        }
        cur = next;
    }
    return modified;
}

bool UseList::replaceWithInFunc(Function* func, ValuePtr newValue) {
    auto cur = useHead.next;
    if(!cur)
        return false;
    bool modified = false;
    while(cur) {
        auto next = cur->next;
        if(cur->user->getBasicBlock()->belongfunc == func) {
            cur->replaceValue(newValue);
            modified = true;
        }
        cur = next;
    }
    return modified;
}

void UseList::dump(std::ostream& out) {
    out << "UseList: ";
    Use* tmp = useHead.next;
    while(tmp) {
        out << tmp->user->getStr() << " ";
        tmp = tmp->next;
    }
    out << std::endl;
}

bool User::replaceWith(ValuePtr value) {
    assert(value != this);
    return users().replaceWith(value);
}

bool User::replaceWithInBlock(BasicBlockPtr block, ValuePtr value) {
    assert(value != this);
    return users().replaceWithInBlock(block, value);
}

bool User::replaceWithInFunc(Function* func, ValuePtr value) {
    assert(value != this);
    return users().replaceWithInFunc(func, value);
}

// TODO: 实现 clone 和 dump 以及 phi 指令的一些接口

void PhiInstruction::addIncoming(BasicBlockPtr bb, ValuePtr val) {
    assert(val->type->getID() == getType()->getID());
    assert(!mIncomings.count(bb));
    // if(mIncomings.count(bb)) {
    //     mIncomings.at(bb)->replaceValue(val);
    // }
    mIncomings.emplace(bb, addOperand(val));
}

void PhiInstruction::replaceSource(BasicBlockPtr oldBB, BasicBlockPtr newBB) {
    assert(oldBB != newBB);
    auto iter = mIncomings.find(oldBB);
    if(iter != mIncomings.end()) {
        mIncomings.emplace(newBB, iter->second);
        mIncomings.erase(iter);
    }
}

void PhiInstruction::removeSource(BasicBlockPtr bb) {
    auto iter = mIncomings.find(bb);
    assert(iter != mIncomings.end());
    auto value = iter->second->val;
    mIncomings.erase(iter);
    auto& operands = getOperandsRef();
    auto it = std::find_if(operands.begin(), operands.end(), [&](Operand& op) { return op->val == value; });
    assert(it != operands.end());
    operands.erase(it);
}

// FIXME: 很可能存在问题
void PhiInstruction::keepOneIncoming(BasicBlockPtr bb) {
    const auto val = mIncomings.at(bb);
    mIncomings = { { bb, val } };
    auto& operands = getOperandsRef();
    for(auto& ref : operands)
        if(ref.get() == val)
            ref.release();
    operands.clear();
    operands.push_back(Operand{ val });
}

void PhiInstruction::clear() {
    getOperandsRef().clear();
    mIncomings.clear();
}

[[nodiscard]] Instruction* ReturnInstruction::clone() const {
    Instruction* cloneInst;
    if(mOperands.empty())
        cloneInst = new ReturnInstruction(nullptr);
    else
        cloneInst = new ReturnInstruction(mOperands[0]->val);
    cloneInst->setLabel(getName());
    return cloneInst;
};
[[nodiscard]] Instruction* AllocaInstruction::clone() const {
    Instruction* cloneInst = new AllocaInstruction(getType(), isConst);
    cloneInst->setLabel("alloca");
    return cloneInst;
};
[[nodiscard]] Instruction* StoreInstruction::clone() const {
    Instruction* cloneInst = new StoreInstruction(mOperands[0]->val, mOperands[1]->val);
    // FIXME:
    cloneInst->as<StoreInstruction>()->gep = gep;
    cloneInst->setLabel("store");
    return cloneInst;
};

[[nodiscard]] Instruction* LoadInstruction::clone() const {
    Instruction* cloneInst = new LoadInstruction(mOperands[0]->val, getType());
    cloneInst->as<LoadInstruction>()->gep = gep;
    cloneInst->setLabel("load");
    return cloneInst;
};
[[nodiscard]] Instruction* AtomicAddInstruction::clone() const {
    assert(false);
};
[[nodiscard]] Instruction* BitCastInstruction::clone() const {
    Instruction* cloneInst = new BitCastInstruction(mOperands[0]->val, getType());
    cloneInst->setLabel("bitcast");
    return cloneInst;
};
[[nodiscard]] Instruction* CallInstruction::clone() const {
    vector<Value*> args;
    int num = getNumOperands();
    for(int i = 0; i < getNumOperands() - 1; i++) {
        args.push_back(mOperands[i]->val);
    }
    Instruction* cloneInst = new CallInstruction(getOperand(num - 1), args);
    cloneInst->setLabel("call");
    return cloneInst;
};
[[nodiscard]] Instruction* ExtInstruction::clone() const {
    Instruction* cloneInst = new ExtInstruction(mOperands[0]->val, getType(), isign);
    cloneInst->setLabel("ext");
    return cloneInst;
};
[[nodiscard]] Instruction* SitofpInstruction::clone() const {
    Instruction* cloneInst = new SitofpInstruction(mOperands[0]->val);
    cloneInst->setLabel("sitofp");
    return cloneInst;
};
[[nodiscard]] Instruction* FptosiInstruction::clone() const {
    Instruction* cloneInst = new FptosiInstruction(mOperands[0]->val);
    cloneInst->setLabel("fptosi");
    return cloneInst;
};
[[nodiscard]] Instruction* PhiInstruction::clone() const {
    PhiInstruction* cloneInst = new PhiInstruction(getType());
    for(auto [block, val] : mIncomings) {
        cloneInst->addIncoming(block, val->val);
    }
    cloneInst->val = val;
    cloneInst->setLabel("phi");
    return cloneInst;
};
[[nodiscard]] Instruction* SelectInstruction::clone() const {
    SelectInstruction* cloneInst = new SelectInstruction(mOperands[0]->val, mOperands[1]->val, mOperands[2]->val);
    cloneInst->setLabel("select");
    return cloneInst;
};
[[nodiscard]] Instruction* GetElementPtrInstruction::clone() const {
    vector<ValuePtr> idx;
    for(int i = 1; i < mOperands.size(); i++) {
        idx.push_back(mOperands[i]->val);
    }

    GetElementPtrInstruction* cloneInst = new GetElementPtrInstruction(mOperands[0]->val, idx);
    cloneInst->type = type;
    // assert(cloneInst->type->getID() == type->getID());
    // cloneInst->type = type;
    cloneInst->loopGEP = loopGEP;
    cloneInst->setLabel("gep");
    cloneInst->type = type;
    return cloneInst;
};

[[nodiscard]] Instruction* BinaryInstruction::clone() const {
    BinaryInstruction* cloneInst = new BinaryInstruction(op, mOperands[0]->val, mOperands[1]->val);
    cloneInst->isrv64 = isrv64;
    cloneInst->setLabel("binary");
    return cloneInst;
};

[[nodiscard]] Instruction* FnegInstruction::clone() const {
    FnegInstruction* cloneInst = new FnegInstruction(mOperands[0]->val);
    cloneInst->setLabel("fneg");
    return cloneInst;
};

[[nodiscard]] Instruction* IcmpInstruction::clone() const {
    IcmpInstruction* cloneInst = new IcmpInstruction(mOperands[0]->val, mOperands[1]->val, kind);
    cloneInst->setLabel("icmp");
    return cloneInst;
};

[[nodiscard]] Instruction* LoopGEPInstruction::clone() const {
    assert(false);
}

[[nodiscard]] Instruction* FcmpInstruction::clone() const {
    FcmpInstruction* cloneInst = new FcmpInstruction(mOperands[0]->val, mOperands[1]->val, kind);
    cloneInst->setLabel("fcmp");
    return cloneInst;
};
[[nodiscard]] Instruction* BrInstruction::clone() const {
    if(mOperands.empty()) {
        BrInstruction* cloneInst = new BrInstruction(trueTarget);
        cloneInst->setLabel("br");
        return cloneInst;
    }
    BrInstruction* cloneInst = new BrInstruction(mOperands[0]->val, trueTarget, falseTarget);
    cloneInst->setLabel("br");
    return cloneInst;
};

void ReturnInstruction::dump(std::ostream& out) {
    if(getRetVal()->getType()->isVoid())
        out << "  ret void" << endl;
    else {
        out << "  ret " << getRetVal()->getTypeAndStr() << endl;
    }
}

void AllocaInstruction::dump(std::ostream& out) {
    out << "  " << getStr() << " = alloca " << getTypeStr() << endl;
}

void StoreInstruction::dump(std::ostream& out) {
    if(gep) {
        out << "  store " << getValue()->getTypeAndStr() << ", " << getPtr()->getTypeStr() << "* getelementptr inbounds ("
            << gep->getBase()->getTypeStr() << ", " << gep->getBase()->getTypePointStr();
        for(const auto& ind : gep->getIdx())
            out << ", " << ind->getTypeAndStr();
        out << ")" << '\n';
    } else {
        // out << "  store " << value->getTypeStr() << ", " << des->getPointStr() << endl;
        // out << "  store " << getValue()->getTypeAndStr() << ", " << getPtr()->getTypePointStr() << endl;
        out << "  store " << getValue()->getTypeAndStr() << ", " << getValue()->getType()->getStr() << "* " << getPtr()->getStr() << endl;
    }
}

void AtomicAddInstruction::dump(std::ostream& out) {
    // if(gep) {
    //     ASSERT(false, "not implemented");
    // } else {
    //     out << "atomicadd  " << value->getTypeStr() << ", " << des->getPointStr() << endl;
    // }
}

void LoadInstruction::dump(std::ostream& out) {
    if(gep) {
        // out << "  " << to->getStr() << " = load " << from->type->getStr() << ", " << from->type->getStr()
        //     << "* getelementptr inbounds (" << gep->from->type->getStr() << ", " << gep->from->getPointStr();
        out << "  " << getStr() << " = load " << getPtr()->getTypeStr() << ", " << getPtr()->getTypeStr()
            << "* getelementptr inbounds (" << gep->getBase()->getTypeStr() << ", " << gep->getBase()->getTypePointStr();
        for(const auto& ind : gep->getIdx())
            // out << ", " << ind->getTypeStr();
            out << ", " << ind->getTypeAndStr();
        // out << ")" << endl;
        out << ")" << '\n';
    } else {
        // out << "  " << to->getStr() << " = load " << from->type->getStr() << ", " << from->getPointStr() << endl;
        out << "  " << getStr() << " = load " << type->getStr() << ", "<< type->getStr()<<"* " << getPtr()->getStr() << endl;
    }
}

void BitCastInstruction::dump(std::ostream& out) {
    // out << "  " << reg->getStr() << " = bitcast " << from->getPointStr() << " to " << toType->getStr() << endl;
    out << "  " << getStr() << " = bitcast " << getFrom()->getTypePointStr() << " to " << getToType()->getStr() << endl;
}

void CallInstruction::dump(std::ostream& out) {
    out << "  ";
    if(!getCallee()->type->isVoid())
        //     out << reg->getStr() << " = ";
        // out << "call " << func->getTypeStr() << "(";
        out << getStr() << " = ";
    out << "call " << getCallee()->getTypeAndStr() << "(";
    for(int i = 0; i < getNumArg(); i++) {
        //if (getFunc()->getName().find("yatccCacheLook") != string::npos) break;
        if(i)
            out << ", ";
        out << getFunc()->getArg(i)->getTypeStr();
        out << " ";
        // out << argv[i]->getTypeStr();
        out << getArg(i)->getStr();
    }
    out << ")" << endl;
}

void ExtInstruction::dump(std::ostream& out) {
    // out << "  " << reg->getStr() << " = " << (isign ? "s" : "z") << "ext " << from->getTypeStr() << " to " << to->getStr()
    //     << endl;
    out << "  " << getStr() << " = " << (isign ? "s" : "z") << "ext " << getFrom()->getTypeAndStr() << " to "
        << getToType()->getStr() << endl;
}

void SitofpInstruction::dump(std::ostream& out) {
    // out << "  " << reg->getStr() << " = sitofp " << from->getTypeStr() << " to float" << endl;
    out << "  " << getStr() << " = sitofp " << getFrom()->getTypeAndStr() << " to float" << endl;
}

void FptosiInstruction::dump(std::ostream& out) {
    // out << "  " << reg->getStr() << " = fptosi " << from->getTypeStr() << " to i32" << endl;
    out << "  " << getStr() << " = fptosi " << getFrom()->getTypeAndStr() << " to i32" << endl;
}

void PhiInstruction::dump(std::ostream& out) {

    out << "  " << getStr() << " = phi " << type->getStr() << " ";
    int i = 0;
    for(auto& [bb, use] : mIncomings) {
        out << "[ " << use->val->getStr() << ", %" << bb->getStr() << "]";
        if(i != mIncomings.size() - 1) {
            i++;
            out << ", ";
        } else
            out << endl;
    }
}

void SelectInstruction::dump(std::ostream& out) {
    out << " " << getStr() << " = select " << getExp()->getTypeAndStr() << ", " << getVal1()->getTypeAndStr() << ", "
        << getVal2()->getTypeAndStr() << endl;
}

void GetElementPtrInstruction::dump(std::ostream& out) {
    if(getBase()->type->isPtr())  // init.element
    {
        // out << "  " << reg->getStr() << " = getelementptr inbounds "
        //     << (dynamic_cast<PtrType*>(from->type.get()))->inner->getStr() << ", " << from->getTypeStr();
        out << "  " << getStr() << " = getelementptr inbounds " << (dynamic_cast<PtrType*>(getBase()->type))->inner->getStr()
            << ", " << getBase()->getTypeAndStr();
    } else {
        // out << "  " << reg->getStr() << " = getelementptr inbounds " << from->type->getStr() << ", " << from->getPointStr();
        out << "  " << getStr() << " = getelementptr inbounds " << getBase()->getTypeStr() << ", "
            << getBase()->getTypePointStr();
    }
    auto index = getIdx();
    for(const auto& ind : index)
        // out << ", " << ind->getTypeStr();
        out << ", " << ind->getTypeAndStr();
    out << endl;
}

void BinaryInstruction::dump(std::ostream& out) {
    // out << "  " << reg->getStr() << " = ";
    out << "  " << getStr() << " = ";
    auto a = getLHS();
    auto b = getRHS();
    if(a->type->isInt() || a->type->isBool()) {
        // switch(getOp()) {
        //     case '+':
        //         out << "add nsw ";
        //         break;
        //     case '-':
        //         out << "sub nsw ";
        //         break;
        //     case '*':
        //         out << "mul nsw ";
        //         break;
        //     case '/':
        //         out << "sdiv ";
        //         break;
        //     case '%':
        //         out << "srem ";
        //         break;
        //     case '!':
        //         out << "xor ";
        //         break;
        //     case ',':
        //         out << "shl ";
        //         break;
        //     case '.':
        //         out << "ashr ";
        //         break;
        //     default:
        //         out << "unknow op!";
        //         break;
        // }
        switch(op) {
            case BinaryInstructionOps::Add:
                out << "add nsw ";
                break;
            case BinaryInstructionOps::Sub:
                out << "sub nsw ";
                break;
            case BinaryInstructionOps::Mul:
                out << "mul nsw ";
                break;
            case BinaryInstructionOps::Div:
                out << "sdiv ";
                break;
            case BinaryInstructionOps::Rem:
                out << "srem ";
                break;
            case BinaryInstructionOps::Xor:
                out << "xor ";
                break;
            case BinaryInstructionOps::Shl:
                out << "shl ";
                break;
            case BinaryInstructionOps::AShr:
                out << "ashr ";
                break;
            case BinaryInstructionOps::FAdd:
                out << "add nsw ";
                break;
            case BinaryInstructionOps::FSub:
                out << "sub nsw ";
                break;
            case BinaryInstructionOps::FMul:
                out << "mul nsw ";
                break;
            case BinaryInstructionOps::FDiv:
                out << "sdiv ";
                break;
            case BinaryInstructionOps::Not:
                out << "xor ";
                break;
            default:
                assert(false && "unknow op!");
                break;
        }
    } else if(a->type->isFloat()) {
        // switch(getOp()) {
        //     case '+':
        //         out << "fadd ";
        //         break;
        //     case '-':
        //         out << "fsub ";
        //         break;
        //     case '*':
        //         out << "fmul ";
        //         break;
        //     case '/':
        //         out << "fdiv ";
        //         break;
        //     default:
        //         out << "unknow op!";
        //         break;
        // }
        switch(op) {
            case BinaryInstructionOps::Add:
                out << "fadd ";
                break;
            case BinaryInstructionOps::Sub:
                out << "fsub ";
                break;
            case BinaryInstructionOps::Mul:
                out << "fmul ";
                break;
            case BinaryInstructionOps::Div:
                out << "fdiv ";
                break;
            case BinaryInstructionOps::FAdd:
                out << "fadd ";
                break;
            case BinaryInstructionOps::FSub:
                out << "fsub ";
                break;
            case BinaryInstructionOps::FMul:
                out << "fmul ";
                break;
            case BinaryInstructionOps::FDiv:
                out << "fdiv ";
                break;
            default:
                assert(false && "unknow op!");
                break;
        }
    }

    out << a->type->getStr() << " " << a->getStr() << ", " << b->getStr() << endl;
}

void FnegInstruction::dump(std::ostream& out) {
    // out << "  " << reg->getStr() << " = fneg " << a->getTypeStr() << endl;
    out << "  " << getStr() << " = fneg " << getSrc()->getTypeAndStr() << endl;
}

void IcmpInstruction::dump(std::ostream& out) {
    // out << "  " << reg->getStr() << " = icmp ";
    out << "  " << getStr() << " = icmp ";
    // if(op == "!=") {
    //     out << "ne ";
    // } else if(op == ">") {
    //     out << "sgt ";
    // } else if(op == ">=") {
    //     out << "sge ";
    // } else if(op == "<") {
    //     out << "slt ";
    // } else if(op == "<=") {
    //     out << "sle ";
    // } else if(op == "==") {
    //     out << "eq ";
    // }
    switch(kind) {
        case IcmpKind::ICmpEQ:
            out << "eq ";
            break;
        case IcmpKind::ICmpNE:
            out << "ne ";
            break;
        case IcmpKind::ICmpSLT:
            out << "slt ";
            break;
        case IcmpKind::ICmpSLE:
            out << "sle ";
            break;
        case IcmpKind::ICmpSGT:
            out << "sgt ";
            break;
        case IcmpKind::ICmpSGE:
            out << "sge ";
            break;
        default:
            break;
    }
    // out << a->getTypeStr() << ", " << b->getStr() << endl;
    out << getLHS()->getTypeAndStr() << ", " << getRHS()->getStr() << endl;
}

void FcmpInstruction::dump(std::ostream& out) {
    // out << "  " << reg->getStr() << " = fcmp ";
    out << "  " << getStr() << " = fcmp ";
    // if(op == "!=") {
    //     out << "one ";
    // } else if(op == ">") {
    //     out << "ogt ";
    // } else if(op == ">=") {
    //     out << "oge ";
    // } else if(op == "<") {
    //     out << "olt ";
    // } else if(op == "<=") {
    //     out << "ole ";
    // } else if(op == "==") {
    //     out << "oeq ";
    // }
    switch(kind) {
        case IcmpKind::ICmpEQ:
            out << "oeq ";
            break;
        case IcmpKind::ICmpSGT:
            out << "ogt ";
            break;
        case IcmpKind::ICmpSGE:
            out << "oge ";
            break;
        case IcmpKind::ICmpSLT:
            out << "olt ";
            break;
        case IcmpKind::ICmpSLE:
            out << "ole ";
            break;
        case IcmpKind::ICmpNE:
            out << "one ";
            break;
        default:
            assert(false && "unknow fcmp kind");
    }

    // out << a->getTypeStr() << ", " << b->getStr() << endl;
    out << getLHS()->getTypeAndStr() << ", " << getRHS()->getStr() << endl;
}

void BrInstruction::dump(std::ostream& out) {
    auto cond = getCondition();
    if(cond)
        // out << "  br " << exp->getTypeStr() << ", " << label1->getStr() << ", " << label2->getStr() << endl;
        out << "  br " << cond->getTypeAndStr() << ", " << "label %" + getTrueTarget()->getStr() << ", "
            << "label %" + getFalseTarget()->getStr() << endl;
    else
        // out << "  br " << label1->getStr() << endl;
        out << "  br " << "label %" + getTrueTarget()->getStr() << endl;
}

const TypePtr GetElementPtrInstruction::getValueType(ValuePtr base, const std::vector<ValuePtr>& indices) {
    // assert(base->getType()->isPtr());
    if(indices.size() == 1)
        return base->getType();
    else {
        auto curr = base->getType();
        for(int i = 1; i < indices.size(); i++) {
            // assert(curr->isArr());
            if(curr->isPtr())
                curr = dynamic_cast<PtrType*>(curr)->inner;
            else
                curr = dynamic_cast<ArrType*>(curr)->inner;
        }
        return curr;
    }
}

void LoopGEPInstruction::dump(std::ostream& out) {}

// void replaceVarByVarForLCSSA(ValuePtr from, ValuePtr to, Use* use) {
//     // from已经不会再使用，所以没必要维护from的Use
//     int cnt = 0;
//     Instruction* la = nullptr;
//     unordered_map<Instruction*, bool> mp;
//
//     auto user = use->user;
//     int numOperand = user->getNumOperands();
//     if(!user->replaceValue(from, to)) {
//         cerr << user->reg->name << " error " << user->type << "\n";
//         // assert(false && "error when replaceVarByVar");
//     }
//     use->rmUse();
//     newUse(to.get(), user);
//     deleteUser(from);
// }
//

// void PhiInstruction::removeIncomingByBB(shared_ptr<BasicBlock> bb) {
//     int indexToRemove = -1;
//     for(int i = 0; i < from.size(); i++) {
//         auto predUse = from.at(i);
//         if(predUse.second == bb)
//             indexToRemove = i;
//     }
//
//     if(indexToRemove >= 0) {
//         auto use1 = from.at(indexToRemove).first;
//         deleteUser(use1);
//         from.erase(from.begin() + indexToRemove);
//         //incoming.erase(bb.get());
//         // for(auto i = 0; i < operands.size(); i++)
//         //     operands.at(i)->reSetPos(i);
//     }
// }

// void GetElementPtrInstruction::clearAllIndex() {
//     base = nullptr;
//     allIndex.clear();
// }
//
// void GetElementPtrInstruction::calulateAllIndex() {
//     assert(index.size() > 0);
//     if(index.size() == 1) {
//         allIndex.emplace_back(index[0]);
//     } else {
//         for(int i = 1; i < index.size(); i++)
//             allIndex.emplace_back(index[i]);
//     }
//
//     if(auto innerGEP = dynamic_cast<GetElementPtrInstruction*>(from->I); innerGEP) {
//         for(auto& i : innerGEP->allIndex)
//             allIndex.emplace_back(i);
//         base = innerGEP->base;
//     } else if(from->type->isArr()) {
//         base = from;
//     }
//     // if(from->I)
//     //     from->I->print();
//     // print();
//     // fflush(stdout);
//     // printf("base: %s, allIndex: ", base ? base->getStr().c_str(): "");
//     // for(int i = 0; i < allIndex.size(); i ++)
//     //     printf("%s, ", allIndex[i]->getStr().c_str());
//     // printf("\n");
// }

int LoopGEPInstruction::getInitOffset() {
    return getStepOffset() * (initVal - 1);
}

int LoopGEPInstruction::getStepOffset() {
    //    //     if(gep->index.size() == 1)
    //    //         return stepVal;
    //    int i = 0;
    //    int stepOffset = 1;
    //    auto type = gep->from->type.get();
    //    // auto arrayType = dynamic_cast<ArrType *>(gep->from->type.get());
    //    while(i < gep->index.size() && index != gep->index[i]) {
    //        auto arrayType = dynamic_cast<ArrType*>(type);
    //        assert(arrayType);
    //        type = arrayType->inner.get();
    //        i++;
    //    }
    //    if(auto arrayType = dynamic_cast<ArrType*>(type); arrayType)
    //        return arrayType->getSize() * stepVal;
    //    if(auto ptrType = dynamic_cast<PtrType*>(type); ptrType) {
    //        if(auto arrayType = dynamic_cast<ArrType*>(ptrType->inner.get()); arrayType)
    //            return arrayType->getSize() * stepVal;
    //        return stepVal;
    //    }
    //    return stepVal;
    // FIXME:
    return 0;
}
//// NOLINTEND
