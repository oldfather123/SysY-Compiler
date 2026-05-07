#ifndef BACKEND_REGISTER_H_
#define BACKEND_REGISTER_H_

#include "Common.h"
#include "IR.h"

namespace backend {
    class VirtualRegister;

    // Base class for all registers to ensure common interface
    class Register {
    public:
        enum class Type {
            GENERAL,
            FLOAT,
        } regType{Type::GENERAL};

        bool isVirtual = false;

        virtual ~Register() = default;
        Type getType() const { return regType; };
        virtual std::string regString() const = 0;
        virtual int getNumber() const = 0;

        bool operator==(const Register &other) const;
        bool operator!=(const Register &other) const;
        bool operator<(const Register &other) const;

        bool isTmpReg() const;

        Register() = default;
        Register(Type regType, bool isVirtual) : regType(regType), isVirtual(isVirtual) { }

#ifdef DEBUG
        String regStringCache = "";
#endif

        struct RegisterPtrHash {
            std::size_t operator()(const std::shared_ptr<Register> &reg) const {
                size_t result = 0;
                hashCombine(result, reg->isVirtual);
                hashCombine(result, reg->getNumber());
                hashCombine(result, static_cast<int>(reg->getType()));
                return result;
            }
            bool operator()(const std::shared_ptr<Register> &lhs, const std::shared_ptr<Register> &rhs) const {
                return *lhs < *rhs;
            }
        };
    };

    // General purpose registers
    class GeneralRegister : public Register {
    public:
        enum class Id {
            Zero = 0,
            Ra,
            Sp,
            Gp,
            Tp,
            T0,
            T1,
            T2,
            S0,
            S1,
            A0,
            A1,
            A2,
            A3,
            A4,
            A5,
            A6,
            A7,
            S2,
            S3,
            S4,
            S5,
            S6,
            S7,
            S8,
            S9,
            S10,
            S11,
            T3,
            T4,
            T5,
            T6
        };

        GeneralRegister(Id id) : Register(Register::Type::GENERAL, false), id(id) {
#ifdef DEBUG
            regStringCache = regString();
#endif
        }

        std::string regString() const override;
        int getNumber() const override;

    private:
        Id id;
    };

    // Floating point registers
    class FloatRegister : public Register {
    public:
        enum class Id {
            Ft0 = 0,
            Ft1,
            Ft2,
            Ft3,
            Ft4,
            Ft5,
            Ft6,
            Ft7,
            Fs0,
            Fs1,
            Fa0,
            Fa1,
            Fa2,
            Fa3,
            Fa4,
            Fa5,
            Fa6,
            Fa7,
            Fs2,
            Fs3,
            Fs4,
            Fs5,
            Fs6,
            Fs7,
            Fs8,
            Fs9,
            Fs10,
            Fs11,
            Ft8,
            Ft9,
            Ft10,
            Ft11
        };

        FloatRegister(Id id) : Register(Register::Type::FLOAT, false), id(id) {
#ifdef DEBUG
            regStringCache = regString();
#endif
        }

        std::string regString() const override;
        int getNumber() const override;

    private:
        Id id;
    };

    class VirtualRegister : public Register {
    public:
        VirtualRegister(int number, ir::RegPtr irReg, Register::Type regType)
            : Register(regType, true), number(number), irReg(irReg) {
#ifdef DEBUG
            regStringCache = regString();
#endif
        }

        std::string regString() const override {
            return "V" + std::to_string(number) + "_" + irReg->toString();
        }

        int getNumber() const override {
            return number;
        }

        auto getSourceReg() const {
            return irReg;
        }

        Ptr<Register> assignedReg = nullptr; // Valid if not nullptr
        int memVarOffset = -1;               // Valid if >= 0
        int spillOffset = -1;                // Valid if >= 0
        int memArgOffset = -1;               // Valid if >= 0
        bool boundToReg = false;

        bool isBound() const { return boundToReg; }
        bool isAssigned() const { return assignedReg != nullptr; }
        bool isMemVar() const { return memVarOffset != -1; }
        bool isSpill() const { return spillOffset != -1; }
        bool isArg() const { return irReg->isArg(); }
        bool isGlobal() const { return irReg->isGlobal(); }
        bool isAllocated() const { return isAssigned() || isSpill() || memArgOffset != -1 || isGlobal(); }

    private:
        int number;
        ir::RegPtr irReg;
    };

    // Utility functions to create registers
    std::shared_ptr<VirtualRegister> createVirtualRegister(int, ir::RegPtr, Register::Type);
    std::shared_ptr<GeneralRegister> createGeneralRegister(GeneralRegister::Id id);
    std::shared_ptr<FloatRegister> createFloatRegister(FloatRegister::Id id);

    // Register sets for allocation, spilling, etc.
    extern std::shared_ptr<Register> zero;
    extern std::shared_ptr<Register> s0;
    extern std::shared_ptr<Register> ra;
    extern std::shared_ptr<Register> sp;
    std::shared_ptr<Register> initZero();
    std::shared_ptr<Register> initS0Register();
    std::shared_ptr<Register> initRaRegister();
    std::shared_ptr<Register> initSpRegister();
    extern const std::vector<std::shared_ptr<Register>> REG_ALLOC;
    extern const std::vector<std::shared_ptr<Register>> REG_SPILL_GENERAL;
    extern const std::vector<std::shared_ptr<Register>> REG_SPILL_FLOAT;
    extern const std::set<std::shared_ptr<Register>, Register::RegisterPtrHash> REG_CALLEE_SAVED;
    extern const std::set<std::shared_ptr<Register>, Register::RegisterPtrHash> REG_CALLER_SAVED;
    extern const std::set<std::shared_ptr<Register>, Register::RegisterPtrHash> REG_SAVE_GENERAL;
    extern const std::set<std::shared_ptr<Register>, Register::RegisterPtrHash> REG_SAVE_FLOAT;
    extern const std::vector<std::shared_ptr<Register>> REG_ARGS_GENERAL;
    extern const std::vector<std::shared_ptr<Register>> REG_ARGS_FLOAT;
    extern const std::vector<std::shared_ptr<Register>> REG_TEMP_GENERAL;
    extern const std::vector<std::shared_ptr<Register>> REG_TEMP_FLOAT;

} // namespace backend

#endif
