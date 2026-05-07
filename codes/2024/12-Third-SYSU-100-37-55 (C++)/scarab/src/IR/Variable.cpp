#include "Variable.h"

ValuePtr Void::value = ValuePtr(new Void());

unordered_map<int8_t, ValuePtr> Const::Int8Map;
unordered_map<int,ValuePtr> Const::IntMap;
unordered_map<long long, ValuePtr> Const::longlongMap;
unordered_map<float, ValuePtr> Const::FloatMap;
unordered_map<bool, ValuePtr> Const::BoolMap;


ValuePtr Const::createConstant(TypePtr type,string val,string name){
    float floatVal = 0.0;
    long long intVal = 0;
    try
    {
        if (type->isFloat())
        {
            floatVal = std::stod(val);
            return createConstant(type,floatVal,name);
        }
        else
        {
            int scale = 10;
            if (val.size() > 2 && val.substr(0, 2) == "0x")
                scale = 16;
            else if (val.size() > 1 && val[0] == '0')
                scale = 8;
            intVal = stoll(val, 0, scale);
            return createConstant(type, intVal, name);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "const init error(" << intVal << ", " << floatVal << ")" << '\n';
        return nullptr;
    }
}
ValuePtr Const::createZero(TypePtr type){
    if (type->isInt()) {
        return Const::createConstant(Type::getInt(), int(0));
    } else if (type->isFloat()) {
        return Const::createConstant(Type::getFloat(), 0.0f);
    }else if (type->isBool()){
        return Const::createConstant(Type::getBool(), false);
    }
    return nullptr;
}

ValuePtr Const::createOne(TypePtr type){
    if (type->isInt()) {
        return Const::createConstant(Type::getInt(), int(1));
    } else if (type->isFloat()) {
        return Const::createConstant(Type::getFloat(), 1.0f);
    } else if (type->isBool()) {
        return Const::createConstant(Type::getBool(), true);
    }
    return nullptr;
}



Use *newUse(Value *value, Instruction *user) {
    auto use = new Use(value, user);
    value->addUseNode(use);
    return use;
}

Use *newUse(Value *value, Instruction *user, Value *userValue) {
    auto use = new Use(value, user, userValue);
    value->addUseNode(new Use(value, user, userValue));
    return use;
}

Use *findUse(Value *value, Value *userValue) {
    Use *use = value->useHead, *next = nullptr;
    while(use)
    {
        next = use->next;
        if (use->userValue == userValue) return use;
        use = next;
    }
    return nullptr;
}

Use *findUse(Value *value, Instruction *user) {
    Use *use = value->useHead, *next = nullptr;
    while(use)
    {
        next = use->next;
        if (use->user == user) return use;
        use = next;
    }
    return nullptr;
}

void Use::rmUse() {
    if (pred == nullptr && next == nullptr) {
        val->useHead = nullptr;
    } 
    else if (pred == nullptr && next != nullptr) {
        val->useHead = next;
        next->pred = nullptr;
    } 
    else if (pred != nullptr && next == nullptr) {
        pred->next = nullptr;
    } 
    else if (pred != nullptr && next != nullptr) {
        pred->next = next;
        next->pred = pred;
    }
    else{

    }
    this->val->useCount --;
}



bool rmInsUse(Instruction* I,ValuePtr v){
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



string Const::toString()
{
    switch(type->ID){
        case Int8ID:
        case IntID:
        case Int64ID:{
            return to_string(intVal);
        }
        case FLoatID:{
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
        case BoolID:{
            return boolVal ? "true" : "false";
        }
        default:
            return "wrong";
    }
}

string Variable::toString()
{
    return (isGlobal ? "@" : "%") + name;
}

shared_ptr<Variable> Variable::copy(shared_ptr<Variable> var){
    switch(var->type->ID){
        case IntID:{
            return VariablePtr(new Int(var->name, var->isGlobal, var->isConst));
        }
        case FLoatID:{
            return VariablePtr(new Float(var->name, var->isGlobal, var->isConst));
        }
        case ArrID:{
            return VariablePtr(new Arr(var->name, var->isGlobal, var->isConst, var->type));
        }
        default:{
            return VariablePtr(new Ptr(var->name, var->isGlobal, var->isConst, var->type));
        }
    }
}

Int::Int(string name, bool isGlobal, bool isConst, ValuePtr value) : Variable{Type::getInt(), name, isGlobal, isConst}
{
    if (value->type->isInt())
        intVal = dynamic_cast<Const *>(value.get())->intVal;
    else
        intVal = dynamic_cast<Const *>(value.get())->floatVal;
}

void Int::print(){
    cout << "@" << name << " = " << (isConst ? "constant" : "global") << " " << type->toString() << " " << to_string(intVal) << endl;
}

void Int::printHelper()
{
    cout << type->toString() << " " << to_string(intVal);
}

// 判断是不是为0
bool Int::zero()
{
    return intVal == 0;
}

Float::Float(string name, bool isGlobal, bool isConst, ValuePtr value) : Variable{Type::getFloat(), name, isGlobal, isConst}
{
    if (value->type->isInt())
        floatVal = dynamic_cast<Const *>(value.get())->intVal;
    else
        floatVal = dynamic_cast<Const *>(value.get())->floatVal;
}

void Float::print(){
    cout << "@" << name << " = " << (isConst ? "constant" : "global") << " " << type->toString() << " ";
    union{
        uint64_t u;
        double f;
    } f2u;
    f2u.f = floatVal;
    printf("0x%" PRIx64 "\n", f2u.u);
}

void Float::printHelper()
{
    cout << type->toString() << " ";
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
    return abs(floatVal) < 1e-6;
}

void Ptr::print(){
    cout << "@" << name << " = " << "global" << " ";
    cout << type->toString();
    cout << " null";
    cout << endl;
}

void Ptr::printHelper()
{

}


bool Ptr::zero()
{
    return false;
}

bool Arr::push(VariablePtr variable){
    if (getElementType()->operator==(variable->type)){
        if (inner.size() < getElementLength()){
            inner.emplace_back(variable);
            return true;
        }
        else
            return false;
    }
    else{
        if (inner.size() <= getElementLength())
        {
            if (inner.size() && dynamic_cast<Arr *>(inner.back().get())->push(variable)){
                return true;
            }
            else{
                if (inner.size() < getElementLength()){
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

void Arr::fill(){
    auto sonType = getElementType();
    int i;
    for (i = 0; i < inner.size(); i++){
        if (sonType->isArr())
            dynamic_cast<Arr *>(inner[i].get())->fill();
    }
    for (; i < getElementLength(); i++){
        if (sonType->isArr()){
            auto tmp = shared_ptr<Arr>(new Arr(name, isGlobal, true, sonType));
            tmp->fill();
            inner.emplace_back(tmp);
        }
        else{
            if(sonType->isInt()) inner.emplace_back(VariablePtr(new Int(name, isGlobal, true, 0)));
            else if(sonType->isFloat()) inner.emplace_back(VariablePtr(new Float(name, isGlobal, true, 0)));
        }
    }
}

void Arr::print(){
    cout << "@" << name << " = " << (isConst ? "constant" : "global") << " ";
    printHelper();
    cout << endl;
}

void Arr::printHelper()
{
    cout << type->toString();
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

bool Arr::zero(){
    if(inner.empty())
        return true;
    for (auto i : inner)
        if (!(i->zero()))
            return false;
    return true;
}

ValuePtr createNewConst(ValuePtr val1,ValuePtr val2, unsigned opCode){
    Const * const1 = dynamic_cast<Const*>(val1.get());
    Const * const2 = dynamic_cast<Const*>(val2.get());
    // 不处理bool
    if(!const1 || !const2 || const1->type->isBool()){
        return nullptr;
    }

    switch (opCode){
    case Add:{
        if(const1->type->isInt()){
            return Const::createConstant(Type::getInt(), const1->intVal+const2->intVal);
        }
        else if(const1->type->isFloat()){
            return Const::createConstant(Type::getInt(), int(const1->floatVal) + int(const2->floatVal));
        }
        return nullptr;
    }
    case FAdd:{
        if(const1->type->isInt()){
            return Const::createConstant(Type::getFloat(), const1->intVal + const2->intVal);
        }
        else if(const1->type->isFloat()){
            return Const::createConstant(Type::getFloat(), const1->floatVal + const2->floatVal);
        }
        return nullptr;
    }
    case Mul:{
        if(const1->type->isInt()){
            return Const::createConstant(Type::getInt(), const1->intVal * const2->intVal);
        }
        else if(const1->type->isFloat()){
            return Const::createConstant(Type::getInt(), int(const1->floatVal) * int(const2->floatVal));
        }
        return nullptr;
    }
    case FMul:{
        if(const1->type->isInt()){
            return Const::createConstant(Type::getFloat(), const1->intVal*const2->intVal);
        }
        else if(const1->type->isFloat()){
            return Const::createConstant(Type::getFloat(), const1->floatVal*const2->floatVal);
        }
        return nullptr;
    }
    
    default:
        return nullptr;
    }
}