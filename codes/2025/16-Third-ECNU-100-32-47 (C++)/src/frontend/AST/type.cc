#include "AST/type.h"
#include "common.h"
#include <cstdint>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>

namespace aaac {
namespace frontend {

void Type::PrintTo(std::ostream &os) const {
    os << getTypePrefix() << getTypePostfix();
}

std::string Type::getAsString() const {
    std::ostringstream os;
    PrintTo(os);
    return os.str();
}

std::ostream &Type::dump(std::ostream &os) const {
    PrintTo(os);
    return os;
}

std::string BuiltInType::getTypePrefix() const {
    return btype;
}

std::string BuiltInType::getTypePostfix() const {
    return "";
}

std::string ArrayType::getTypePrefix() const {
    return base->getTypePrefix();
}

std::string ArrayType::getTypePostfix() const {
    return "[" + std::to_string(length) + "]" + base->getTypePostfix();
}

std::string FunctionType::getTypePrefix() const {
    return ret->getTypePrefix();
}

std::string FunctionType::getTypePostfix() const {
    std::ostringstream os;
    os << "(";
    for(auto formal : this->formals) {
        if(formal != this->formals.front()) {
            os << ",";
        }
        os << formal->getAsString();
    }
    os << ")";
    return os.str();
}

std::shared_ptr<BuiltInType>
TypeFactory::getBuiltinType(std::string type) {
    if(type == "void") return VoidTy;
    if(type == "int") return IntTy;
    if(type == "float") return FloatTy;
    Assert(0, "%s is not BuiltinType", type.c_str());
}

std::shared_ptr<TypeFactory> TypeFactory::s_TypeFactory = nullptr;
std::shared_ptr<TypeFactory> TypeFactory::getTypeFactory() {
    if(s_TypeFactory == nullptr) {
        s_TypeFactory = std::shared_ptr<TypeFactory>(new TypeFactory());
    }
    return s_TypeFactory;
}

/*  singleton ArrayType, so we can compare two arrayType thru their address */
std::shared_ptr<ArrayType> 
TypeFactory::getArrayType(std::shared_ptr<Type> base, uint64_t subscript) {
    std::ostringstream os;
    os << base->getTypePrefix();
    os << "[" + std::to_string(subscript) + "]";
    os << base->getTypePostfix();
    auto it = Bases.find(os.str());
    if(it != Bases.end()) {
        return std::static_pointer_cast<ArrayType>(it->second);
    }
    std::shared_ptr<ArrayType> arrType = std::make_shared<ArrayType>(base,subscript);
    Bases[os.str()] = arrType;
    return arrType;
}

std::shared_ptr<FunctionType> 
TypeFactory::getFuncType(std::vector<std::shared_ptr<Type>> formals, std::shared_ptr<Type> ret) {
    return std::make_shared<FunctionType>(formals,ret);
}

} // namespace frontend
} // namespace aaac