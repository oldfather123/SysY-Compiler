#include "Instruction.h"
#include "Type.h"
#include "Value.h"
#include <cassert>
#include <functional>  // For std::hash
#include <iomanip>
#include <memory>

Value* Void::value = new Void();

Value* Void::get() {
    return value;
}

// NOLINTBEGIN

unordered_map<int, unique_ptr<Value>> Const::constIntMap;
unordered_map<float, unique_ptr<Value>> Const::constFloatMap;
unordered_map<bool, unique_ptr<Value>> Const::constBoolMap;
unordered_map<int8_t, unique_ptr<Value>> Const::constInt8Map;
unordered_map<long long, unique_ptr<Value>> Const::constInt64Map;

bool Const::isStrongEqual(Const* rhs) {
    if(id != rhs->id) {
        return false;
    }
    if(type->isInt())
        return intVal == rhs->intVal;
    if(type->isBool())
        return boolVal == rhs->boolVal;
    if(type->isFloat())
        return floatVal == rhs->floatVal;
    return false;
}

bool Const::isWeakEqual(Const* rhs) {
    if(type->isInt())
        return intVal == rhs->intVal;
    if(type->isBool())
        return boolVal == rhs->boolVal;
    if(type->isFloat())
        // FIXME: 暂时没有使用
        return std::abs(floatVal - rhs->floatVal) < 1e-6;
    return false;
}

template <typename T>
ValuePtr Const::getConst(TypePtr type, T val) {
    if(Type::get<T>()->id != type->id) {
        if(type->id == TypeID::Float) {
            return getConst(type, float(val));
        }
        if(type->id == TypeID::Bool) {
            return getConst(type, bool(val));
        }
        if(type->id == TypeID::Int8) {
            return getConst(type, int8_t(val));
        }
        if(type->id == TypeID::Int64) {
            return getConst(type, (long long)(val));
        }
        if(type->id == TypeID::Int) {
            return getConst(type, int(val));
        }
    }
    if(type->id == TypeID::Int) {
        if(Const::constIntMap.find(val) != Const::constIntMap.end()) {
            return Const::constIntMap[val].get();
        }
        Const::constIntMap[val] = std::make_unique<Const>(type, val);
        return Const::constIntMap[val].get();
    }
    if(type->isFloat()) {
        if(Const::constFloatMap.find(val) != Const::constFloatMap.end()) {
            return Const::constFloatMap[val].get();
        }
        Const::constFloatMap[val] = std::make_unique<Const>(type, val);
        return Const::constFloatMap[val].get();
    }
    if(type->isBool()) {
        if(Const::constBoolMap.find(val) != Const::constBoolMap.end()) {
            return Const::constBoolMap[val].get();
        }
        Const::constBoolMap[val] = std::make_unique<Const>(type, val);
        return Const::constBoolMap[val].get();
    }
    if(type->id == TypeID::Int8) {
        if(Const::constInt8Map.find(val) != Const::constInt8Map.end()) {
            return Const::constInt8Map[val].get();
        }
        Const::constInt8Map[val] = std::make_unique<Const>(type, val);
        return Const::constInt8Map[val].get();
    }
    if(type->id == TypeID::Int64) {
        if(Const::constInt64Map.find(val) != Const::constInt64Map.end()) {
            return Const::constInt64Map[val].get();
        }
        Const::constInt64Map[val] = std::make_unique<Const>(type, val);
        return Const::constInt64Map[val].get();
    }
    assert(false);
    //// TODO:: 对于 float的 cache 暂时存疑，可以直接find吗？？
    // if(Const::constMap.find(val) != Const::constMap.end()) {
    //     return Const::constMap[val];
    // }
    // auto ret = ValuePtr(new Const(type, val));
    // return Const::constMap[val] = ret;
}

ValuePtr Const::getConst(TypePtr type, string val, string name) {
    float floatVal;
    long long intVal;
    try {
        if(type->isFloat()) {
            floatVal = std::stod(val);  // NOLINT
            return getConst(type, floatVal);
        }
        int scale = 10;
        if(val.size() > 2 && val.substr(0, 2) == "0x")
            scale = 16;
        else if(val.size() > 1 && val[0] == '0')
            scale = 8;
        intVal = stoll(val, 0, scale);  // NOLINT
        return getConst(type, intVal);

    } catch(const std::exception& e) {
        std::cerr << "const init error(" << intVal << ", " << floatVal << ")" << '\n';
        return nullptr;
    }
}

string Const::getStr() {
    if(type->isInt())
        return to_string(intVal);
    if(type->isBool())
        return boolVal ? "true" : "false";
    if(type->isFloat()) {
        union {
            double f;
            uint64_t u;
        } f2u;
        f2u.f = floatVal;
        char buff[20] = {};
        sprintf(buff, "0x%" PRIx64, f2u.u);

        return string(buff);
    }
    return "wrong const type!";
}

// NOLINTEND
