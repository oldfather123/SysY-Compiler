#include "Constant.hpp"
#include "Module.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>

struct pair_hash {
    template <typename T>
    std::size_t operator()(const std::pair<T, Module *> val) const {
        auto lhs = std::hash<T>()(val.first);
        auto rhs =
            std::hash<uintptr_t>()(reinterpret_cast<uintptr_t>(val.second));
        return lhs ^ rhs;
    }
};

static std::unordered_map<std::pair<int, Module *>,
                          std::unique_ptr<ConstantInt>, pair_hash>
    cached_int;
static std::unordered_map<std::pair<bool, Module *>,
                          std::unique_ptr<ConstantInt>, pair_hash>
    cached_bool;
static std::unordered_map<std::pair<float, Module *>,
                          std::unique_ptr<ConstantFP>, pair_hash>
    cached_float;
static std::unordered_map<std::pair<long long, Module *>,
                          std::unique_ptr<ConstantInt>, pair_hash>
    cached_long_long;
static std::unordered_map<Type *, std::unique_ptr<ConstantZero>> cached_zero;

ConstantInt *ConstantInt::get(int val, Module *m) {
    if (cached_int.find(std::make_pair(val, m)) != cached_int.end())
        return cached_int[std::make_pair(val, m)].get();
    return (cached_int[std::make_pair(val, m)] = std::unique_ptr<ConstantInt>(
                new ConstantInt(m->get_int32_type(), val)))
        .get();
}
ConstantInt *ConstantInt::get(bool val, Module *m) {
    if (cached_bool.find(std::make_pair(val, m)) != cached_bool.end())
        return cached_bool[std::make_pair(val, m)].get();
    return (cached_bool[std::make_pair(val, m)] = std::unique_ptr<ConstantInt>(
                new ConstantInt(m->get_int1_type(), val ? 1 : 0)))
        .get();
}
ConstantInt *ConstantInt::get(long long val, Module *m) {
    if (cached_long_long.find(std::make_pair(val, m)) != cached_long_long.end())
        return cached_long_long[std::make_pair(val, m)].get();
    return (cached_long_long[std::make_pair(val, m)] =
                std::unique_ptr<ConstantInt>(new ConstantInt(m->get_int64_type(), val)))
        .get();
}

ConstantArray::ConstantArray(ArrayType *ty, const std::vector<Constant *> &val)
    : Constant(ty, "") {
    // for (unsigned i = 0; i < val.size(); i++)
    //     set_operand(i, val[i]);
    this->const_array.assign(val.begin(), val.end());
}

Constant *ConstantArray::get_element_value(int index) {
    return this->const_array[index];
}

ConstantArray *ConstantArray::get(ArrayType *ty,
                                  const std::vector<Constant *> &val) {
    return new ConstantArray(ty, val);
}

ConstantFP *ConstantFP::get(float val, Module *m) {
    if (cached_float.find(std::make_pair(val, m)) != cached_float.end())
        return cached_float[std::make_pair(val, m)].get();
    return (cached_float[std::make_pair(val, m)] = std::unique_ptr<ConstantFP>(
                new ConstantFP(m->get_float_type(), val)))
        .get();
}

ConstantZero *ConstantZero::get(Type *ty, Module *m) {
    if (not cached_zero[ty])
        cached_zero[ty] = std::unique_ptr<ConstantZero>(new ConstantZero(ty));
    return cached_zero[ty].get();
}

