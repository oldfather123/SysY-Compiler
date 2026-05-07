#include "Variable.h"

ValuePtr Void::value = ValuePtr(new Void());

Use *newUse(Value *value, Instruction *user) {
    auto use = new Use;
    use->val = value;
    use->user = user;
    value->addUse(use);
    return use;
}

void Use::rmUse() {
    if (prev == nullptr && next == nullptr) {
        val->useHead = nullptr;
    } 
    else if (prev == nullptr && next != nullptr) {
        val->useHead = next;
        next->prev = nullptr;
    } 
    else if (prev != nullptr && next == nullptr) {
        prev->next = nullptr;
    } 
    else if (prev != nullptr && next != nullptr) {
        prev->next = next;
        next->prev = prev;
    }
}
bool rmInstructionUse(shared_ptr<Instruction> I,ValuePtr v){
    auto useH = v->useHead;
    while(useH!=nullptr){
        if(useH->user == I.get()){
            useH->rmUse();
            return true;
        }
        useH=useH->next;
    }
    return false;
}

//非智能指针版本
bool rmInstructionUse(Instruction* I,ValuePtr v){
    auto useH = v->useHead;
    while(useH!=nullptr){
        if(useH->user == I){
            useH->rmUse();
            return true;
        }
        useH=useH->next;
    }
    return false;
}



string Const::getStr()
{
    if (type->isInt())
        return to_string(intVal);
    else if (type->isBool())
        return boolVal ? "true" : "false";
    else if (type->isFloat())
    {
        union
        {
            double f;
            uint64_t u;
        } f2u;
        f2u.f = floatVal;
        char buff[20] = {};
        sprintf(buff, "0x%" PRIx64, f2u.u);

        return string(buff);
    }
    else
        return "wrong const type!";
}

string Variable::getStr()
{
    return (isGlobal ? "@" : "%") + name;
}

shared_ptr<Variable> Variable::copy(shared_ptr<Variable> var)
{
    if (var->type->isInt())
        return VariablePtr(new Int(var->name, var->isGlobal, var->isConst));
    else if (var->type->isFloat())
        return VariablePtr(new Float(var->name, var->isGlobal, var->isConst));
    else if (var->type->isArr())
        return VariablePtr(new Arr(var->name, var->isGlobal, var->isConst, var->type));
    else
        return VariablePtr(new Ptr(var->name, var->isGlobal, var->isConst, var->type));
}

Int::Int(string name, bool isGlobal, bool isConst, ValuePtr value) : Variable{Type::getInt(), name, isGlobal, isConst}
{
    assert(value->type->isInt() || value->type->isFloat());
    if (value->type->isInt())
        intVal = dynamic_cast<Const *>(value.get())->intVal;
    else
        intVal = dynamic_cast<Const *>(value.get())->floatVal;
}

void Int::print()
{
    assert(isGlobal);
    cout << "@" << name << " = " << (isConst ? "constant" : "global") << " " << type->getStr() << " " << to_string(intVal) << endl;
}

void Int::printHelper()
{
    cout << type->getStr() << " " << to_string(intVal);
}

bool Int::zero()
{
    return !intVal;
}

Float::Float(string name, bool isGlobal, bool isConst, ValuePtr value) : Variable{Type::getFloat(), name, isGlobal, isConst}
{
    assert(value->type->isInt() || value->type->isFloat());
    if (value->type->isInt())
        floatVal = dynamic_cast<Const *>(value.get())->intVal;
    else
        floatVal = dynamic_cast<Const *>(value.get())->floatVal;
}

void Float::print()
{
    assert(isGlobal);
    cout << "@" << name << " = " << (isConst ? "constant" : "global") << " " << type->getStr() << " ";
    union
    {
        double f;
        uint64_t u;
    } f2u;
    f2u.f = floatVal;
    printf("0x%" PRIx64 "\n", f2u.u);
}

void Float::printHelper()
{
    cout << type->getStr() << " ";
    union
    {
        double f;
        uint64_t u;
    } f2u;
    f2u.f = floatVal;
    printf("0x%" PRIx64, f2u.u);
}

bool Float::zero()
{
    return std::abs(floatVal) < 1e-6;
}

void Ptr::print()
{
    assert(0);
}

void Ptr::printHelper()
{
}

bool Ptr::zero()
{
    return false;
}

bool Arr::push(VariablePtr variable)
{
    if (getElementType()->operator==(variable->type))
    {
        if (inner.size() < getElementLength())
        {
            inner.emplace_back(variable);
            return true;
        }
        else
            return false;
    }
    else
    {
        if (inner.size() <= getElementLength())
        {
            if (inner.size() && dynamic_cast<Arr *>(inner.back().get())->push(variable))
            {
                return true;
            }
            else
            {
                if (inner.size() < getElementLength())
                {
                    inner.emplace_back(VariablePtr(new Arr(name, isGlobal, isConst, getElementType())));
                    return dynamic_cast<Arr *>(inner.back().get())->push(variable);
                }
                else
                    return false;
            }
            return false;
        }
        else
            return false;
    }
}

void Arr::fill()
{
    auto sonType = getElementType();
    int i = 0;
    for (; i < inner.size(); i++)
    {
        if (sonType->isArr())
            dynamic_cast<Arr *>(inner[i].get())->fill();
    }
    for (; i < getElementLength(); i++)
    {
        if (sonType->isArr())
        {
            auto tmp = shared_ptr<Arr>(new Arr(name, isGlobal, true, sonType));
            tmp->fill();
            inner.emplace_back(tmp);
        }
        else
        {
            if(sonType->isInt()) inner.emplace_back(VariablePtr(new Int(name, isGlobal, true, 0)));
            else if(sonType->isFloat()) inner.emplace_back(VariablePtr(new Float(name, isGlobal, true, 0)));
        }
    }
}

void Arr::print()
{
    cout << "@" << name << " = " << (isConst ? "constant" : "global") << " ";
    printHelper();
    cout << endl;
}

void Arr::printHelper()
{
    cout << type->getStr();
    if (zero())
        cout << " zeroinitializer";
    else
    {
        cout << " [";
        int i = 0;
        for (; i < inner.size(); i++)
        {
            if (i)
                cout << ", ";
            inner[i]->printHelper();
        }
        for (; i < getElementLength(); i++)
        {
            cout << ", ";
            if (getElementType()->isArr())
            {
                VariablePtr(new Arr(name, isGlobal, true, getElementType()))->printHelper();
            }
            else if(getElementType()->isInt())
            {
                VariablePtr(new Int(name, isGlobal, false, 0))->printHelper();
            }else if(getElementType()->isFloat()){
                VariablePtr(new Float(name, isGlobal, false, 0))->printHelper();
            }
        }
        cout << "]";
    }
}

bool Arr::zero()
{
    if (inner.empty())
        return true;
    for (auto i : inner)
        if (!(i->zero()))
            return false;
    return true;
}
