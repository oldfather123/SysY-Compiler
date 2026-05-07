#include "Values.h"
#include "Scopes.h"
#include "Instruction.h"
#include <iostream>
#include <iomanip>
using namespace ir;

RegPtr Register::create(const FuncPtr &func, const DataType &dataType) {
    auto ret = makePtr<Register>(dataType, func->getUniqueRegName());
    ret->_init(ret, func);
    ret->_isAnonymous = true;
    return ret;
}
RegPtr Register::create(const FuncPtr &func, const DataType &dataType, const String &name) {
    auto ret = makePtr<Register>(dataType, func->getUniqueRegName(name));
    ret->_init(ret, func);
    return ret;
}

void Register::rename(const String &name, bool unique) { _name = unique ? _func->getUniqueRegName(name) : name; }

void Register::joinAliasGroup(RegPtr dest) {
    dbgassert(_dataType.isPointer(), "Register is not a pointer");
    dbgassert(_aliasUnionFindParent == _self || findAliasGroup() == dest->findAliasGroup(), "Register is already in another alias group");
    _aliasUnionFindParent = dest->findAliasGroup();
}

RegPtr Register::findAliasGroup() {
    dbgassert(_dataType.isPointer(), "Register is not a pointer");
    return _aliasUnionFindParent == _self ? _self : _aliasUnionFindParent = _aliasUnionFindParent->findAliasGroup();
}

Literal::Literal(const float &floatValue) : Literal(PrimaryDataType::Float) {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<float>::max_digits10) << floatValue;
    this->_floatStr = out.str();
}

DataType Value::dataType() const {
    switch (this->_valueType) {
        case Type::Literal: {
            return this->getLiteral().dataType();
        }
        case Type::Register: {
            return this->getRegister()->dataType();
        }
        case Type::ArrayInitializer: {
            return this->_arrayDataType;
        }
        default: {
            dbgassert(false, "Unspecified value type");
        }
    }
    return PrimaryDataType::Unknown;
}

String Variable::toString() const {
    if (this->isReg()) {
        return _reg->toString();
    } else {
        return _memPtr->toString();
    }
}

String Definition::toString() const {
    return inst->toString();
}
