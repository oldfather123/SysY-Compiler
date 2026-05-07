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
int ExtInstruction::extNum = 0;
int SitofpInstruction::convNum = 0;

void Instruction::deleteSelfInBB()
{
    for (auto it = basicblock->instructions.begin(); it != basicblock->instructions.end(); it++)
    {
        if ((*it).get() == this)
        {
            basicblock->instructions.erase(it);
        }
    }
}

void ReturnInstruction::print()
{
    cout << "  ret " << retValue->getTypeStr() << endl;
}

void AllocaInstruction::print()
{
    cout << "  " << des->getStr() << " = alloca " << des->type->getStr() << endl;
}

void StoreInstruction::print()
{
    if (gep)
    {
        cout << "  store " << value->getTypeStr() << ", " << des->type->getStr() << "* getelementptr inbounds (" << gep->from->type->getStr() << ", " << gep->from->getPointStr();
        for (auto ind : gep->index)
            cout << ", " << ind->getTypeStr();
        cout << ")" << endl;
    }
    else
    {
        cout << "  store " << value->getTypeStr() << ", " << des->getPointStr() << endl;
    }
}

void LoadInstruction::print()
{
    if (gep)
    {
        cout << "  " << to->getStr() << " = load " << from->type->getStr() << ", " << from->type->getStr() << "* getelementptr inbounds (" << gep->from->type->getStr() << ", " << gep->from->getPointStr();
        for (auto ind : gep->index)
            cout << ", " << ind->getTypeStr();
        cout << ")" << endl;
    }
    else
    {
        cout << "  " << to->getStr() << " = load " << from->type->getStr() << ", " << from->getPointStr() << endl;
    }
}

void BitCastInstruction::print()
{
    cout << "  " << reg->getStr() << " = bitcast " << from->getPointStr() << " to " << toType->getStr() << endl;
}

void CallInstruction::print()
{
    cout << "  ";
    if (!func->retVal->type->isVoid())
        cout << reg->getStr() << " = ";
    cout << "call " << func->getTypeStr() << "(";
    for (int i = 0; i < argv.size(); i++)
    {
        if (i)
            cout << ", ";
        cout << argv[i]->getTypeStr();
    }
    cout << ")" << endl;
}

void ExtInstruction::print()
{
    cout << "  " << reg->getStr() << " = " << (isign ? "s" : "z") << "ext " << from->getTypeStr() << " to " << to->getStr() << endl;
}

void SitofpInstruction::print()
{
    cout << "  " << reg->getStr() << " = sitofp " << from->getTypeStr() << " to float" << endl;
}

void FptosiInstruction::print()
{
    cout << "  " << reg->getStr() << " = fptosi " << from->getTypeStr() << " to i32" << endl;
}

void PhiInstruction::print()
{

    cout << "  " << reg->getStr() << " = phi " << val->type->getStr() << " ";

    for (int i = 0; i < from.size(); i++)
    {
        cout << "[ " << from[i].first->getStr() << ", %" << from[i].second->label->name << "]";
        if (i != from.size() - 1)
        {
            cout << ", ";
        }
        else
        {
            cout << endl;
        }
    }
}

CallInstruction::CallInstruction(shared_ptr<Function> func, vector<ValuePtr> argv, shared_ptr<BasicBlock> bb) : Instruction{InsID::Call, bb}, func{func}, argv{argv}, reg{getCallReg(func->retVal->type)}
{
    for (int i = 0; i < argv.size(); i++)
    {
        newUse(argv[i].get(), this);
    }
    //reg即这条指令本身
    reg->I = this;
}

void GetElementPtrInstruction::print()
{
    if(from->type->isPtr()){
        cout << "  " << reg->getStr() << " = getelementptr inbounds " << ((PtrType*)(from->type.get()))->inner->getStr() << ", " << from->getTypeStr();
    }
    else{
        cout << "  " << reg->getStr() << " = getelementptr inbounds " << from->type->getStr() << ", " << from->getPointStr();
    }
    for (auto ind : index)
        cout << ", " << ind->getTypeStr();
    cout << endl;
}

void BinaryInstruction::print()
{
    cout << "  " << reg->getStr() << " = ";
    if (a->type->isInt() || a->type->isBool())
    {
        switch (op)
        {
        case '+':
            cout << "add nsw ";
            break;
        case '-':
            cout << "sub nsw ";
            break;
        case '*':
            cout << "mul nsw ";
            break;
        case '/':
            cout << "sdiv ";
            break;
        case '%':
            cout << "srem ";
            break;
        case '!':
            cout << "xor ";
            break;
        default:
            cout << "unknow op!";
            break;
        }
    }
    else if (a->type->isFloat())
    {
        switch (op)
        {
        case '+':
            cout << "fadd ";
            break;
        case '-':
            cout << "fsub ";
            break;
        case '*':
            cout << "fmul ";
            break;
        case '/':
            cout << "fdiv ";
            break;
        default:
            cout << "unknow op!";
            break;
        }
    }

    cout << a->type->getStr() << " " << a->getStr() << ", " << b->getStr() << endl;
}

void FnegInstruction::print()
{
    cout << "  " << reg->getStr() << " = fneg " << a->getTypeStr() << endl;
}

void IcmpInstruction::print()
{
    cout << "  " << reg->getStr() << " = icmp ";
    if (op == "!=")
    {
        cout << "ne ";
    }
    else if (op == ">")
    {
        cout << "sgt ";
    }
    else if (op == ">=")
    {
        cout << "sge ";
    }
    else if (op == "<")
    {
        cout << "slt ";
    }
    else if (op == "<=")
    {
        cout << "sle ";
    }
    else if (op == "==")
    {
        cout << "eq ";
    }
    cout << a->getTypeStr() << ", " << b->getStr() << endl;
}

void FcmpInstruction::print()
{
    cout << "  " << reg->getStr() << " = fcmp ";
    if (op == "!=")
    {
        cout << "one ";
    }
    else if (op == ">")
    {
        cout << "ogt ";
    }
    else if (op == ">=")
    {
        cout << "oge ";
    }
    else if (op == "<")
    {
        cout << "olt ";
    }
    else if (op == "<=")
    {
        cout << "ole ";
    }
    else if (op == "==")
    {
        cout << "oeq ";
    }
    cout << a->getTypeStr() << ", " << b->getStr() << endl;
}

void BrInstruction::print()
{
    if (exp)
        cout << "  br " << exp->getTypeStr() << ", " << label1->getStr() << ", " << label2->getStr() << endl;
    else
        cout << "  br " << label1->getStr() << endl;
}

bool ReturnInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if (retValue == target)
    {
        retValue = newValue;
        return true;
    }
    return false;
}

bool AllocaInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    return false;
}

bool StoreInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if(gep) gep->replaceValue(target, newValue);
    if (value == target)
    {
        value = newValue;
        return true;
    }
    else if (des == target)
    {
        des = newValue;
        return true;
    }
    return false;
}

bool LoadInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if(gep) gep->replaceValue(target, newValue);
    if (from == target)
    {
        from = newValue;
        return true;
    }
    return false;
}

bool BitCastInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if (from == target)
    {
        from = newValue;
        return true;
    }
    return false;
}

bool CallInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    bool flag = false;
    for (int i = 0; i < argv.size(); i++)
    {
        if (argv[i] == target)
        {
            argv[i] = newValue;
            flag = true;
        }
    }
    return flag;
}

bool ExtInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if (from == target)
    {
        from = newValue;
        return true;
    }
    return false;
}

bool SitofpInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if (from == target)
    {
        from = newValue;
        return true;
    }
    return false;
}

bool FptosiInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if (from == target)
    {
        from = newValue;
        return true;
    }
    return false;
}

bool PhiInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    bool flag = false;
    for(int i=0;i<from.size();i++){
        if(from[i].first == target){
            from[i].first = newValue;
            flag = true;
        }
    }
    return flag;
}

bool GetElementPtrInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    bool flag = false;
    if (target == from)
    {
        from = newValue;
        return true;
    }
    for (int i = 0; i < index.size(); i++)
    {
        if (index[i] == target)
        {
            index[i] = newValue;
            flag = true;
        }
    }
    return flag;
}

bool BinaryInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if (a == target)
    {
        a = newValue;
        return true;
    }
    else if (b == target)
    {
        b = newValue;
        return true;
    }
    return false;
}

bool FnegInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if (a == target)
    {
        a = newValue;
        return true;
    }

    return false;
}

bool IcmpInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if (a == target)
    {
        a = newValue;
        return true;
    }
    else if (b == target)
    {
        b = newValue;
        return true;
    }
    return false;
}

bool FcmpInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if (a == target)
    {
        a = newValue;
        return true;
    }
    else if (b == target)
    {
        b = newValue;
        return true;
    }
    return false;
}

bool BrInstruction::replaceValue(ValuePtr target, ValuePtr newValue)
{
    if (exp == target)
    {
        exp = newValue;
        return true;
    }

    return false;
}

void replaceVarByVar(ValuePtr from, ValuePtr to)
{
    // from已经不会再使用，所以没必要维护from的Use
    Use *p = from->useHead;
    while (p != nullptr)
    {
        auto user = p->user;
        if (!user->replaceValue(from, to))
        {
            cerr << "error" << user->type << "\n";
        }
        newUse(to.get(), user);
        p = p->next;
    }
}