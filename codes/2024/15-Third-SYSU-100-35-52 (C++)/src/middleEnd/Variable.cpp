#include "Label.h"
#include "Type.h"
#include "Value.h"
#include <Variable.h>
#include <iomanip>

string Variable::getStr() {
    return (isGlobal ? "@" : "%") + label.name();
}
// NOLINTBEGIN

bool Int::zero() {
    return !intVal;
}

bool Float::zero() {
    return std::abs(floatVal) < 1e-6;
}

bool Ptr::zero() {
    return false;
}

bool Arr::zero() {
    if(inner.empty())
        return true;
    for(auto i : inner)
        if(!(i->zero()))
            return false;
    return true;
}

// FIXME: 这里 copy 获取name  会重命名？？
Variable* Variable::copy(Variable* var) {
    if(var->type->isInt())
        return VariablePtr(new Int(var->getName(), var->isGlobal, var->isConst));
    if(var->type->isFloat())
        return VariablePtr(new Float(var->getName(), var->isGlobal, var->isConst));
    if(var->type->isArr())
        return VariablePtr(new Arr(var->getName(), var->isGlobal, var->isConst, var->type));
    return VariablePtr(new Ptr(var->getName(), var->isGlobal, var->isConst, var->type));
}

Int::Int(string name, bool isGlobal, bool isConst, ValuePtr value) : Variable{ Type::getInt(), name, isGlobal, isConst } {
    assert(value->type->isInt() || value->type->isFloat());
    if(value->type->isInt())
        intVal = dynamic_cast<Const*>(value)->intVal;
    else
        intVal = dynamic_cast<Const*>(value)->floatVal;
}

void Int::dump(std::ostream& out) {
    assert(isGlobal);
    out << "@" << label.name() << " = " << (isConst ? "constant" : "global") << " " << type->getStr() << " " << to_string(intVal)
        << endl;
}

//
void Int::printHelper(std::ostream& out) {
    out << type->getStr() << " " << to_string(intVal);
}

Float::Float(string name, bool isGlobal, bool isConst, ValuePtr value) : Variable{ Type::getFloat(), name, isGlobal, isConst } {
    assert(value->type->isInt() || value->type->isFloat());
    if(value->type->isInt())
        floatVal = dynamic_cast<Const*>(value)->intVal;
    else
        floatVal = dynamic_cast<Const*>(value)->floatVal;
}

void Float::dump(std::ostream& out) {
    assert(isGlobal);
    out << "@" << label.name() << " = " << (isConst ? "constant" : "global") << " " << type->getStr() << " ";
    union {
        double f;
        uint64_t u;
    } f2u;
    f2u.f = floatVal;
    // printf("0x%" PRIx64 "\n", f2u.u);
    out << "0x" << std::hex << std::setw(16) << std::setfill('0') << f2u.u << std::endl;
}
//
void Float::printHelper(std::ostream& out) {
    out << type->getStr() << " ";
    union {
        double f;
        uint64_t u;
    } f2u;
    f2u.f = floatVal;
    // printf("0x%" PRIx64, f2u.u);
    out << "0x" << std::hex << std::setw(16) << std::setfill('0') << f2u.u << std::endl;
}

void Ptr::dump(std::ostream& out) {
    assert(0);
}
//
void Ptr::printHelper(std::ostream& out) {}

// FIXME: 这里 copy 获取name  会重命名？？
bool Arr::push(VariablePtr variable) {
    if(getElementType()->operator==(variable->type)) {
        if(inner.size() < getElementLength()) {
            inner.emplace_back(variable);
            return true;
        }
        return false;
    }
    if(inner.size() <= getElementLength()) {
        if(inner.size() && dynamic_cast<Arr*>(inner.back())->push(variable)) {
            return true;
        }
        if(inner.size() < getElementLength()) {
            inner.emplace_back(VariablePtr(new Arr(getName(), isGlobal, isConst, getElementType())));
            return dynamic_cast<Arr*>(inner.back())->push(variable);
        }
        return false;

        return false;
    }
    return false;
}

// FIXME: 这里 copy 获取name  会重命名？？
void Arr::fill() {
    auto sonType = getElementType();
    int i = 0;
    for(; i < inner.size(); i++) {
        if(sonType->isArr())
            dynamic_cast<Arr*>(inner[i])->fill();
    }
    for(; i < getElementLength(); i++) {
        if(sonType->isArr()) {
            auto tmp = new Arr(getName(), isGlobal, true, sonType);
            tmp->fill();
            inner.emplace_back(tmp);
        } else {
            if(sonType->isInt())
                inner.emplace_back(VariablePtr(new Int(getName(), isGlobal, true, (int)0)));
            else if(sonType->isFloat())
                inner.emplace_back(VariablePtr(new Float(getName(), isGlobal, true, (float)0)));
        }
    }
}

void Arr::dump(std::ostream& out) {
    out << "@" << getName() << " = " << (isConst ? "constant" : "global") << " ";
    printHelper(out);
    out << endl;
}
//
void Arr::printHelper(std::ostream& out) {
    out << type->getStr();
    if(zero())
        out << " zeroinitializer";
    else {
        out << " [";
        int i = 0;
        for(; i < inner.size(); i++) {
            if(i)
                out << ", ";
            inner[i]->printHelper(out);
        }
        for(; i < getElementLength(); i++) {
            out << ", ";
            if(getElementType()->isArr()) {
                auto tmp = new Arr(getName(), isGlobal, true, getElementType());
                tmp->printHelper(out);
                delete tmp;
            } else if(getElementType()->isInt()) {
                auto tmp = new Int(getName(), isGlobal, false, (int)0);
                tmp->printHelper(out);
                delete tmp;
            } else if(getElementType()->isFloat()) {
                auto tmp = new Float(getName(), isGlobal, false, (float)0);
                tmp->printHelper(out);
                delete tmp;
            }
        }
        out << "]";
    }
}

// NOLINTEND
