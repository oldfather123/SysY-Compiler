#include "Instruction.h"

int CallInstruction::callRegNum = 0;
int BinaryInstruction::BinaryRegNum = 0;
int FnegInstruction::FnegRegNum = 0;
int GetElementPtrInstruction::arrayIdxNum = 0;
int GetElementPtrInstruction::arrayElementNum = 0;
int IcmpInstruction::cmpRegNum = 0;
int PhiInstruction::phiRegNum = 0;
int BrInstruction::ifThenNum = 0;
int BrInstruction::ifElseNum = 0;
int BrInstruction::ifEndNum = 0;
int BrInstruction::orNum = 0;
int BrInstruction::andNum = 0;
int BrInstruction::whileCondNum = 0;
int BrInstruction::whileBodyNum = 0;
int BrInstruction::whileEndNum = 0;
int ExtInstruction::extCount = 0;
int SignToFloatInstruction::convNum = 0;
int SelectInstruction::selRegNum = 0;
map<string, IcmpKind> IcmpInstruction::kindMap{{"==", ICmpEQ}, {"!=", ICmpNE}, {"<", ICmpSLT},
                             {"<=", ICmpSLE}, {">", ICmpSGT}, {">=", ICmpSGE}};


void Instruction::moveBefore(shared_ptr<Instruction> targetIns, shared_ptr<Instruction> beforeIns){
    if(beforeIns == targetIns){
        return;
    }
    auto bb = beforeIns->basicblock;
    basicblock->removeInstruction(targetIns);
    bb->insertInstructionBefore(targetIns, beforeIns);
}

void Instruction::insertBefore(shared_ptr<Instruction> targetIns, shared_ptr<Instruction> beforeIns) {
    auto& instructions = beforeIns->basicblock->instructions;
    auto curInstruction = std::find_if(instructions.begin(), instructions.end(),
        [beforeIns](const InstructionPtr& instr) { return instr == beforeIns; });

    // 确保找到了插入点
    assert(curInstruction != instructions.end());
    
    // 插入指令
    instructions.emplace(curInstruction, targetIns);
    targetIns->basicblock = beforeIns->basicblock;
}



void Instruction::setRegisterName(string newName)
{
    if (reg) {
        reg->name = newName;
    }
    else {
        assert(false && "Attempt to set name on an instruction without a register");
    }
}

string Instruction::getRegisterName()
{
    if (reg) {
        return reg->name;
    }
    else {
        assert(false && "Attempt to get name from an instruction without a register");
    }
}

vector<shared_ptr<Instruction>>::iterator Instruction::getIterator() {
    auto it = std::find_if(
        basicblock->instructions.begin(), 
        basicblock->instructions.end(), 
        [this](const shared_ptr<Instruction>& instr) { return instr.get() == this; }
    );
    
    if (it != basicblock->instructions.end()) {
        return it;
    }
    
    assert(false && "Failed to find iterator for instruction in basic block");
    return basicblock->instructions.end(); // Unreachable, but avoids compiler warning
}

void Instruction::removeFromParent() {
    auto it = std::find_if(
        basicblock->instructions.begin(), 
        basicblock->instructions.end(), 
        [this](const shared_ptr<Instruction>& instr) { return instr.get() == this; }
    );
    
    if (it != basicblock->instructions.end()) {
        basicblock->instructions.erase(it);
    }
    else {
        assert(false && "Failed to delete instruction from basic block");
    }
}

BinaryInstruction::BinaryInstruction(ValuePtr a, ValuePtr b, char op, Instruction *I) : Instruction{InsID::Binary, I->basicblock, getBinaryReg(a->type)}, a{a}, b{b}, op{op}
{
    newUse(a.get(), this, reg.get());
    newUse(b.get(), this, reg.get());
    reg->I = this;
    if (op == '!')
        reg->type = Type::getBool();

    if (a->type->isInt() && b->type->isInt() && (op == '+' || op == '*' || op == '!')) {
        isCommutative = true;
        isAssociative = true;
    }
}


CallInstruction::CallInstruction(shared_ptr<Function> func, vector<ValuePtr> argv, shared_ptr<BasicBlock> bb) : Instruction{InsID::Call, bb, getCallReg(func->returnValue->type)}, func{func}, argv{argv}
{
    for (const auto& arg : argv) {
        newUse(arg.get(), this);
    }
    reg->I = this;
}

void ReturnInstruction::print()
{
    cout << "  ret " << retValue->returnTypeToString() << endl;
}

void AllocaInstruction::print()
{
    cout << "  " << des->toString() << " = alloca " << des->type->toString() << endl;
}

void StoreInstruction::print()
{
    if (gep) {
        cout << "  store " << value->returnTypeToString() << ", " << des->type->toString() << "* getelementptr inbounds (" << gep->from->type->toString() << ", " << gep->from->pointerTypeToString();
        for (auto index : gep->index)
            cout << ", " << index->returnTypeToString();
        cout << ")" << endl;
    }
    else {
        cout << "  store " << value->returnTypeToString() << ", " << des->pointerTypeToString() << endl;
    }
}

void LoadInstruction::print()
{
    if (gep) {
        cout << "  " << to->toString() << " = load " << from->type->toString() << ", " << from->type->toString() << "* getelementptr inbounds (" << gep->from->type->toString() << ", " << gep->from->pointerTypeToString();
        for (auto index : gep->index)
            cout << ", " << index->returnTypeToString();
        cout << ")" << endl;
    }
    else {
        cout << "  " << to->toString() << " = load " << from->type->toString() << ", " << from->pointerTypeToString() << endl;
    }
}

void BitCastInstruction::print()
{
    cout << "  " << reg->toString() << " = bitcast " << from->pointerTypeToString() << " to " << targetType->toString() << endl;
}

void CallInstruction::print()
{
    cout << "  ";
    if (!func->returnValue->type->isVoid()) {
        cout << reg->toString() << " = ";
    }
    cout << "call " << func->returnTypeToString() << "(";
    for (int i = 0; i < argv.size(); i++) {
        if (i > 0) {
            cout << ", ";
        }
        cout << argv[i]->returnTypeToString();
    }
    cout << ")" << endl;
}

void ExtInstruction::print()
{
    cout << "  " << reg->toString() << " = " << (isSigned ? "s" : "z") << "ext " << from->returnTypeToString() << " to " << to->toString() << endl;
}

void SignToFloatInstruction::print()
{
    cout << "  " << reg->toString() << " = sitofp " << from->returnTypeToString() << " to float" << endl;
}

void FloatToSignInstruction::print()
{
    cout << "  " << reg->toString() << " = fptosi " << from->returnTypeToString() << " to i32" << endl;
}

void PhiInstruction::print()
{
    cout << "  " << reg->toString() << " = phi " << val->type->toString() << " ";
    for (int i = 0; i < from.size(); i++) {
        cout << "[ " << from[i].first->toString() << ", %" << from[i].second->label->name << "]";
        if (i < from.size() - 1) {
            cout << ", ";
        }
    }
    cout << endl;
}

void SelectInstruction::print(){
    cout<<"  "<<reg->toString()<<" = select "<<exp->returnTypeToString()<<", "<<val1->returnTypeToString()<<", "<<val2->returnTypeToString()<<endl; 
}

void GetElementPtrInstruction::print()
{
    cout << "  " << reg->toString() << " = getelementptr inbounds ";
    if (from->type->isPtr()) {
        cout << ((PtrType *)(from->type.get()))->inner->toString() << ", " << from->returnTypeToString();
    }
    else {
        cout << from->type->toString() << ", " << from->pointerTypeToString();
    }
    for (auto index : index) {
        cout << ", " << index->returnTypeToString();
    }
    cout << endl;
}

void BinaryInstruction::print()
{
    cout << "  " << reg->toString() << " = ";
    if (a->type->isInt() || a->type->isBool()) {
        switch (op) {
            case '+': cout << "add nsw "; break;
            case '-': cout << "sub nsw "; break;
            case '*': cout << "mul nsw "; break;
            case '/': cout << "sdiv "; break;
            case '%': cout << "srem "; break;
            case '!': cout << "xor "; break;
            case ',': cout << "shl "; break;
            case '.': cout << "ashr "; break;
            default: cout << "unknow op!"; break;
        }
    }
    else if (a->type->isFloat()) {
        switch (op) {
            case '+': cout << "fadd "; break;
            case '-': cout << "fsub "; break;
            case '*': cout << "fmul "; break;
            case '/': cout << "fdiv "; break;
            default: cout << "unknow op!"; break;
        }
    }
    cout << a->type->toString() << " " << a->toString() << ", " << b->toString() << endl;
}

void FnegInstruction::print()
{
    cout << "  " << reg->toString() << " = fneg " << a->returnTypeToString() << endl;
}

void IcmpInstruction::print()
{
    cout << "  " << reg->toString() << " = icmp ";
    if (op == "!=") {
        cout << "ne ";
    }
    else if (op == ">") {
        cout << "sgt ";
    }
    else if (op == ">=") {
        cout << "sge ";
    }
    else if (op == "<") {
        cout << "slt ";
    }
    else if (op == "<=") {
        cout << "sle ";
    }
    else if (op == "==") {
        cout << "eq ";
    }
    cout << a->returnTypeToString() << ", " << b->toString() << endl;
}

void FcmpInstruction::print()
{
    cout << "  " << reg->toString() << " = fcmp ";
    if (op == "!=") {
        cout << "one ";
    }
    else if (op == ">") {
        cout << "ogt ";
    }
    else if (op == ">=") {
        cout << "oge ";
    }
    else if (op == "<") {
        cout << "olt ";
    }
    else if (op == "<=") {
        cout << "ole ";
    }
    else if (op == "==") {
        cout << "oeq ";
    }
    cout << a->returnTypeToString() << ", " << b->toString() << endl;
}

void BrInstruction::print()
{
    if (exp) {
        cout << "  br " << exp->returnTypeToString() << ", " << label1->toString() << ", " << label2->toString() << endl;
    }
    else {
        cout << "  br " << label1->toString() << endl;
    }
}

void FuncCastInstruction::print(){
    // 打印出来的意义只是调试
    cout << "  function cast " << funcName << " to i8*" << endl;
}

void AtomicAddInstruction::print(){
    cout << "  atomicadd " << desVar->pointerTypeToString() << ", " << addValue->returnTypeToString() << endl;
}

bool ReturnInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    if (retValue == target) {
        retValue = newValue;
        return true;
    }
    return false;
}

bool AllocaInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    return false;
}

bool StoreInstruction::replaceOperand(ValuePtr target, ValuePtr newValue) {
    bool replaced = false;
    if (gep && gep->replaceOperand(target, newValue)) {
        replaced = true;
    }
    if (value == target) {
        value = std::move(newValue);
        return true;
    }
    if (des == target) {
        des = std::move(newValue);
        return true;
    }
    return replaced;
}

bool LoadInstruction::replaceOperand(ValuePtr target, ValuePtr newValue) {
    bool replaced = false;
    if (gep && gep->replaceOperand(target, newValue)) {
        replaced = true;
    }
    if (from == target) {
        from = std::move(newValue);
        return true;
    }
    return replaced;
}

bool BitCastInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    if (from == target) {
        from = newValue;
        return true;
    }
    return false;
}

bool CallInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    bool replaced = false;
    for (auto& arg : argv) {
        if (arg == target) {
            arg = std::move(newValue);
            replaced = true;
        }
    }
    return replaced;
}

bool ExtInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    if (from == target) {
        from = newValue;
        return true;
    }
    return false;
}

bool SignToFloatInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    if (from == target) {
        from = newValue;
        return true;
    }
    return false;
}

bool FloatToSignInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    if (from == target) {
        from = newValue;
        return true;
    }
    return false;
}

bool PhiInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    bool flag = false;
    for (int i = 0; i < from.size(); i++) {
        if (from[i].first == target) {
            from[i].first = newValue;
            flag = true;
        }
    }
    return flag;
}

bool SelectInstruction::replaceOperand(ValuePtr target, ValuePtr newValue) {
    bool replaced = false;
    if (exp == target) {
        exp = std::move(newValue);
        replaced = true;
    }
    if (val1 == target) {
        val1 = std::move(newValue);
        replaced = true;
    }
    if (val2 == target) {
        val2 = std::move(newValue);
        replaced = true;
    }
    return replaced;
}

bool GetElementPtrInstruction::replaceOperand(ValuePtr target, ValuePtr newValue) {
    bool replaced = false;
    if (from == target) {
        from = std::move(newValue);
        replaced = true;
    }
    for (auto& idx : index) {
        if (idx == target) {
            idx = std::move(newValue);
            replaced = true;
        }
    }
    return replaced;
}

bool BinaryInstruction::replaceOperand(ValuePtr target, ValuePtr newValue) {
    bool replaced = false;
    if (a == target) {
        a = std::move(newValue);
        replaced = true;
    }
    else if (b == target) {
        b = std::move(newValue);
        replaced = true;
    }
    return replaced;
}

bool FnegInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    if (a == target) {
        a = newValue;
        return true;
    }

    return false;
}

bool IcmpInstruction::replaceOperand(ValuePtr target, ValuePtr newValue) {
    bool replaced = false;
    if (a == target) {
        a = std::move(newValue);
        replaced = true;
    }
    else if (b == target) {
        b = std::move(newValue);
        replaced = true;
    }
    return replaced;
}

bool FcmpInstruction::replaceOperand(ValuePtr target, ValuePtr newValue) {
    bool replaced = false;
    if (a == target) {
        a = std::move(newValue);
        replaced = true;
    }
    else if (b == target) {
        b = std::move(newValue);
        replaced = true;
    }
    return replaced;
}

bool BrInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    if (exp == target) {
        exp = newValue;
        return true;
    }
    return false;
}

bool FuncCastInstruction::replaceOperand(ValuePtr target, ValuePtr newValue){
    // 不需要replace
    return false;
}

bool AtomicAddInstruction::replaceOperand(ValuePtr target, ValuePtr newValue)
{
    if (addValue == target) {
        addValue = newValue;
        return true;
    }
    else if(desVar == target){
        // 不允许这种转换
        return false;
    }
    return false;
}

void removeUserFromOperands(Instruction* I) {
    int numOperands = I->getOperandCount();
    for (int i = 0; i < numOperands; i++) {
        auto operand = I->getOperand(i);
        Use* use = operand->useHead;
        while (use) {
            if (use->user == I) {
                use->rmUse();
            }
            use = use->next;
        }
    }
}

void deleteUser(ValuePtr user) {
    if (auto I = user->I) {
        removeUserFromOperands(I);
    }
}

void deleteUser(InstructionPtr user) {
    if (user) {
        removeUserFromOperands(user.get());
    }
}

void deleteUser(Instruction* user) {
    if (user) {
        removeUserFromOperands(user);
    }
}

void replaceUses(ValuePtr from, ValuePtr to) {
    Use* currentUse = from->useHead;
    while (currentUse != nullptr) {
        auto user = currentUse->user;
        if (!user->replaceOperand(from, to)) {
            std::cerr << user->reg->name << " error " << user->type << "\n";
        }
        auto nextUse = currentUse->next;
        currentUse->rmUse();
        newUse(to.get(), user);
        currentUse = nextUse;
    }
    deleteUser(from);
}

void replaceVarByVar(ValuePtr from, ValuePtr to) {
    replaceUses(from, to);
}

void replaceVarByVarPointer(ValuePtr from, ValuePtr to) {
    replaceUses(from, to);
}

void replaceVarByVarForLCSSA(ValuePtr from, ValuePtr to, Use* use) {
    auto user = use->user;
    if (!user->replaceOperand(from, to)) {
        std::cerr << user->reg->name << " error " << user->type << "\n";
    }
    use->rmUse();
    newUse(to.get(), user);
    deleteUser(from);
}

unsigned ReturnInstruction::getOperandCount()
{
    return 1;
}
unsigned AllocaInstruction::getOperandCount()
{
    return 1;
}
unsigned GetElementPtrInstruction::getOperandCount()
{
    return 1 + index.size();
}
unsigned StoreInstruction::getOperandCount()
{
    return 2;
}
unsigned LoadInstruction::getOperandCount()
{
    return 1;
}
unsigned BitCastInstruction::getOperandCount()
{
    return 1;
}
unsigned ExtInstruction::getOperandCount()
{
    return 1;
}
unsigned SignToFloatInstruction::getOperandCount()
{
    return 1;
}
unsigned FloatToSignInstruction::getOperandCount()
{
    return 1;
}
unsigned CallInstruction::getOperandCount()
{
    return argv.size();
}
unsigned BinaryInstruction::getOperandCount()
{
    return 2;
}
unsigned FnegInstruction::getOperandCount()
{
    return 1;
}
unsigned IcmpInstruction::getOperandCount()
{
    return 2;
}
unsigned FcmpInstruction::getOperandCount()
{
    return 2;
}
unsigned BrInstruction::getOperandCount()
{
    if (exp != nullptr)
        return 1;
    else
        return 0;
}
unsigned PhiInstruction::getOperandCount()
{
    return from.size();
}
unsigned SelectInstruction::getOperandCount()
{
    return 3;
}

unsigned FuncCastInstruction::getOperandCount(){
    return 0;
}

unsigned AtomicAddInstruction::getOperandCount(){
    return 1; // 因为保证了变量是全局变量，所以我们这里不分析
}

ValuePtr ReturnInstruction::getOperand(unsigned i)
{
    assert(i == 0 && "returnInstruction getOperand Error\n");
    return retValue;
}
ValuePtr AllocaInstruction::getOperand(unsigned i)
{
    assert(i == 0 && "allocaInstruction getOperand Error\n");
    return des;
}
ValuePtr GetElementPtrInstruction::getOperand(unsigned i)
{
    assert(i <= index.size() && "GetElementPtrInstruction getOperand Error\n");
    if (i == 0) {
        return from;
    }
    else {
        return index[i - 1];
    }
}
ValuePtr StoreInstruction::getOperand(unsigned i)
{
    assert(i < 2 && "StoreInstruction getOperand Error\n");
    if (i == 0) {
        return des;
    }
    else {
        return value;
    }
}
ValuePtr LoadInstruction::getOperand(unsigned i)
{
    assert(i == 0 && "LoadInstruction getOperand Error\n");
    return from;
}
ValuePtr BitCastInstruction::getOperand(unsigned i)
{
    assert(i == 0 && "BitCastInstruction getOperand Error\n");
    return from;
}
ValuePtr ExtInstruction::getOperand(unsigned i)
{
    assert(i == 0 && "ExtInstruction getOperand Error\n");
    return from;
}
ValuePtr SignToFloatInstruction::getOperand(unsigned i)
{
    assert(i == 0 && "SignToFloatInstruction getOperand Error\n");
    return from;
}

ValuePtr FloatToSignInstruction::getOperand(unsigned i)
{
    assert(i == 0 && "FloatToSignInstruction getOperand Error\n");
    return from;
}
ValuePtr CallInstruction::getOperand(unsigned i)
{
    assert(i < argv.size() && "CallInstruction getOperand Error\n");
    return argv[i];
}
ValuePtr BinaryInstruction::getOperand(unsigned i)
{
    assert(i < 2 && "BinaryInstruction getOperand Error\n");
    if (i == 0) {
        return a;
    }
    else {
        return b;
    }
}
ValuePtr FnegInstruction::getOperand(unsigned i)
{
    assert(i == 0 && "FnegInstruction getOperand Error\n");
    return a;
}

ValuePtr IcmpInstruction::getOperand(unsigned i)
{
    assert(i < 2 && "IcmpInstruction getOperand Error\n");
    if (i == 0) {
        return a;
    }
    else {
        return b;
    }
}
ValuePtr FcmpInstruction::getOperand(unsigned i)
{
    assert(i < 2 && "FcmpInstruction getOperand Error\n");
    if (i == 0) {
        return a;
    }
    else {
        return b;
    }
}
ValuePtr BrInstruction::getOperand(unsigned i)
{
    assert(i == 0 && "BrInstruction getOperand Error\n");
    return exp;
}
ValuePtr PhiInstruction::getOperand(unsigned i)
{
    assert(i < from.size() && "PhiInstruction getOperand Error\n");

    return from[i].first;
}
ValuePtr SelectInstruction::getOperand(unsigned i)
{
    assert(i < 3 && "FcmpInstruction getOperand Error\n");
    if (i == 0) {
        return exp;
    }
    else if (i==1) {
        return val1;
    }
    else {
        return val2;
    }
}

ValuePtr FuncCastInstruction::getOperand(unsigned i){
    assert(false && "FuncCastInstruction has no operand.");
    return nullptr;
}

ValuePtr AtomicAddInstruction::getOperand(unsigned i){
    assert(i==0 && "AtomicAddInstruction getOperand Error\n");
    return addValue;
}

void PhiInstruction::removeIncomingByBB(shared_ptr<BasicBlock> bb) {
    auto it = std::find_if(from.begin(), from.end(), 
                           [&bb](const std::pair<ValuePtr, shared_ptr<BasicBlock>>& pair) {
                               return pair.second == bb;
                           });

    if (it != from.end()) {
        auto use = it->first;
        deleteUser(use);
        from.erase(it);
    }
}

ReturnInstruction::~ReturnInstruction(){
    deleteUser(this);
}
AllocaInstruction::~AllocaInstruction(){
    deleteUser(this);
}
GetElementPtrInstruction::~GetElementPtrInstruction(){
    deleteUser(this);
}
StoreInstruction::~StoreInstruction(){
    deleteUser(this);
}
LoadInstruction::~LoadInstruction(){
    deleteUser(this);
}
BitCastInstruction::~BitCastInstruction(){
    deleteUser(this);
}
ExtInstruction::~ExtInstruction(){
    deleteUser(this);
}
SignToFloatInstruction::~SignToFloatInstruction(){
    deleteUser(this);
}
FloatToSignInstruction::~FloatToSignInstruction(){
    deleteUser(this);
}
CallInstruction::~CallInstruction(){
    deleteUser(this);
}
BinaryInstruction::~BinaryInstruction(){
    deleteUser(this);
}
FnegInstruction::~FnegInstruction(){
    deleteUser(this);
}
IcmpInstruction::~IcmpInstruction(){
    deleteUser(this);
}
FcmpInstruction::~FcmpInstruction(){
    deleteUser(this);
}
BrInstruction::~BrInstruction(){
    deleteUser(this);
}
PhiInstruction::~PhiInstruction(){
    deleteUser(this);
}
SelectInstruction::~SelectInstruction(){
    deleteUser(this);
}
FuncCastInstruction::~FuncCastInstruction(){
    // 没有use
}

AtomicAddInstruction::~AtomicAddInstruction(){
    deleteUser(this);
}

void GetElementPtrInstruction::clearAllIndex() {
    base = nullptr;
    allIndex.clear();
}

void GetElementPtrInstruction::calculateAllIndex() {
    assert(index.size() > 0);
    if(index.size() == 1) {
        allIndex.emplace_back(index.front());
    }
    else {
        allIndex.insert(allIndex.end(), index.begin() + 1, index.end());
    }

    if(auto innerGEP = dynamic_cast<GetElementPtrInstruction *>(from->I); innerGEP) {
        allIndex.insert(allIndex.end(), innerGEP->allIndex.begin(), innerGEP->allIndex.end());
        base = innerGEP->base;
    }
    else if(from->type->isArr()) {
        base = from;
    }
}


void insertInstructionBefore(InstructionPtr instruction, Instruction* InsertBefore) {
    auto& instructions = InsertBefore->basicblock->instructions;
    auto curInstruction = std::find_if(instructions.begin(), instructions.end(),
    [&InsertBefore](InstructionPtr& instr) { 
        return instr.get() == InsertBefore; 
    });

    // 确保找到了插入点
    assert(curInstruction != instructions.end());
    
    // 插入指令
    instructions.emplace(curInstruction, instruction);
    instruction->basicblock = InsertBefore->basicblock;
}

InstructionPtr createBinaryBefore(ValuePtr op1, ValuePtr op2, char op, InstructionPtr insBefore){
    switch(op){
        case '+':{
            auto addIns = InstructionPtr(new BinaryInstruction(op1, op2, '+', insBefore.get()));
            insertInstructionBefore(addIns, insBefore.get());
            return addIns; 
        }
        case '-':{
            if(op1->type->isFloat()){
                auto negIns =  InstructionPtr(new FnegInstruction(op1, insBefore->basicblock)); 
                insertInstructionBefore(negIns, insBefore.get());
                return negIns;
            }else if(op1->type->isInt()){
                auto negIns =  InstructionPtr(new BinaryInstruction(Const::createConstant(Type::getInt(), int(0)), op1, '-', insBefore.get())); 
                insertInstructionBefore(negIns, insBefore.get());
                return negIns;
            }
        }
        case '*':{
            auto mulIns = InstructionPtr(new BinaryInstruction(op1, op2, '*', insBefore.get()));
            insertInstructionBefore(mulIns, insBefore.get());
            return mulIns;
        }
        default:
            return nullptr;
    }
}

// 是否是0 - x的格式
bool isFneg(BinaryInstruction* bi){
    if (bi == nullptr) {
        return false;
    }
    auto op0 = dynamic_cast<Const*>(bi->getOperand(0).get());

    if(op0 && ((bi->getOpcode() == Sub && op0->type->isInt() && op0->intVal == 0) || (bi->getOpcode() == FSub && op0->type->isFloat() && op0->floatVal == 0))){
        return true;
    }

    return false;
}

bool mayBeDeadCode(Instruction* Ins){
    if((!Ins->reg || Ins->reg->useHead) || (Ins->type == Call && ((!dynamic_cast<CallInstruction*>(Ins)->func->isReentrant) || dynamic_cast<CallInstruction*>(Ins)->func->isLibraryFunction))){
        return false;
    }
    return true;
}