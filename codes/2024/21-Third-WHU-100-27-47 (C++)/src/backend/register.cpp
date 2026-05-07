#ifndef BACKEND_REGISTER_CPP_
#define BACKEND_REGISTER_CPP_
#include "register.h"

namespace backend {

    bool Register::isTmpReg() const {
        return *this == *REG_TEMP_GENERAL[0] || *this == *REG_TEMP_GENERAL[1] || *this == *REG_TEMP_GENERAL[2] || *this == *REG_TEMP_FLOAT[0] || *this == *REG_TEMP_FLOAT[1] || *this == *REG_TEMP_FLOAT[2];
    }

    // GeneralRegister methods

    std::string GeneralRegister::regString() const {
        static const std::unordered_map<Id, std::string> idToString = {
            {Id::Zero, "zero"}, {Id::Ra, "ra"}, {Id::Sp, "sp"}, {Id::Gp, "gp"}, {Id::Tp, "tp"}, {Id::T0, "t0"}, {Id::T1, "t1"}, {Id::T2, "t2"}, {Id::S0, "s0"}, {Id::S1, "s1"}, {Id::A0, "a0"}, {Id::A1, "a1"}, {Id::A2, "a2"}, {Id::A3, "a3"}, {Id::A4, "a4"}, {Id::A5, "a5"}, {Id::A6, "a6"}, {Id::A7, "a7"}, {Id::S2, "s2"}, {Id::S3, "s3"}, {Id::S4, "s4"}, {Id::S5, "s5"}, {Id::S6, "s6"}, {Id::S7, "s7"}, {Id::S8, "s8"}, {Id::S9, "s9"}, {Id::S10, "s10"}, {Id::S11, "s11"}, {Id::T3, "t3"}, {Id::T4, "t4"}, {Id::T5, "t5"}, {Id::T6, "t6"}};
        return idToString.at(id);
    }

    int GeneralRegister::getNumber() const {
        return static_cast<int>(id);
    }

    // FloatRegister methods

    std::string FloatRegister::regString() const {
        static const std::unordered_map<Id, std::string> idToString = {
            {Id::Ft0, "ft0"}, {Id::Ft1, "ft1"}, {Id::Ft2, "ft2"}, {Id::Ft3, "ft3"}, {Id::Ft4, "ft4"}, {Id::Ft5, "ft5"}, {Id::Ft6, "ft6"}, {Id::Ft7, "ft7"}, {Id::Fs0, "fs0"}, {Id::Fs1, "fs1"}, {Id::Fa0, "fa0"}, {Id::Fa1, "fa1"}, {Id::Fa2, "fa2"}, {Id::Fa3, "fa3"}, {Id::Fa4, "fa4"}, {Id::Fa5, "fa5"}, {Id::Fa6, "fa6"}, {Id::Fa7, "fa7"}, {Id::Fs2, "fs2"}, {Id::Fs3, "fs3"}, {Id::Fs4, "fs4"}, {Id::Fs5, "fs5"}, {Id::Fs6, "fs6"}, {Id::Fs7, "fs7"}, {Id::Fs8, "fs8"}, {Id::Fs9, "fs9"}, {Id::Fs10, "fs10"}, {Id::Fs11, "fs11"}, {Id::Ft8, "ft8"}, {Id::Ft9, "ft9"}, {Id::Ft10, "ft10"}, {Id::Ft11, "ft11"}};
        return idToString.at(id);
    }

    int FloatRegister::getNumber() const {
        return static_cast<int>(id);
    }

    // Register base class methods
    // Register base class methods
    bool Register::operator==(const Register &other) const {
        return isVirtual == other.isVirtual && getNumber() == other.getNumber() && getType() == other.getType();
    }

    bool Register::operator!=(const Register &other) const {
        return !(*this == other);
    }

    bool Register::operator<(const Register &other) const {
        if (isVirtual != other.isVirtual) {
            return isVirtual < other.isVirtual;
        }
        if (getType() != other.getType()) {
            return getType() < other.getType();
        }
        return getNumber() < other.getNumber();
    }

    // Utility functions
    std::shared_ptr<VirtualRegister> createVirtualRegister(int number, ir::RegPtr irReg, Register::Type type) {
        return std::make_unique<VirtualRegister>(number, irReg, type);
    }

    std::shared_ptr<GeneralRegister> createGeneralRegister(GeneralRegister::Id id) {
        return std::make_unique<GeneralRegister>(id);
    }

    std::shared_ptr<FloatRegister> createFloatRegister(FloatRegister::Id id) {
        return std::make_unique<FloatRegister>(id);
    }

    // Register sets for allocation, spilling, etc.
    const std::vector<std::shared_ptr<Register>> REG_ALLOC = {
        createGeneralRegister(GeneralRegister::Id::T3),
        createGeneralRegister(GeneralRegister::Id::T4),
        createGeneralRegister(GeneralRegister::Id::T5),
        createGeneralRegister(GeneralRegister::Id::T6),

        createGeneralRegister(GeneralRegister::Id::A7),
        createGeneralRegister(GeneralRegister::Id::A6),
        createGeneralRegister(GeneralRegister::Id::A5),
        createGeneralRegister(GeneralRegister::Id::A4),
        createGeneralRegister(GeneralRegister::Id::A3),
        createGeneralRegister(GeneralRegister::Id::A2),
        createGeneralRegister(GeneralRegister::Id::A1),
        createGeneralRegister(GeneralRegister::Id::A0),

        createGeneralRegister(GeneralRegister::Id::S1),
        createGeneralRegister(GeneralRegister::Id::S2),
        createGeneralRegister(GeneralRegister::Id::S3),
        createGeneralRegister(GeneralRegister::Id::S4),
        createGeneralRegister(GeneralRegister::Id::S5),
        createGeneralRegister(GeneralRegister::Id::S6),
        createGeneralRegister(GeneralRegister::Id::S7),
        createGeneralRegister(GeneralRegister::Id::S8),
        createGeneralRegister(GeneralRegister::Id::S9),
        createGeneralRegister(GeneralRegister::Id::S10),
        createGeneralRegister(GeneralRegister::Id::S11),

        createFloatRegister(FloatRegister::Id::Ft3),
        createFloatRegister(FloatRegister::Id::Ft4),
        createFloatRegister(FloatRegister::Id::Ft5),
        createFloatRegister(FloatRegister::Id::Ft6),
        createFloatRegister(FloatRegister::Id::Ft7),
        createFloatRegister(FloatRegister::Id::Ft8),
        createFloatRegister(FloatRegister::Id::Ft9),
        createFloatRegister(FloatRegister::Id::Ft10),
        createFloatRegister(FloatRegister::Id::Ft11),

        createFloatRegister(FloatRegister::Id::Fa7),
        createFloatRegister(FloatRegister::Id::Fa6),
        createFloatRegister(FloatRegister::Id::Fa5),
        createFloatRegister(FloatRegister::Id::Fa4),
        createFloatRegister(FloatRegister::Id::Fa3),
        createFloatRegister(FloatRegister::Id::Fa2),
        createFloatRegister(FloatRegister::Id::Fa1),
        createFloatRegister(FloatRegister::Id::Fa0),

        createFloatRegister(FloatRegister::Id::Fs1),
        createFloatRegister(FloatRegister::Id::Fs2),
        createFloatRegister(FloatRegister::Id::Fs3),
        createFloatRegister(FloatRegister::Id::Fs4),
        createFloatRegister(FloatRegister::Id::Fs5),
        createFloatRegister(FloatRegister::Id::Fs6),
        createFloatRegister(FloatRegister::Id::Fs7),
        createFloatRegister(FloatRegister::Id::Fs8),
        createFloatRegister(FloatRegister::Id::Fs9),
        createFloatRegister(FloatRegister::Id::Fs10),
        createFloatRegister(FloatRegister::Id::Fs11),

    };

    const std::vector<std::shared_ptr<Register>> REG_SPILL_GENERAL = {
        createGeneralRegister(GeneralRegister::Id::T0),
        createGeneralRegister(GeneralRegister::Id::T1),
    };

    const std::vector<std::shared_ptr<Register>> REG_SPILL_FLOAT = {
        createFloatRegister(FloatRegister::Id::Ft0),
        createFloatRegister(FloatRegister::Id::Ft1),
        createFloatRegister(FloatRegister::Id::Ft2),
    };

    const std::set<std::shared_ptr<Register>, Register::RegisterPtrHash> REG_CALLER_SAVED = {
        createGeneralRegister(GeneralRegister::Id::Ra),

        createGeneralRegister(GeneralRegister::Id::A0),
        createGeneralRegister(GeneralRegister::Id::A1),
        createGeneralRegister(GeneralRegister::Id::A2),
        createGeneralRegister(GeneralRegister::Id::A3),
        createGeneralRegister(GeneralRegister::Id::A4),
        createGeneralRegister(GeneralRegister::Id::A5),
        createGeneralRegister(GeneralRegister::Id::A6),
        createGeneralRegister(GeneralRegister::Id::A7),

        createGeneralRegister(GeneralRegister::Id::T3),
        createGeneralRegister(GeneralRegister::Id::T4),
        createGeneralRegister(GeneralRegister::Id::T5),
        createGeneralRegister(GeneralRegister::Id::T6),

        createFloatRegister(FloatRegister::Id::Fa0),
        createFloatRegister(FloatRegister::Id::Fa1),
        createFloatRegister(FloatRegister::Id::Fa2),
        createFloatRegister(FloatRegister::Id::Fa3),
        createFloatRegister(FloatRegister::Id::Fa4),
        createFloatRegister(FloatRegister::Id::Fa5),
        createFloatRegister(FloatRegister::Id::Fa6),
        createFloatRegister(FloatRegister::Id::Fa7),

        createFloatRegister(FloatRegister::Id::Ft0),
        createFloatRegister(FloatRegister::Id::Ft1),
        createFloatRegister(FloatRegister::Id::Ft2),
        createFloatRegister(FloatRegister::Id::Ft3),
        createFloatRegister(FloatRegister::Id::Ft4),
        createFloatRegister(FloatRegister::Id::Ft5),
        createFloatRegister(FloatRegister::Id::Ft6),
        createFloatRegister(FloatRegister::Id::Ft7),
        createFloatRegister(FloatRegister::Id::Ft8),
        createFloatRegister(FloatRegister::Id::Ft9),
        createFloatRegister(FloatRegister::Id::Ft10),
        createFloatRegister(FloatRegister::Id::Ft11),
    };

    const std::set<std::shared_ptr<Register>, Register::RegisterPtrHash> REG_CALLEE_SAVED = {
        createGeneralRegister(GeneralRegister::Id::Ra),
        createGeneralRegister(GeneralRegister::Id::Gp),
        createGeneralRegister(GeneralRegister::Id::Tp),

        createGeneralRegister(GeneralRegister::Id::S0),
        createGeneralRegister(GeneralRegister::Id::S1),
        createGeneralRegister(GeneralRegister::Id::S2),
        createGeneralRegister(GeneralRegister::Id::S3),
        createGeneralRegister(GeneralRegister::Id::S4),
        createGeneralRegister(GeneralRegister::Id::S5),
        createGeneralRegister(GeneralRegister::Id::S6),
        createGeneralRegister(GeneralRegister::Id::S7),
        createGeneralRegister(GeneralRegister::Id::S8),
        createGeneralRegister(GeneralRegister::Id::S9),
        createGeneralRegister(GeneralRegister::Id::S10),
        createGeneralRegister(GeneralRegister::Id::S11),

        createFloatRegister(FloatRegister::Id::Fs0),
        createFloatRegister(FloatRegister::Id::Fs1),
        createFloatRegister(FloatRegister::Id::Fs2),
        createFloatRegister(FloatRegister::Id::Fs3),
        createFloatRegister(FloatRegister::Id::Fs4),
        createFloatRegister(FloatRegister::Id::Fs5),
        createFloatRegister(FloatRegister::Id::Fs6),
        createFloatRegister(FloatRegister::Id::Fs7),
        createFloatRegister(FloatRegister::Id::Fs8),
        createFloatRegister(FloatRegister::Id::Fs9),
        createFloatRegister(FloatRegister::Id::Fs10),
        createFloatRegister(FloatRegister::Id::Fs11),
    };

    const std::set<std::shared_ptr<Register>, Register::RegisterPtrHash> REG_SAVE_GENERAL = {
        createGeneralRegister(GeneralRegister::Id::S1),
        createGeneralRegister(GeneralRegister::Id::S2),
        createGeneralRegister(GeneralRegister::Id::S3),
        createGeneralRegister(GeneralRegister::Id::S4),
        createGeneralRegister(GeneralRegister::Id::S5),
        createGeneralRegister(GeneralRegister::Id::S6),
        createGeneralRegister(GeneralRegister::Id::S7),
        createGeneralRegister(GeneralRegister::Id::S8),
        createGeneralRegister(GeneralRegister::Id::S9),
        createGeneralRegister(GeneralRegister::Id::S10),
        createGeneralRegister(GeneralRegister::Id::S11),
    };

    const std::set<std::shared_ptr<Register>, Register::RegisterPtrHash> REG_SAVE_FLOAT = {
        createFloatRegister(FloatRegister::Id::Fs1),
        createFloatRegister(FloatRegister::Id::Fs2),
        createFloatRegister(FloatRegister::Id::Fs3),
        createFloatRegister(FloatRegister::Id::Fs4),
        createFloatRegister(FloatRegister::Id::Fs5),
        createFloatRegister(FloatRegister::Id::Fs6),
        createFloatRegister(FloatRegister::Id::Fs7),
        createFloatRegister(FloatRegister::Id::Fs8),
        createFloatRegister(FloatRegister::Id::Fs9),
        createFloatRegister(FloatRegister::Id::Fs10),
        createFloatRegister(FloatRegister::Id::Fs11),
    };

    const std::vector<std::shared_ptr<Register>> REG_ARGS_GENERAL = {
        createGeneralRegister(GeneralRegister::Id::A0),
        createGeneralRegister(GeneralRegister::Id::A1),
        createGeneralRegister(GeneralRegister::Id::A2),
        createGeneralRegister(GeneralRegister::Id::A3),
        createGeneralRegister(GeneralRegister::Id::A4),
        createGeneralRegister(GeneralRegister::Id::A5),
        createGeneralRegister(GeneralRegister::Id::A6),
        createGeneralRegister(GeneralRegister::Id::A7),
    };

    const std::vector<std::shared_ptr<Register>> REG_ARGS_FLOAT = {
        createFloatRegister(FloatRegister::Id::Fa0),
        createFloatRegister(FloatRegister::Id::Fa1),
        createFloatRegister(FloatRegister::Id::Fa2),
        createFloatRegister(FloatRegister::Id::Fa3),
        createFloatRegister(FloatRegister::Id::Fa4),
        createFloatRegister(FloatRegister::Id::Fa5),
        createFloatRegister(FloatRegister::Id::Fa6),
        createFloatRegister(FloatRegister::Id::Fa7),
    };

    const std::vector<std::shared_ptr<Register>> REG_TEMP_GENERAL = {
        createGeneralRegister(GeneralRegister::Id::T0),
        createGeneralRegister(GeneralRegister::Id::T1),
        createGeneralRegister(GeneralRegister::Id::T2),
    };

    const std::vector<std::shared_ptr<Register>> REG_TEMP_FLOAT = {
        createFloatRegister(FloatRegister::Id::Ft0),
        createFloatRegister(FloatRegister::Id::Ft1),
        createFloatRegister(FloatRegister::Id::Ft2),
    };

    std::shared_ptr<Register> zero = initZero();
    std::shared_ptr<Register> initZero() {
        return createGeneralRegister(GeneralRegister::Id::Zero);
    }
    std::shared_ptr<Register> s0 = initS0Register();
    std::shared_ptr<Register> initS0Register() {
        return createGeneralRegister(GeneralRegister::Id::S0);
    }
    std::shared_ptr<Register> ra = initRaRegister();
    std::shared_ptr<Register> initRaRegister() {
        return createGeneralRegister(GeneralRegister::Id::Ra);
    }
    std::shared_ptr<Register> sp = initSpRegister();
    std::shared_ptr<Register> initSpRegister() {
        return createGeneralRegister(GeneralRegister::Id::Sp);
    }

} // namespace backend
#endif
