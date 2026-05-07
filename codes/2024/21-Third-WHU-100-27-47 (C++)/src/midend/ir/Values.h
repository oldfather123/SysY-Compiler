#pragma once

#include "Common.h"
#include "Types.h"
#include <functional>

namespace ir_builder {
    class Builder;
}

namespace ir {
    class BasicBlock;
    struct Literal;
    class Register;
    class Address;
    struct Value;
    class DefInst;

    /// @brief Literal value, can be int/float/string/void, readonly & copyable
    struct Literal {
        friend struct std::hash<Literal>;

    private:
        DataType _dataType;
        int _intValue = 0;
        String _floatStr = "0.";
        bool _boolValue = false;
        String _stringValue = "";

    public: /* constructors */
        Literal(DataType dataType = PrimaryDataType::Unknown) {
            _dataType = dataType.constType();
            if (dataType == PrimaryDataType::Unknown) {
                return;
            }
            dbgassert(!dataType.isPointer(), "Only non-pointer data type can be used for Literal");
            dbgassert(dataType.isPrimary(), "Only primary data type can be used for Literal");
            auto primaryDataType = dataType.baseDataType();
        }
        /// @brief Constructor for void literal value
        static Literal ofVoid() { return Literal{PrimaryDataType::Void}; }
        /// @brief Constructor for int literal value
        Literal(const int &intValue) : Literal(PrimaryDataType::Int) { _intValue = intValue; }
        /// @brief Constructor for float literal value
        Literal(const float &floatValue);
        static Literal ofFloat(const String &floatStr) {
            auto ret = Literal{PrimaryDataType::Float};
            ret._floatStr = floatStr;
            return ret;
        }
        /// @brief Constructor for bool literal value
        Literal(const bool &boolValue) : Literal(PrimaryDataType::Bool) { _boolValue = boolValue; }
        /// @brief Constructor for string literal value
        Literal(const String &stringValue) : Literal(PrimaryDataType::String) { _stringValue = stringValue; }
        /// @brief Copy constructor
        /// @param other
        Literal(const Literal &other) = default;

    public: /* properties */
        DataType &dataType() { return _dataType; }
        const DataType &dataType() const { return _dataType; }
        bool isUnspecified() const { return _dataType == PrimaryDataType::Unknown; }
        bool hasValue() const { return !isUnspecified(); }
        bool isVoid() const { return _dataType == PrimaryDataType::Void; }
        bool isInt() const { return _dataType == PrimaryDataType::Int; }
        bool isFloat() const { return _dataType == PrimaryDataType::Float; }
        bool isBool() const { return _dataType == PrimaryDataType::Bool; }
        bool isString() const { return _dataType == PrimaryDataType::String; }

    public: /* methods */
        const int getInt() const {
            dbgassert(isInt(), "Cannot get int value when literal value is not Int");
            return _intValue;
        }
        const float getFloat() const {
            dbgassert(isFloat(), "Cannot get float value when literal value is not Float");
            return std::stof(_floatStr);
        }
        const bool getBool() const {
            dbgassert(isBool(), "Cannot get bool value when literal value is not Bool");
            return _boolValue;
        }
        const String getString() const {
            dbgassert(isString(), "Cannot get string value when literal value is not String");
            return _stringValue;
        }

        String toString() const;

        bool operator==(const Literal &other) const {
            if (this->_dataType != other._dataType) {
                return false;
            }
            if (isInt()) {
                return _intValue == other._intValue;
            }
            if (isFloat()) {
                return _floatStr == other._floatStr;
            }
            if (isBool()) {
                return _boolValue == other._boolValue;
            }
            if (isString()) {
                return _stringValue == other._stringValue;
            }
            return false;
        }
        bool operator!=(const Literal &other) const {
            return !(*this == other);
        }
        operator float() const {
            if (isInt()) {
                return (float)getInt();
            } else if (isFloat()) {
                return getFloat();
            } else if (isBool()) {
                return (float)getBool();
            } else {
                dbgassert(false, "Not convertible to float");
            }
            return 0.0f;
        }
        operator int() const {
            if (isInt()) {
                return getInt();
            } else if (isFloat()) {
                return (int)getFloat();
            } else if (isBool()) {
                return (int)getBool();
            } else {
                dbgassert(false, "Not convertible to int");
            }
            return 0;
        }
        operator bool() const {
            if (isInt()) {
                return (bool)getInt();
            } else if (isFloat()) {
                return (bool)getFloat();
            } else if (isBool()) {
                return getBool();
            } else {
                dbgassert(false, "Not convertible to bool");
            }
            return false;
        }
        Literal cast(DataType toType) const {
            if (_dataType == toType) {
                return Literal{*this};
            } else if (toType.isPrimary()) {
                if (toType == PrimaryDataType::Int) {
                    return (int)*this;
                } else if (toType == PrimaryDataType::Float) {
                    return (float)*this;
                } else if (toType == PrimaryDataType::Bool) {
                    return (bool)*this;
                }
            }
            dbgassert(false, "Unsupported cast");
            return *this;
        }
    };

    /// @brief Can be a literal value or a register
    struct Value {
        friend struct std::hash<Value>;

    public:
        enum class Type { Unspecified,
                          Literal,
                          Register,
                          ArrayInitializer };

    private:
        Type _valueType{Type::Unspecified};
        Literal _literalValue{};
        RegPtr _registerValue = nullptr;
        OrderedMap<int, Ptr<Value>> _arrayInitMap{}; // I have to use Ptr<Value> since std::unordered_map need the complete type of the value...
        DataType _arrayDataType{PrimaryDataType::Unknown};

    public: /* constructors */
        Value() { }
        /// @brief Constructor for literal value
        Value(const Literal &literal) : _valueType(Type::Literal), _literalValue(literal) { }
        /// @brief Constructor for register
        Value(const RegPtr &reg) : _valueType(Type::Register), _registerValue(reg) { }
        /// @brief Constructor for an array initializer
        Value(const OrderedMap<int, Ptr<Value>> &arrayInitMap, const DataType &arrayDataType) : _valueType(Type::ArrayInitializer), _arrayInitMap(arrayInitMap), _arrayDataType(arrayDataType) { }
        /// @brief Copy constructor
        /// @param other
        Value(const Value &other) = default;

    public: /* properties */
        const Type &valueType() const { return _valueType; }
        DataType dataType() const;
        bool isUnspecified() const {
            return _valueType == Type::Unspecified;
        }
        bool hasValue() const {
            return !isUnspecified();
        }
        bool isLiteral() const {
            return _valueType == Type::Literal;
        }
        bool isRegister() const {
            return _valueType == Type::Register;
        }
        bool isArrayInitializer() const {
            return _valueType == Type::ArrayInitializer;
        }

    public: /* methods */
        Literal &getLiteral() {
            dbgassert(_valueType == Type::Literal, "Cannot get literal when value type is not Literal");
            return _literalValue;
        }
        const Literal &getLiteral() const {
            dbgassert(_valueType == Type::Literal, "Cannot get literal when value type is not Literal");
            return _literalValue;
        }
        RegPtr &getRegister() {
            dbgassert(_valueType == Type::Register, "Cannot get register when value type is not Register");
            return _registerValue;
        }
        const RegPtr &getRegister() const {
            dbgassert(_valueType == Type::Register, "Cannot get register when value type is not Register");
            return _registerValue;
        }
        OrderedMap<int, Ptr<Value>> &getArrayInitMap() {
            dbgassert(_valueType == Type::ArrayInitializer, "Cannot get vector when value type is not Vector");
            return _arrayInitMap;
        }
        const OrderedMap<int, Ptr<Value>> &getArrayInitMap() const {
            dbgassert(_valueType == Type::ArrayInitializer, "Cannot get vector when value type is not Vector");
            return _arrayInitMap;
        }

        bool operator==(const Value &other) const {
            return this->_valueType == other._valueType &&
                   (this->_valueType == Type::Unspecified ||
                    (this->_valueType == Type::Literal
                         ? this->_literalValue == other._literalValue
                         : this->_registerValue == other._registerValue));
        }
        bool operator!=(const Value &other) const {
            return !(*this == other);
        }

        String toString() const;
    };

    /// @brief Virtual register
    class Register {
        friend class ir_builder::Builder;
        friend class DefInst;
        friend class Instruction;

    private:
        RegPtr _self;

        DataType _dataType;
        String _name; // W/o "%" or "@" prefix
        bool _isGlobal = false;
        Value _constVal;
        bool _isAnonymous = false;
        bool _isDiscard = false;
        bool _isRet = false;                    // Return value register
        int _argIndex = -1;                     // If this is a function argument, its index (0-based)
        RegPtr _aliasUnionFindParent = nullptr; // If this is a memory variable, its union find parent for its alias group (i.e. the base memory pointer); if this is not a memory variable, the property should not be used
        FuncPtr _func = nullptr;
        void _init(RegPtr self, FuncPtr func) {
            _self = self;
            _func = func;
            _aliasUnionFindParent = self;
        }

    public: /* constructors */
        Register(const DataType &dataType, const String &name) : _dataType(dataType), _name(name) { }
        static RegPtr create(const FuncPtr &func, const DataType &dataType);
        static RegPtr create(const FuncPtr &func, const DataType &dataType, const String &name);
        ~Register() {
            _func.reset(); // BREAK THE LOOP REFERENCE
        }

    public: /* properties */
        DataType &dataType() { return _dataType; }
        const String &name() const { return _name; }
        bool &isGlobal() { return _isGlobal; }
        /// @brief Constant value (if any)
        Value &constVal() { return _constVal; }
        bool &isAnonymous() { return _isAnonymous; }
        /// @brief The instruction that defines this register (in SSA form)
        /// @brief Discard register (used for discarding values)
        bool &isDiscard() { return _isDiscard; }
        /// @brief Return value register
        bool &isRet() { return _isRet; }
        /// @brief If this is a function argument, its index (0-based)
        int &argIndex() { return _argIndex; }
        bool isArg() { return _argIndex != -1; }

        String toString() const;

    public: /* methods */
        void rename(const String &name, bool unique = true);
        void joinAliasGroup(RegPtr dest);
        RegPtr findAliasGroup();
    };

    struct RegPtrRef {
        friend class std::hash<RegPtrRef>;
        friend class Instruction;

    private:
        RegPtr *_ref;               // Address of the slot
        Value *_valueRef = nullptr; // Added: address of the value (if any)
        /// @brief Change the value stored in the slot
        /// @param value New value to be stored in the slot
        void _replace(Value value) {
            if (this->_valueRef) {
                *this->_valueRef = value;
                if (!value.isRegister()) {
                    this->_ref = nullptr;
                }
            } else {
                dbgassert(value.isRegister(), "Cannot assign non-register value to register-only reference");
                *this->_ref = value.getRegister();
            }
        }

    public:
        RegPtrRef() : _ref(nullptr) { }
        RegPtrRef(RegPtr *ref) : _ref(ref) { }
        RegPtrRef(Value *valueRef) : _ref(&(*valueRef).getRegister()), _valueRef(valueRef) { } // Added
        RegPtrRef(const RegPtrRef &other) {
            _ref = other._ref;
            _valueRef = other._valueRef;
        }
        RegPtrRef(RegPtrRef &&other) = default;
        ~RegPtrRef() = default;

        RegPtrRef &operator=(const RegPtrRef &other) = default;

        bool operator==(const RegPtrRef &other) const {
            return *this->_ref == *other._ref && this->_valueRef == other._valueRef;
        }
        bool operator!=(const RegPtrRef &other) const {
            return !(*this == other);
        }
        operator bool() const {
            return *_ref != nullptr;
        }
        RegPtr &get() const { return *_ref; }
    };

    struct PhiTuple {
        friend struct std::hash<PhiTuple>;

    private:
        BBPtr _bb;
        Value _value;

    public: /* constructors */
        PhiTuple(const BBPtr &bb, const Value &value) : _bb(bb), _value(value) { }
        PhiTuple(const PhiTuple &other) = default;

    public: /* properties */
        BBPtr &bb() { return _bb; }
        const BBPtr &bb() const { return _bb; }
        Value &value() { return _value; }
        const Value &value() const { return _value; }

    public: /* methods */
        String toString() const;

        bool operator==(const PhiTuple &other) const {
            return this->_bb == other._bb && this->_value == other._value;
        }
        bool operator!=(const PhiTuple &other) const {
            return !(*this == other);
        }
    };

    /// @brief Variable can be a register (e.g. local variable, function argument) or the memory location pointed by a register (e.g. array element, global variable), so that we can use the same logic to handle register variables and memory variables
    struct Variable {
        friend struct std::hash<Variable>;

    private:
        RegPtr _reg = nullptr;
        RegPtr _memPtr = nullptr;
        Vector<Variable> _excepts{}; // WHY can't I use std::unordered_set anyway??? Tell me how to use it without using pointers of some stuff
        Variable(RegPtr reg, RegPtr memPtr) : _reg(reg), _memPtr(memPtr) { }

    public:
        Variable() { }
        Variable(const Variable &other) = default;
        static Variable reg(RegPtr reg) {
            return Variable{reg, nullptr};
        }
        static Variable mem(RegPtr memPtr) {
            dbgassert(memPtr->dataType().isPointer(), "Memory pointer must be a pointer type");
            return Variable{nullptr, memPtr};
        }

    public:
        bool hasExcept(Variable var) const {
            return std::find(_excepts.begin(), _excepts.end(), var) != _excepts.end();
        }
        void addExcept(Variable var) {
            if (!hasExcept(var)) {
                _excepts.push_back(var);
            }
        }

        bool isReg() const {
            return _reg != nullptr;
        }
        bool isMem() const {
            return _memPtr != nullptr;
        }
        bool isNull() const {
            return _reg == nullptr && _memPtr == nullptr;
        }
        RegPtr &getReg() {
            dbgassert(isReg(), "Cannot get register when variable is not a register");
            return _reg;
        }
        const RegPtr &getReg() const {
            dbgassert(isReg(), "Cannot get register when variable is not a register");
            return _reg;
        }
        RegPtr &getMemPtr() {
            dbgassert(isMem(), "Cannot get memory pointer when variable is not a memory variable");
            return _memPtr;
        }
        const RegPtr &getMemPtr() const {
            dbgassert(isMem(), "Cannot get memory pointer when variable is not a memory variable");
            return _memPtr;
        }
        Variable getBaseVar() const {
            return isReg() ? *this : Variable::mem(getMemPtr()->findAliasGroup());
        }

        bool operator==(const Variable &other) const {
            return this->_reg == other._reg && this->_memPtr == other._memPtr;
        }
        bool operator!=(const Variable &other) const {
            return !(*this == other);
        }
        String toString() const;
    };

    struct Definition {
        friend struct std::hash<Definition>;

    public:
        Variable var{};
        InstPtr inst = nullptr;
        Definition() { }
        Definition(const Variable &var, InstPtr inst) : var(var), inst(inst) { }

        bool operator==(const Definition &other) const {
            return this->var == other.var && this->inst == other.inst;
        }
        bool operator!=(const Definition &other) const {
            return !(*this == other);
        }
        String toString() const;
    };

} // namespace ir

namespace std {
    template <>
    struct hash<ir::RegPtrRef> {
        std::size_t operator()(const ir::RegPtrRef &ptrRef) const {
            std::size_t result = 0;
            if (ptrRef._ref) {
                hashCombine(result, *ptrRef._ref);
            }
            if (ptrRef._valueRef) {
                hashCombine(result, *ptrRef._valueRef);
            }
            return result;
        }
    };

    template <>
    struct hash<ir::Literal> {
        std::size_t operator()(const ir::Literal &literal) const {
            std::size_t result = 0;
            hashCombine(result, literal._dataType);
            if (literal.isInt()) {
                hashCombine(result, literal._intValue);
            } else if (literal.isFloat()) {
                hashCombine(result, literal._floatStr);
            } else if (literal.isBool()) {
                hashCombine(result, literal._boolValue);
            } else if (literal.isString()) {
                hashCombine(result, literal._stringValue);
            }
            return result;
        }
    };

    template <>
    struct hash<ir::Value> {
        std::size_t operator()(const ir::Value &value) const {
            std::size_t result = 0;
            hashCombine(result, value._valueType);
            if (value._valueType == ir::Value::Type::Literal) {
                hashCombine(result, value._literalValue);
            } else if (value._valueType == ir::Value::Type::Register) {
                hashCombine(result, value._registerValue);
            } else if (value._valueType == ir::Value::Type::ArrayInitializer) {
                for (auto &[k, v] : value._arrayInitMap) {
                    hashCombine(result, k);
                    hashCombine(result, *v);
                }
                hashCombine(result, value._arrayDataType);
            }
            return result;
        }
    };

    template <>
    struct hash<ir::PhiTuple> {
        std::size_t operator()(const ir::PhiTuple &phiTuple) const {
            std::size_t result = 0;
            hashCombine(result, phiTuple._bb);
            hashCombine(result, phiTuple._value);
            return result;
        }
    };

    template <>
    struct hash<ir::Variable> {
        std::size_t operator()(const ir::Variable &variable) const {
            std::size_t result = 0;
            hashCombine(result, variable._reg);
            hashCombine(result, variable._memPtr);
            return result;
        }
    };

    template <>
    struct hash<ir::Definition> {
        std::size_t operator()(const ir::Definition &definition) const {
            std::size_t result = 0;
            hashCombine(result, definition.var);
            hashCombine(result, definition.inst);
            return result;
        }
    };
}
