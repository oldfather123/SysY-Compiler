#include "opt/exec/interpreter.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <limits>
#include <unordered_map>
#include <unordered_set>

#include "ir/instruction.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"

namespace opt::exec {

using Clock = std::chrono::steady_clock;

// Byte-level simulation state
enum class ByteState : uint8_t { Read = 1 << 0, Write = 1 << 1, None = 0 };
static inline ByteState operator|(ByteState a, ByteState b) {
    return static_cast<ByteState>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
static inline ByteState operator&(ByteState a, ByteState b) {
    return static_cast<ByteState>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
static inline ByteState operator~(ByteState a) { return static_cast<ByteState>(~static_cast<uint8_t>(a)); }
static inline bool hasTag(ByteState provided, ByteState required) { return (provided & required) == required; }

class MemoryContext final {
    using ByteStorage = std::pair<ByteState, std::byte>;

    static constexpr uintptr_t globalOffset = 0xf000000000000000ULL;
    static constexpr uintptr_t stackOffset = 0x1000000000000000ULL;
    static constexpr std::byte invalidByte{0xCC};
    static constexpr ByteStorage invalid{ByteState::None, invalidByte};

    size_t mBudget;
    bool mHasMemoryError = false;
    std::vector<ByteStorage> mGlobal;
    std::unordered_map<std::shared_ptr<ir::GlobalVariable>, uintptr_t> mGlobalAlloc;
    std::vector<ByteStorage> mStack;
    std::vector<std::pair<uintptr_t, uintptr_t>> mStackAlloc;
    std::unordered_set<uintptr_t> mPoppedBase;

    static void extendTo(std::vector<ByteStorage> &storage, size_t size) {
        storage.reserve(size);
        while (storage.size() < size) storage.push_back(invalid);
    }
    static uintptr_t padTo(std::vector<ByteStorage> &storage, size_t base, size_t align) {
        const auto pos = (base / align + 1) * align;
        extendTo(storage, pos);
        for (size_t i = base; i < pos; ++i) storage[i] = invalid;
        return pos;
    }
    static void setTag(std::vector<ByteStorage> &storage, size_t base, size_t size, ByteState tag) {
        extendTo(storage, base + size);
        for (size_t i = 0; i < size; ++i) storage[base + i].first = tag;
    }
    static void removeTag(std::vector<ByteStorage> &storage, size_t base, size_t size, ByteState tag) {
        extendTo(storage, base + size);
        for (size_t i = 0; i < size; ++i)
            storage[base + i].first = static_cast<ByteState>(storage[base + i].first & ~tag);
    }

public:
    explicit MemoryContext(size_t budget) : mBudget(budget) { mStackAlloc.emplace_back(0U, 16U); }

    bool isExceeded() const noexcept { return (mGlobal.size() + mStack.size()) * sizeof(ByteStorage) > mBudget; }
    bool hasException() const noexcept { return mHasMemoryError; }
    void triggerMemoryError() { mHasMemoryError = true; }

    std::byte loadByte(std::vector<ByteStorage> &storage, uintptr_t ptr) {
        if (ptr < storage.size()) {
            const auto val = storage[ptr];
            if (hasTag(val.first, ByteState::Read)) return val.second;
        }
        triggerMemoryError();
        return invalidByte;
    }
    void storeByte(std::vector<ByteStorage> &storage, uintptr_t ptr, std::byte v) {
        if (ptr < storage.size()) {
            auto &ref = storage[ptr];
            if (hasTag(ref.first, ByteState::Write)) {
                ref.second = v;
                return;
            }
        }
        triggerMemoryError();
    }

    std::byte load(uintptr_t ptr) {
        return ptr >= globalOffset ? loadByte(mGlobal, ptr - globalOffset) : loadByte(mStack, ptr - stackOffset);
    }
    void store(uintptr_t ptr, std::byte v) {
        return ptr >= globalOffset ? storeByte(mGlobal, ptr - globalOffset, v)
                                   : storeByte(mStack, ptr - stackOffset, v);
    }

    uintptr_t getGlobalVarAddress(const std::shared_ptr<ir::GlobalVariable> &var) {
        auto it = mGlobalAlloc.find(var);
        if (it != mGlobalAlloc.end()) return it->second;

        auto refTy = std::dynamic_pointer_cast<ir::PointerType>(var->get_type())->get_reference_type();
        const auto align = 4U;  // minimal alignment for our IR types (i32/f32/pointers)
        const auto size = static_cast<size_t>(refTy->bits_num() / 8);
        const auto base = padTo(mGlobal, mGlobal.size(), align);
        setTag(mGlobal, base, size, ByteState::Read | ByteState::Write);

        // initialize with init or zero
        auto init = var->get_init_value();
        if (init)
            writeValue(base + globalOffset, init, refTy);
        else
            memReset(base + globalOffset, size, std::byte{0});

        mGlobalAlloc.emplace(var, base + globalOffset);
        return base + globalOffset;
    }

    uintptr_t stackPush(size_t size, size_t align) {
        auto [beg, end] = mStackAlloc.back();
        (void)beg;
        const auto base = padTo(mStack, end, align);
        setTag(mStack, base, size, ByteState::Read | ByteState::Write);
        mStackAlloc.emplace_back(base, base + size);
        return base + stackOffset;
    }
    void stackPop(uintptr_t basePtr) {
        auto base = basePtr - stackOffset;
        if (mStackAlloc.back().first == base) {
            const auto popOne = [&] {
                const auto [b, e] = mStackAlloc.back();
                setTag(mStack, b, e - b, ByteState::None);
                mStackAlloc.pop_back();
            };
            popOne();
            while (true) {
                auto it = mPoppedBase.find(mStackAlloc.back().first);
                if (it == mPoppedBase.end()) return;
                popOne();
                mPoppedBase.erase(it);
            }
        } else {
            mPoppedBase.insert(base);
        }
    }

    template <typename T>
    T loadValue(uintptr_t ptr) {
        const auto align = alignof(T);
        if (ptr % align != 0) {
            triggerMemoryError();
            return T{};
        }
        T val{};
        auto *bytes = reinterpret_cast<std::byte *>(&val);
        for (size_t i = 0; i < sizeof(T); ++i) bytes[i] = load(ptr + i);
        return val;
    }

    template <typename T>
    void storeValue(uintptr_t ptr, T value) {
        const auto align = alignof(T);
        if (ptr % align != 0) {
            triggerMemoryError();
            return;
        }
        auto *bytes = reinterpret_cast<const std::byte *>(&value);
        for (size_t i = 0; i < sizeof(T); ++i) store(ptr + i, bytes[i]);
    }

    void memReset(uintptr_t ptr, size_t size, std::byte byte) {
        for (size_t i = 0; i < size; ++i) store(ptr + i, byte);
    }

    void writeValue(uintptr_t ptr, const std::shared_ptr<ir::Constant> &value, const std::shared_ptr<ir::Type> &type) {
        if (type->is_integer_ty()) {
            auto ci = std::dynamic_pointer_cast<ir::ConstantInt>(value);
            uint32_t v = static_cast<uint32_t>(ci->get_val());
            storeValue(ptr, v);
            return;
        }
        if (type->is_float_ty()) {
            auto cf = std::dynamic_pointer_cast<ir::ConstantFloat>(value);
            float v = cf->get_val();
            storeValue(ptr, v);
            return;
        }
        if (type->is_array_ty()) {
            auto arrTy = std::dynamic_pointer_cast<ir::ArrayType>(type);
            auto elemTy = arrTy->get_element_type();
            auto offset = static_cast<size_t>(elemTy->bits_num() / 8);
            auto carr = std::dynamic_pointer_cast<ir::ConstantArray>(value);
            for (int i = 0; i < arrTy->get_size(); ++i) {
                auto dst = ptr + i * offset;
                if (i >= static_cast<int>(carr->get_vals().size()))
                    memReset(dst, offset, std::byte{0});
                else
                    writeValue(dst, carr->get_vals()[i], elemTy);
            }
            return;
        }
        // other types unsupported yet
        triggerMemoryError();
    }
};

struct Frame final {
    std::shared_ptr<ir::Instruction> caller;  // return value receiver
    std::shared_ptr<ir::BasicBlock> block;
    std::shared_ptr<ir::BasicBlock> predBlock;
    ir::InstructionNode execIter;
    std::unordered_map<std::shared_ptr<ir::Value>, RuntimeValue> locals;
    std::unordered_map<uintptr_t, std::shared_ptr<ir::Instruction>> stackAllocs;
};

Interpreter::Interpreter(uint64_t timeBudgetNs, size_t memBudgetBytes, size_t maxRecursiveDepth, bool collectStats)
    : mTimeBudgetNs(timeBudgetNs), mMemBudgetBytes(memBudgetBytes), mMaxRecursiveDepth(maxRecursiveDepth),
      mCollectStats(collectStats) {}

std::variant<std::shared_ptr<ir::Constant>, FailReason> Interpreter::execute(
    ir::Module &module,
    const std::shared_ptr<ir::Function> &func,
    const std::vector<std::shared_ptr<ir::Constant>> &arguments,
    IOContext *io) const {
    (void)io;
    if (!func) return FailReason::Unsupported;
    // type check
    if (func->get_param_types().size() != arguments.size()) return FailReason::Unsupported;

    MemoryContext mem{mMemBudgetBytes};

    std::vector<Frame> stack;
    if (func->get_basic_blocks_ref().empty()) return FailReason::Unsupported;

    // setup entry frame
    Frame entry{};
    entry.caller = nullptr;
    entry.block = func->entry_block();
    entry.predBlock = nullptr;
    entry.execIter = entry.block->get_instructions_ref().begin();
    // helper: convert Constant -> RuntimeValue
    auto constToRV = [&](const std::shared_ptr<ir::Constant> &c) -> std::optional<RuntimeValue> {
        if (auto ci = std::dynamic_pointer_cast<ir::ConstantInt>(c)) return RuntimeValue{*ci};
        if (auto cf = std::dynamic_pointer_cast<ir::ConstantFloat>(c)) return RuntimeValue{*cf};
        return std::nullopt;  // unsupported constant kinds for arguments
    };
    for (size_t i = 0; i < arguments.size(); ++i) {
        auto rv = constToRV(arguments[i]);
        if (!rv) return FailReason::Unsupported;
        entry.locals.emplace(func->get_arguments_ref()[i], *rv);
    }
    stack.push_back(std::move(entry));

    const auto start = Clock::now();
    size_t steps = 0;

    const auto toConst = [&](const RuntimeValue &rv,
                             const std::shared_ptr<ir::Type> &type) -> std::shared_ptr<ir::Constant> {
        if (auto pi = std::get_if<ir::ConstantInt>(&rv))
            return std::make_shared<ir::ConstantInt>(std::dynamic_pointer_cast<ir::IntegerType>(type), pi->get_val());
        if (auto pf = std::get_if<ir::ConstantFloat>(&rv))
            return std::make_shared<ir::ConstantFloat>(std::dynamic_pointer_cast<ir::FloatType>(type), pf->get_val());
        return nullptr;
    };

    auto readScalar = [&](std::shared_ptr<ir::Value> v) -> std::optional<RuntimeValue> {
        if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(v)) return RuntimeValue{*c};
        if (auto c = std::dynamic_pointer_cast<ir::ConstantFloat>(v)) return RuntimeValue{*c};
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            auto fIt = it->locals.find(v);
            if (fIt != it->locals.end()) return fIt->second;
        }
        // function symbol
        if (auto f = std::dynamic_pointer_cast<ir::Function>(v)) return RuntimeValue{f};
        // globals: treat as pointers
        if (auto g = std::dynamic_pointer_cast<ir::GlobalVariable>(v)) {
            return RuntimeValue{mem.getGlobalVarAddress(g)};
        }
        return std::nullopt;
    };

    while (true) {
        // budgets
        if (++steps > mStepBudget) {
            return FailReason::ExceedTimeLimit;
        }
        if (mem.isExceeded() || mem.hasException()) {
            return FailReason::MemoryError;
        }
        auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
        if (elapsed_ns > static_cast<int64_t>(mTimeBudgetNs)) {
            return FailReason::ExceedTimeLimit;
        }

        // progress logging removed

        auto &cur = stack.back();
        auto it = cur.execIter;
        if (it == cur.block->get_instructions_ref().end()) return FailReason::UnknownError;

        // PHI collect
        if (auto phi = std::dynamic_pointer_cast<ir::Phi>(*it)) {
            std::unordered_map<std::shared_ptr<ir::Instruction>, RuntimeValue> phiValues;
            while (it != cur.block->get_instructions_ref().end()) {
                if (!std::dynamic_pointer_cast<ir::Phi>(*it)) break;
                auto curPhi = std::dynamic_pointer_cast<ir::Phi>(*it);
                auto incomings = curPhi->get_operands_ref();
                // [v, block] pairs
                for (size_t i = 0; i < incomings.size(); i += 2) {
                    auto blk = std::dynamic_pointer_cast<ir::BasicBlock>(incomings[i + 1]);
                    if (blk == cur.predBlock) {
                        auto val = readScalar(incomings[i]);
                        if (!val) return FailReason::Unsupported;
                        cur.locals[*it] = *val;
                        break;
                    }
                }
                ++it;
            }
            cur.execIter = it;
            continue;
        }

        // execute one
        auto &inst = *it;
        cur.execIter = std::next(it);

        using IT = ir::Instruction::InstructionType;
        auto opcode = inst->get_ins_type();

        auto binInt = [&](auto f) -> std::optional<RuntimeValue> {
            auto a = readScalar(inst->get_operands_ref()[0]);
            auto b = readScalar(inst->get_operands_ref()[1]);
            if (!a || !b) return std::nullopt;
            auto ia = std::get_if<ir::ConstantInt>(&*a);
            auto ib = std::get_if<ir::ConstantInt>(&*b);
            if (!ia || !ib) return std::nullopt;
            int av = ia->get_val();
            int bv = ib->get_val();
            return RuntimeValue{
                ir::ConstantInt(std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type()), f(av, bv))};
        };

        auto binF = [&](auto f) -> std::optional<RuntimeValue> {
            auto a = readScalar(inst->get_operands_ref()[0]);
            auto b = readScalar(inst->get_operands_ref()[1]);
            if (!a || !b) return std::nullopt;
            auto fa = std::get_if<ir::ConstantFloat>(&*a);
            auto fb = std::get_if<ir::ConstantFloat>(&*b);
            if (!fa || !fb) return std::nullopt;
            float av = fa->get_val();
            float bv = fb->get_val();
            return RuntimeValue{ir::ConstantFloat(ir::FloatType::get(), f(av, bv))};
        };

        switch (opcode) {
            case IT::ADD:
                if (auto rv = binInt([](int a, int b) { return a + b; }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::SUB:
                if (auto rv = binInt([](int a, int b) { return a - b; }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::MUL:
                if (auto rv = binInt([](int a, int b) { return a * b; }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::SDIV: {
                auto a = readScalar(inst->get_operands_ref()[0]);
                auto b = readScalar(inst->get_operands_ref()[1]);
                if (!a || !b) return FailReason::Unsupported;
                auto ia = std::get_if<ir::ConstantInt>(&*a);
                auto ib = std::get_if<ir::ConstantInt>(&*b);
                if (!ia || !ib) return FailReason::Unsupported;
                if (ib->get_val() == 0) return FailReason::DividedByZero;
                cur.locals[inst] = RuntimeValue{ir::ConstantInt(
                    std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type()), ia->get_val() / ib->get_val())};
                break;
            }
            case IT::SREM: {
                auto a = readScalar(inst->get_operands_ref()[0]);
                auto b = readScalar(inst->get_operands_ref()[1]);
                if (!a || !b) return FailReason::Unsupported;
                auto ia = std::get_if<ir::ConstantInt>(&*a);
                auto ib = std::get_if<ir::ConstantInt>(&*b);
                if (!ia || !ib) return FailReason::Unsupported;
                if (ib->get_val() == 0) return FailReason::DividedByZero;
                cur.locals[inst] = RuntimeValue{ir::ConstantInt(
                    std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type()), ia->get_val() % ib->get_val())};
                break;
            }
            case IT::AND:
                if (auto rv = binInt([](int a, int b) { return a & b; }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::OR:
                if (auto rv = binInt([](int a, int b) { return a | b; }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::XOR:
                if (auto rv = binInt([](int a, int b) { return a ^ b; }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::SHL: {
                auto a = readScalar(inst->get_operands_ref()[0]);
                auto b = readScalar(inst->get_operands_ref()[1]);
                if (!a || !b) return FailReason::Unsupported;
                auto ia = std::get_if<ir::ConstantInt>(&*a);
                auto ib = std::get_if<ir::ConstantInt>(&*b);
                if (!ia || !ib) return FailReason::Unsupported;
                int sh = ib->get_val();
                if (sh < 0 || sh >= 32) return FailReason::Unsupported;
                int res = static_cast<int>(static_cast<uint32_t>(ia->get_val()) << sh);
                cur.locals[inst] =
                    RuntimeValue{ir::ConstantInt(std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type()), res)};
                break;
            }
            case IT::LSHR: {
                auto a = readScalar(inst->get_operands_ref()[0]);
                auto b = readScalar(inst->get_operands_ref()[1]);
                if (!a || !b) return FailReason::Unsupported;
                auto ia = std::get_if<ir::ConstantInt>(&*a);
                auto ib = std::get_if<ir::ConstantInt>(&*b);
                if (!ia || !ib) return FailReason::Unsupported;
                int sh = ib->get_val();
                if (sh < 0 || sh >= 32) return FailReason::Unsupported;
                int res = static_cast<int>(static_cast<uint32_t>(ia->get_val()) >> sh);
                cur.locals[inst] =
                    RuntimeValue{ir::ConstantInt(std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type()), res)};
                break;
            }
            case IT::ASHR: {
                auto a = readScalar(inst->get_operands_ref()[0]);
                auto b = readScalar(inst->get_operands_ref()[1]);
                if (!a || !b) return FailReason::Unsupported;
                auto ia = std::get_if<ir::ConstantInt>(&*a);
                auto ib = std::get_if<ir::ConstantInt>(&*b);
                if (!ia || !ib) return FailReason::Unsupported;
                int sh = ib->get_val();
                if (sh < 0 || sh >= 32) return FailReason::Unsupported;
                int res = ia->get_val() >> sh;
                cur.locals[inst] =
                    RuntimeValue{ir::ConstantInt(std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type()), res)};
                break;
            }
            case IT::FADD:
                if (auto rv = binF([](float a, float b) { return a + b; }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::FSUB:
                if (auto rv = binF([](float a, float b) { return a - b; }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::FMUL:
                if (auto rv = binF([](float a, float b) { return a * b; }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::FDIV:
                if (auto rv = binF([](float a, float b) { return a / b; }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::FREM:
                if (auto rv = binF([](float a, float b) { return std::fmod(a, b); }))
                    cur.locals[inst] = *rv;
                else
                    return FailReason::Unsupported;
                break;
            case IT::FNEG: {
                auto v = readScalar(inst->get_operands_ref()[0]);
                if (!v) return FailReason::Unsupported;
                auto fv = std::get_if<ir::ConstantFloat>(&*v);
                if (!fv) return FailReason::Unsupported;
                cur.locals[inst] = RuntimeValue{ir::ConstantFloat(ir::FloatType::get(), -fv->get_val())};
                break;
            }
            case IT::ICMP: {
                auto cmp = std::dynamic_pointer_cast<ir::ICmp>(inst);
                auto a = readScalar(inst->get_operands_ref()[0]);
                auto b = readScalar(inst->get_operands_ref()[1]);
                if (!a || !b) return FailReason::Unsupported;
                auto ia = std::get_if<ir::ConstantInt>(&*a);
                auto ib = std::get_if<ir::ConstantInt>(&*b);
                if (!ia || !ib) return FailReason::Unsupported;
                intmax_t av = ia->get_val();
                intmax_t bv = ib->get_val();
                bool r = false;
                switch (cmp->get_cmp_type()) {
                    case ir::ICmp::ICmpType::EQ:
                        r = av == bv;
                        break;
                    case ir::ICmp::ICmpType::NE:
                        r = av != bv;
                        break;
                    case ir::ICmp::ICmpType::SGT:
                        r = av > bv;
                        break;
                    case ir::ICmp::ICmpType::SGE:
                        r = av >= bv;
                        break;
                    case ir::ICmp::ICmpType::SLT:
                        r = av < bv;
                        break;
                    case ir::ICmp::ICmpType::SLE:
                        r = av <= bv;
                        break;
                    case ir::ICmp::ICmpType::UGT:
                        r = static_cast<uintmax_t>(av) > static_cast<uintmax_t>(bv);
                        break;
                    case ir::ICmp::ICmpType::UGE:
                        r = static_cast<uintmax_t>(av) >= static_cast<uintmax_t>(bv);
                        break;
                    case ir::ICmp::ICmpType::ULT:
                        r = static_cast<uintmax_t>(av) < static_cast<uintmax_t>(bv);
                        break;
                    case ir::ICmp::ICmpType::ULE:
                        r = static_cast<uintmax_t>(av) <= static_cast<uintmax_t>(bv);
                        break;
                    default:
                        return FailReason::Unsupported;
                }
                cur.locals[inst] = RuntimeValue{ir::ConstantInt(ir::IntegerType::get(1), r ? 1 : 0)};
                break;
            }
            case IT::FCMP: {
                auto cmp = std::dynamic_pointer_cast<ir::FCmp>(inst);
                auto a = readScalar(inst->get_operands_ref()[0]);
                auto b = readScalar(inst->get_operands_ref()[1]);
                if (!a || !b) return FailReason::Unsupported;
                auto fa = std::get_if<ir::ConstantFloat>(&*a);
                auto fb = std::get_if<ir::ConstantFloat>(&*b);
                if (!fa || !fb) return FailReason::Unsupported;
                double av = fa->get_val();
                double bv = fb->get_val();
                bool na = std::isnan(av), nb = std::isnan(bv);
                bool r = false;
                using FT = ir::FCmp::FCmpType;
                switch (cmp->get_cmp_type()) {
                    case FT::OEQ:
                        r = !na && !nb && av == bv;
                        break;
                    case FT::OGT:
                        r = !na && !nb && av > bv;
                        break;
                    case FT::OGE:
                        r = !na && !nb && av >= bv;
                        break;
                    case FT::OLT:
                        r = !na && !nb && av < bv;
                        break;
                    case FT::OLE:
                        r = !na && !nb && av <= bv;
                        break;
                    case FT::ONE:
                        r = !na && !nb && av != bv;
                        break;
                    case FT::ORD:
                        r = !na && !nb;
                        break;
                    case FT::UEQ:
                        r = na || nb || av == bv;
                        break;
                    case FT::UGT:
                        r = na || nb || av > bv;
                        break;
                    case FT::UGE:
                        r = na || nb || av >= bv;
                        break;
                    case FT::ULT:
                        r = na || nb || av < bv;
                        break;
                    case FT::ULE:
                        r = na || nb || av <= bv;
                        break;
                    case FT::UNE:
                        r = na || nb || av != bv;
                        break;
                    case FT::UNO:
                        r = na || nb;
                        break;
                    default:
                        return FailReason::Unsupported;
                }
                cur.locals[inst] = RuntimeValue{ir::ConstantInt(ir::IntegerType::get(1), r ? 1 : 0)};
                break;
            }
            case IT::FPTOSI: {
                auto v = readScalar(inst->get_operands_ref()[0]);
                if (!v) return FailReason::Unsupported;
                auto fv = std::get_if<ir::ConstantFloat>(&*v);
                if (!fv) return FailReason::Unsupported;
                cur.locals[inst] =
                    RuntimeValue{ir::ConstantInt(ir::IntegerType::get(32), static_cast<int>(fv->get_val()))};
                break;
            }
            case IT::SITOFP: {
                auto v = readScalar(inst->get_operands_ref()[0]);
                if (!v) return FailReason::Unsupported;
                auto iv = std::get_if<ir::ConstantInt>(&*v);
                if (!iv) return FailReason::Unsupported;
                cur.locals[inst] =
                    RuntimeValue{ir::ConstantFloat(ir::FloatType::get(), static_cast<float>(iv->get_val()))};
                break;
            }
            case IT::TRUNC: {
                auto v = readScalar(inst->get_operands_ref()[0]);
                if (!v) return FailReason::Unsupported;
                auto iv = std::get_if<ir::ConstantInt>(&*v);
                if (!iv) return FailReason::Unsupported;
                auto dst = std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type());
                if (!dst) return FailReason::Unsupported;
                if (dst->bits_num() == 1)
                    cur.locals[inst] =
                        RuntimeValue{ir::ConstantInt(ir::IntegerType::get(1), (iv->get_val() & 1) ? 1 : 0)};
                else
                    return FailReason::Unsupported;
                break;
            }
            case IT::ZEXT: {
                auto v = readScalar(inst->get_operands_ref()[0]);
                if (!v) return FailReason::Unsupported;
                auto iv = std::get_if<ir::ConstantInt>(&*v);
                if (!iv) return FailReason::Unsupported;
                auto src = std::dynamic_pointer_cast<ir::IntegerType>(inst->get_operands_ref()[0]->get_type());
                auto dst = std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type());
                if (!src || !dst) return FailReason::Unsupported;
                if (src->bits_num() == 1 && dst->bits_num() == 32)
                    cur.locals[inst] = RuntimeValue{ir::ConstantInt(ir::IntegerType::get(32), iv->get_val() ? 1 : 0)};
                else
                    return FailReason::Unsupported;
                break;
            }
            case IT::BITCAST: {
                auto v = readScalar(inst->get_operands_ref()[0]);
                if (!v) return FailReason::Unsupported;
                auto srcTy = inst->get_operands_ref()[0]->get_type();
                auto dstTy = inst->get_type();
                if (srcTy == dstTy) {
                    cur.locals[inst] = *v;
                    break;
                }
                if (srcTy->is_float_ty() && dstTy->is_integer_ty() &&
                    std::dynamic_pointer_cast<ir::IntegerType>(dstTy)->bits_num() == 32) {
                    auto fv = std::get_if<ir::ConstantFloat>(&*v);
                    if (!fv) return FailReason::Unsupported;
                    float f = fv->get_val();
                    uint32_t bits;
                    std::memcpy(&bits, &f, sizeof(bits));
                    cur.locals[inst] = RuntimeValue{ir::ConstantInt(ir::IntegerType::get(32), static_cast<int>(bits))};
                } else if (srcTy->is_integer_ty() &&
                           std::dynamic_pointer_cast<ir::IntegerType>(srcTy)->bits_num() == 32 &&
                           dstTy->is_float_ty()) {
                    auto iv = std::get_if<ir::ConstantInt>(&*v);
                    if (!iv) return FailReason::Unsupported;
                    uint32_t bits = static_cast<uint32_t>(iv->get_val());
                    float f;
                    std::memcpy(&f, &bits, sizeof(bits));
                    cur.locals[inst] = RuntimeValue{ir::ConstantFloat(ir::FloatType::get(), f)};
                } else {
                    return FailReason::Unsupported;
                }
                break;
            }
            case IT::SELECT: {
                auto c = readScalar(inst->get_operands_ref()[0]);
                if (!c) return FailReason::Unsupported;
                auto ci = std::get_if<ir::ConstantInt>(&*c);
                if (!ci) return FailReason::Unsupported;
                auto chosen =
                    ci->get_val() ? readScalar(inst->get_operands_ref()[1]) : readScalar(inst->get_operands_ref()[2]);
                if (!chosen) return FailReason::Unsupported;
                cur.locals[inst] = *chosen;
                break;
            }
            case IT::PHICOPY: {
                // No-op for interpreter since PHI are resolved at block entry
                break;
            }
            case IT::MOVE: {
                // Move src to dst in locals environment
                auto src = readScalar(inst->get_operands_ref()[0]);
                if (!src) return FailReason::Unsupported;
                auto dst = inst->get_operands_ref()[1];
                cur.locals[dst] = *src;
                break;
            }

            case IT::ALLOCA: {
                auto elemTy = std::dynamic_pointer_cast<ir::Alloca>(inst)->get_content_type();
                size_t size = static_cast<size_t>(elemTy->bits_num() / 8);
                uintptr_t addr = mem.stackPush(size, 4);
                cur.locals[inst] = RuntimeValue{addr};
                cur.stackAllocs.emplace(addr, inst);
                break;
            }
            case IT::LOAD: {
                auto addrV = readScalar(inst->get_operands_ref()[0]);
                if (!addrV) return FailReason::Unsupported;
                auto p = std::get_if<uintptr_t>(&*addrV);
                if (!p) return FailReason::Unsupported;
                auto ty = inst->get_type();
                if (ty->is_integer_ty()) {
                    uint32_t v = mem.loadValue<uint32_t>(*p);
                    cur.locals[inst] = RuntimeValue{
                        ir::ConstantInt(std::dynamic_pointer_cast<ir::IntegerType>(ty), static_cast<int>(v))};
                } else if (ty->is_float_ty()) {
                    float v = mem.loadValue<float>(*p);
                    cur.locals[inst] = RuntimeValue{ir::ConstantFloat(ir::FloatType::get(), v)};
                } else {
                    return FailReason::Unsupported;
                }
                break;
            }
            case IT::STORE: {
                auto val = readScalar(inst->get_operands_ref()[0]);
                auto addrV = readScalar(inst->get_operands_ref()[1]);
                if (!val || !addrV) return FailReason::Unsupported;
                auto p = std::get_if<uintptr_t>(&*addrV);
                if (!p) return FailReason::Unsupported;
                if (auto iv = std::get_if<ir::ConstantInt>(&*val)) {
                    uint32_t v = static_cast<uint32_t>(iv->get_val());
                    mem.storeValue<uint32_t>(*p, v);
                } else if (auto fv = std::get_if<ir::ConstantFloat>(&*val)) {
                    mem.storeValue<float>(*p, fv->get_val());
                } else {
                    return FailReason::Unsupported;
                }
                break;
            }
            case IT::GETELEMENTPTR: {
                auto base = readScalar(inst->get_operands_ref()[0]);
                if (!base) return FailReason::Unsupported;
                auto p = std::get_if<uintptr_t>(&*base);
                if (!p) return FailReason::Unsupported;
                auto baseTy = inst->get_operands_ref()[0]->get_type();
                uintptr_t addr = *p;
                for (size_t i = 1; i < inst->get_operands_ref().size(); ++i) {
                    if (baseTy->is_pointer_ty())
                        baseTy = std::dynamic_pointer_cast<ir::PointerType>(baseTy)->get_reference_type();
                    else if (baseTy->is_array_ty())
                        baseTy = std::dynamic_pointer_cast<ir::ArrayType>(baseTy)->get_element_type();
                    else
                        return FailReason::Unsupported;
                    auto idxV = readScalar(inst->get_operands_ref()[i]);
                    if (!idxV) return FailReason::Unsupported;
                    auto ci = std::get_if<ir::ConstantInt>(&*idxV);
                    if (!ci) return FailReason::Unsupported;
                    size_t elemSize = static_cast<size_t>(baseTy->bits_num() / 8);
                    addr = static_cast<uintptr_t>(static_cast<intptr_t>(addr) + static_cast<intptr_t>(ci->get_val()) *
                                                                                    static_cast<intptr_t>(elemSize));
                }
                cur.locals[inst] = RuntimeValue{addr};
                break;
            }
            case IT::MEMSET: {
                auto mems = std::dynamic_pointer_cast<ir::Memset>(inst);
                auto addrV = readScalar(inst->get_operands_ref()[0]);
                if (!addrV) return FailReason::Unsupported;
                auto p = std::get_if<uintptr_t>(&*addrV);
                if (!p) return FailReason::Unsupported;
                mem.memReset(*p, static_cast<size_t>(mems->get_size()), static_cast<std::byte>(mems->get_val() & 0xFF));
                break;
            }

            case IT::CALL: {
                auto callee = std::dynamic_pointer_cast<ir::Function>(inst->get_operands_ref()[0]);
                if (!callee) return FailReason::Unsupported;
                // runtime/stdlib stubs: keep minimal behavior
                // here, only model putint/putch/putfloat as no-ops and return void/0
                auto name = callee->get_name();
                if (name == ir::Function::putint->get_name() || name == ir::Function::putch->get_name() ||
                    name == ir::Function::putfloat->get_name()) {
                    // no-op
                    break;
                }
                if (name == ir::Function::getint->get_name()) {
                    cur.locals[inst] = RuntimeValue{ir::ConstantInt(ir::IntegerType::get(32), 0)};
                    break;
                }
                if (name == ir::Function::getfloat->get_name()) {
                    cur.locals[inst] = RuntimeValue{ir::ConstantFloat(ir::FloatType::get(), 0.0F)};
                    break;
                }
                if (name == ir::Function::getarray->get_name()) {
                    // return number of elements read, here stub as 0
                    cur.locals[inst] = RuntimeValue{ir::ConstantInt(ir::IntegerType::get(32), 0)};
                    break;
                }
                if (name == ir::Function::getfarray->get_name()) {
                    cur.locals[inst] = RuntimeValue{ir::ConstantInt(ir::IntegerType::get(32), 0)};
                    break;
                }
                if (name == ir::Function::putarray->get_name() || name == ir::Function::putfarray->get_name() ||
                    name == ir::Function::starttime->get_name() || name == ir::Function::stoptime->get_name()) {
                    // no-op
                    break;
                }

                if (stack.size() >= mMaxRecursiveDepth) return FailReason::ExceedMaxRecursiveDepth;

                Frame nf{};
                nf.caller = inst;
                nf.block = callee->entry_block();
                nf.predBlock = nullptr;
                nf.execIter = nf.block->get_instructions_ref().begin();
                auto &ops = inst->get_operands_ref();
                auto &params = callee->get_arguments_ref();
                for (size_t i = 1; i < ops.size(); ++i) {
                    auto argVal = readScalar(ops[i]);
                    if (!argVal) return FailReason::Unsupported;
                    nf.locals.emplace(params[i - 1], *argVal);
                }
                stack.push_back(std::move(nf));
                break;
            }

            case IT::RET: {
                auto caller = cur.caller;
                RuntimeValue retRV{std::monostate{}};
                if (!inst->get_operands_ref().empty()) {
                    auto rv = readScalar(inst->get_operands_ref()[0]);
                    if (!rv) return FailReason::Unsupported;
                    retRV = *rv;
                } else {
                    retRV = RuntimeValue{std::monostate{}};
                }
                // pop stack allocations owned by this frame
                for (const auto &kv : cur.stackAllocs) {
                    auto base = kv.first;
                    mem.stackPop(base);
                }
                stack.pop_back();
                if (stack.empty()) {
                    // top-level return
                    auto retTy = func->get_return_type();
                    if (retTy->is_void_ty()) return std::shared_ptr<ir::Constant>{nullptr};
                    auto c = toConst(retRV, retTy);
                    if (!c) return FailReason::Unsupported;
                    return c;
                }
                // write back to caller
                if (caller) stack.back().locals[caller] = retRV;
                break;
            }

            case IT::BR: {
                auto br = std::dynamic_pointer_cast<ir::Br>(inst);
                std::shared_ptr<ir::BasicBlock> target;
                if (!br->is_cond_branch()) {
                    target = std::dynamic_pointer_cast<ir::BasicBlock>(inst->get_operands_ref()[0]);
                } else {
                    auto cond = readScalar(inst->get_operands_ref()[0]);
                    if (!cond) return FailReason::Unsupported;
                    auto ci = std::get_if<ir::ConstantInt>(&*cond);
                    if (!ci) return FailReason::Unsupported;
                    target = std::dynamic_pointer_cast<ir::BasicBlock>(inst->get_operands_ref()[ci->get_val() ? 1 : 2]);
                }
                stack.back().predBlock = stack.back().block;
                stack.back().block = target;
                stack.back().execIter = target->get_instructions_ref().begin();
                break;
            }

            default:
                return FailReason::Unsupported;
        }
    }
}

std::variant<std::shared_ptr<ir::Constant>, FailReason> Interpreter::execute_with_effects(
    ir::Module &module,
    const std::shared_ptr<ir::Function> &func,
    const std::vector<std::shared_ptr<ir::Constant>> &arguments,
    const std::unordered_set<std::shared_ptr<ir::Instruction>> &tracked_allocas,
    std::vector<StoreEffect> *out_effects,
    IOContext *io) const {
    (void)io;
    if (!func) return FailReason::Unsupported;
    if (func->get_param_types().size() != arguments.size()) return FailReason::Unsupported;

    // Run a local copy of execute() logic but harvest STORE effects
    MemoryContext mem{mMemBudgetBytes};
    std::unordered_map<uintptr_t, std::shared_ptr<ir::Instruction>> addrToAlloca;

    std::vector<Frame> stack;
    if (func->get_basic_blocks_ref().empty()) return FailReason::Unsupported;

    Frame entry{};
    entry.caller = nullptr;
    entry.block = func->entry_block();
    entry.predBlock = nullptr;
    entry.execIter = entry.block->get_instructions_ref().begin();
    auto constToRV = [&](const std::shared_ptr<ir::Constant> &c) -> std::optional<RuntimeValue> {
        if (auto ci = std::dynamic_pointer_cast<ir::ConstantInt>(c)) return RuntimeValue{*ci};
        if (auto cf = std::dynamic_pointer_cast<ir::ConstantFloat>(c)) return RuntimeValue{*cf};
        return std::nullopt;
    };
    for (size_t i = 0; i < arguments.size(); ++i) {
        auto rv = constToRV(arguments[i]);
        if (!rv) return FailReason::Unsupported;
        entry.locals.emplace(func->get_arguments_ref()[i], *rv);
    }
    stack.push_back(std::move(entry));

    const auto start = Clock::now();
    size_t steps = 0;

    const auto toConst = [&](const RuntimeValue &rv,
                             const std::shared_ptr<ir::Type> &type) -> std::shared_ptr<ir::Constant> {
        if (auto pi = std::get_if<ir::ConstantInt>(&rv))
            return std::make_shared<ir::ConstantInt>(std::dynamic_pointer_cast<ir::IntegerType>(type), pi->get_val());
        if (auto pf = std::get_if<ir::ConstantFloat>(&rv))
            return std::make_shared<ir::ConstantFloat>(std::dynamic_pointer_cast<ir::FloatType>(type), pf->get_val());
        return nullptr;
    };

    auto readScalar = [&](std::shared_ptr<ir::Value> v) -> std::optional<RuntimeValue> {
        if (auto c = std::dynamic_pointer_cast<ir::ConstantInt>(v)) return RuntimeValue{*c};
        if (auto c = std::dynamic_pointer_cast<ir::ConstantFloat>(v)) return RuntimeValue{*c};
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            auto fIt = it->locals.find(v);
            if (fIt != it->locals.end()) return fIt->second;
        }
        if (auto f = std::dynamic_pointer_cast<ir::Function>(v)) return RuntimeValue{f};
        if (auto g = std::dynamic_pointer_cast<ir::GlobalVariable>(v)) return RuntimeValue{mem.getGlobalVarAddress(g)};
        return std::nullopt;
    };

    while (true) {
        if (++steps > 2000000ULL) return FailReason::ExceedTimeLimit;
        if (mem.isExceeded() || mem.hasException()) return FailReason::MemoryError;
        if (std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count() >
            static_cast<int64_t>(mTimeBudgetNs))
            return FailReason::ExceedTimeLimit;

        auto &cur = stack.back();
        auto it = cur.execIter;
        if (it == cur.block->get_instructions_ref().end()) return FailReason::UnknownError;

        if (auto phi = std::dynamic_pointer_cast<ir::Phi>(*it)) {
            while (it != cur.block->get_instructions_ref().end()) {
                if (!std::dynamic_pointer_cast<ir::Phi>(*it)) break;
                auto curPhi = std::dynamic_pointer_cast<ir::Phi>(*it);
                auto incomings = curPhi->get_operands_ref();
                for (size_t i = 0; i < incomings.size(); i += 2) {
                    auto blk = std::dynamic_pointer_cast<ir::BasicBlock>(incomings[i + 1]);
                    if (blk == cur.predBlock) {
                        auto val = readScalar(incomings[i]);
                        if (!val) return FailReason::Unsupported;
                        cur.locals[*it] = *val;
                        break;
                    }
                }
                ++it;
            }
            cur.execIter = it;
            continue;
        }

        auto &inst = *it;
        cur.execIter = std::next(it);
        using IT = ir::Instruction::InstructionType;
        auto opcode = inst->get_ins_type();

        auto binInt = [&](auto f) -> std::optional<RuntimeValue> {
            auto a = readScalar(inst->get_operands_ref()[0]);
            auto b = readScalar(inst->get_operands_ref()[1]);
            if (!a || !b) return std::nullopt;
            auto ia = std::get_if<ir::ConstantInt>(&*a);
            auto ib = std::get_if<ir::ConstantInt>(&*b);
            if (!ia || !ib) return std::nullopt;
            int av = ia->get_val();
            int bv = ib->get_val();
            return RuntimeValue{
                ir::ConstantInt(std::dynamic_pointer_cast<ir::IntegerType>(inst->get_type()), f(av, bv))};
        };
        auto binF = [&](auto f) -> std::optional<RuntimeValue> {
            auto a = readScalar(inst->get_operands_ref()[0]);
            auto b = readScalar(inst->get_operands_ref()[1]);
            if (!a || !b) return std::nullopt;
            auto fa = std::get_if<ir::ConstantFloat>(&*a);
            auto fb = std::get_if<ir::ConstantFloat>(&*b);
            if (!fa || !fb) return std::nullopt;
            float av = fa->get_val();
            float bv = fb->get_val();
            return RuntimeValue{ir::ConstantFloat(ir::FloatType::get(), f(av, bv))};
        };

        switch (opcode) {
            case IT::ALLOCA: {
                auto elemTy = std::dynamic_pointer_cast<ir::Alloca>(inst)->get_content_type();
                size_t size = static_cast<size_t>(elemTy->bits_num() / 8);
                uintptr_t addr = mem.stackPush(size, 4);
                cur.locals[inst] = RuntimeValue{addr};
                cur.stackAllocs.emplace(addr, inst);
                // map tracked allocas addresses for later reverse mapping
                if (tracked_allocas.count(std::dynamic_pointer_cast<ir::Instruction>(inst))) {
                    addrToAlloca.emplace(addr, std::dynamic_pointer_cast<ir::Instruction>(inst));
                }
                break;
            }
            case IT::LOAD: {
                auto addrV = readScalar(inst->get_operands_ref()[0]);
                if (!addrV) return FailReason::Unsupported;
                auto p = std::get_if<uintptr_t>(&*addrV);
                if (!p) return FailReason::Unsupported;
                auto ty = inst->get_type();
                if (ty->is_integer_ty()) {
                    uint32_t v = mem.loadValue<uint32_t>(*p);
                    cur.locals[inst] = RuntimeValue{
                        ir::ConstantInt(std::dynamic_pointer_cast<ir::IntegerType>(ty), static_cast<int>(v))};
                } else if (ty->is_float_ty()) {
                    float v = mem.loadValue<float>(*p);
                    cur.locals[inst] = RuntimeValue{ir::ConstantFloat(ir::FloatType::get(), v)};
                } else {
                    return FailReason::Unsupported;
                }
                break;
            }
            case IT::STORE: {
                auto val = readScalar(inst->get_operands_ref()[0]);
                auto addrV = readScalar(inst->get_operands_ref()[1]);
                if (!val || !addrV) return FailReason::Unsupported;
                auto p = std::get_if<uintptr_t>(&*addrV);
                if (!p) return FailReason::Unsupported;
                if (auto iv = std::get_if<ir::ConstantInt>(&*val)) {
                    uint32_t v = static_cast<uint32_t>(iv->get_val());
                    mem.storeValue<uint32_t>(*p, v);
                } else if (auto fv = std::get_if<ir::ConstantFloat>(&*val)) {
                    mem.storeValue<float>(*p, fv->get_val());
                } else {
                    return FailReason::Unsupported;
                }
                // effect harvesting: only when address belongs to a tracked alloca and index is constant
                // find owning alloca frame by base address <= p
                std::shared_ptr<ir::Instruction> baseAlloca{};
                uintptr_t baseAddr = 0;
                for (const auto &kv : addrToAlloca) {
                    auto b = kv.first;
                    if (b <= *p && b >= baseAddr) {
                        baseAddr = b;
                        baseAlloca = kv.second;
                    }
                }
                if (baseAlloca && out_effects) {
                    // compute linear index by assuming f32 element
                    int idx = static_cast<int>((*p - baseAddr) / sizeof(float));
                    std::shared_ptr<ir::Constant> cst;
                    if (auto iv = std::get_if<ir::ConstantInt>(&*val))
                        cst = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(),
                                                                  static_cast<float>(iv->get_val()));
                    else if (auto fv = std::get_if<ir::ConstantFloat>(&*val))
                        cst = std::make_shared<ir::ConstantFloat>(ir::FloatType::get(), fv->get_val());
                    if (cst) out_effects->push_back(StoreEffect{baseAlloca, idx, cst});
                }
                break;
            }
            case IT::GETELEMENTPTR: {
                auto base = readScalar(inst->get_operands_ref()[0]);
                if (!base) return FailReason::Unsupported;
                auto p = std::get_if<uintptr_t>(&*base);
                if (!p) return FailReason::Unsupported;
                auto baseTy = inst->get_operands_ref()[0]->get_type();
                uintptr_t addr = *p;
                for (size_t i = 1; i < inst->get_operands_ref().size(); ++i) {
                    if (baseTy->is_pointer_ty())
                        baseTy = std::dynamic_pointer_cast<ir::PointerType>(baseTy)->get_reference_type();
                    else if (baseTy->is_array_ty())
                        baseTy = std::dynamic_pointer_cast<ir::ArrayType>(baseTy)->get_element_type();
                    else
                        return FailReason::Unsupported;
                    auto idxV = readScalar(inst->get_operands_ref()[i]);
                    if (!idxV) return FailReason::Unsupported;
                    auto ci = std::get_if<ir::ConstantInt>(&*idxV);
                    if (!ci) return FailReason::Unsupported;
                    size_t elemSize = static_cast<size_t>(baseTy->bits_num() / 8);
                    addr = static_cast<uintptr_t>(static_cast<intptr_t>(addr) + static_cast<intptr_t>(ci->get_val()) *
                                                                                    static_cast<intptr_t>(elemSize));
                }
                cur.locals[inst] = RuntimeValue{addr};
                break;
            }
            case IT::MEMSET: {
                auto mems = std::dynamic_pointer_cast<ir::Memset>(inst);
                auto addrV = readScalar(inst->get_operands_ref()[0]);
                if (!addrV) return FailReason::Unsupported;
                auto p = std::get_if<uintptr_t>(&*addrV);
                if (!p) return FailReason::Unsupported;
                mem.memReset(*p, static_cast<size_t>(mems->get_size()), static_cast<std::byte>(mems->get_val() & 0xFF));
                break;
            }
            case IT::CALL: {
                auto callee = std::dynamic_pointer_cast<ir::Function>(inst->get_operands_ref()[0]);
                if (!callee) return FailReason::Unsupported;
                auto name = callee->get_name();
                if (name == ir::Function::putint->get_name() || name == ir::Function::putch->get_name() ||
                    name == ir::Function::putfloat->get_name() || name == ir::Function::putarray->get_name() ||
                    name == ir::Function::putfarray->get_name() || name == ir::Function::starttime->get_name() ||
                    name == ir::Function::stoptime->get_name()) {
                    break;  // ignore side-effecting IO in stub
                }
                if (name == ir::Function::getint->get_name()) {
                    cur.locals[inst] = RuntimeValue{ir::ConstantInt(ir::IntegerType::get(32), 0)};
                    break;
                }
                if (name == ir::Function::getfloat->get_name()) {
                    cur.locals[inst] = RuntimeValue{ir::ConstantFloat(ir::FloatType::get(), 0.0F)};
                    break;
                }
                return FailReason::Unsupported;  // disallow other calls in loop stub
            }
            case IT::ICMP: {
                auto cmp = std::dynamic_pointer_cast<ir::ICmp>(inst);
                auto a = readScalar(inst->get_operands_ref()[0]);
                auto b = readScalar(inst->get_operands_ref()[1]);
                if (!a || !b) return FailReason::Unsupported;
                auto ia = std::get_if<ir::ConstantInt>(&*a);
                auto ib = std::get_if<ir::ConstantInt>(&*b);
                if (!ia || !ib) return FailReason::Unsupported;
                intmax_t av = ia->get_val();
                intmax_t bv = ib->get_val();
                bool r = false;
                switch (cmp->get_cmp_type()) {
                    case ir::ICmp::ICmpType::EQ:
                        r = av == bv;
                        break;
                    case ir::ICmp::ICmpType::NE:
                        r = av != bv;
                        break;
                    case ir::ICmp::ICmpType::SGT:
                        r = av > bv;
                        break;
                    case ir::ICmp::ICmpType::SGE:
                        r = av >= bv;
                        break;
                    case ir::ICmp::ICmpType::SLT:
                        r = av < bv;
                        break;
                    case ir::ICmp::ICmpType::SLE:
                        r = av <= bv;
                        break;
                    case ir::ICmp::ICmpType::UGT:
                        r = static_cast<uintmax_t>(av) > static_cast<uintmax_t>(bv);
                        break;
                    case ir::ICmp::ICmpType::UGE:
                        r = static_cast<uintmax_t>(av) >= static_cast<uintmax_t>(bv);
                        break;
                    case ir::ICmp::ICmpType::ULT:
                        r = static_cast<uintmax_t>(av) < static_cast<uintmax_t>(bv);
                        break;
                    case ir::ICmp::ICmpType::ULE:
                        r = static_cast<uintmax_t>(av) <= static_cast<uintmax_t>(bv);
                        break;
                    default:
                        return FailReason::Unsupported;
                }
                cur.locals[inst] = RuntimeValue{ir::ConstantInt(ir::IntegerType::get(1), r ? 1 : 0)};
                break;
            }
            case IT::FCMP: {
                auto cmp = std::dynamic_pointer_cast<ir::FCmp>(inst);
                auto a = readScalar(inst->get_operands_ref()[0]);
                auto b = readScalar(inst->get_operands_ref()[1]);
                if (!a || !b) return FailReason::Unsupported;
                auto fa = std::get_if<ir::ConstantFloat>(&*a);
                auto fb = std::get_if<ir::ConstantFloat>(&*b);
                if (!fa || !fb) return FailReason::Unsupported;
                double av = fa->get_val();
                double bv = fb->get_val();
                bool na = std::isnan(av), nb = std::isnan(bv);
                bool r = false;
                using FT = ir::FCmp::FCmpType;
                switch (cmp->get_cmp_type()) {
                    case FT::OEQ:
                        r = !na && !nb && av == bv;
                        break;
                    case FT::OGT:
                        r = !na && !nb && av > bv;
                        break;
                    case FT::OGE:
                        r = !na && !nb && av >= bv;
                        break;
                    case FT::OLT:
                        r = !na && !nb && av < bv;
                        break;
                    case FT::OLE:
                        r = !na && !nb && av <= bv;
                        break;
                    case FT::ONE:
                        r = !na && !nb && av != bv;
                        break;
                    case FT::ORD:
                        r = !na && !nb;
                        break;
                    case FT::UEQ:
                        r = na || nb || av == bv;
                        break;
                    case FT::UGT:
                        r = na || nb || av > bv;
                        break;
                    case FT::UGE:
                        r = na || nb || av >= bv;
                        break;
                    case FT::ULT:
                        r = na || nb || av < bv;
                        break;
                    case FT::ULE:
                        r = na || nb || av <= bv;
                        break;
                    case FT::UNE:
                        r = na || nb || av != bv;
                        break;
                    case FT::UNO:
                        r = na || nb;
                        break;
                    default:
                        return FailReason::Unsupported;
                }
                cur.locals[inst] = RuntimeValue{ir::ConstantInt(ir::IntegerType::get(1), r ? 1 : 0)};
                break;
            }
            case IT::RET: {
                auto caller = cur.caller;
                RuntimeValue retRV{std::monostate{}};
                if (!inst->get_operands_ref().empty()) {
                    auto rv = readScalar(inst->get_operands_ref()[0]);
                    if (!rv) return FailReason::Unsupported;
                    retRV = *rv;
                } else {
                    retRV = RuntimeValue{std::monostate{}};
                }
                for (const auto &kv : cur.stackAllocs) {
                    auto base = kv.first;
                    mem.stackPop(base);
                }
                stack.pop_back();
                if (stack.empty()) {
                    auto retTy = func->get_return_type();
                    if (retTy->is_void_ty()) return std::shared_ptr<ir::Constant>{nullptr};
                    auto c = toConst(retRV, retTy);
                    if (!c) return FailReason::Unsupported;
                    return c;
                }
                if (caller) stack.back().locals[caller] = retRV;
                break;
            }
            case IT::BR: {
                auto br = std::dynamic_pointer_cast<ir::Br>(inst);
                std::shared_ptr<ir::BasicBlock> target;
                if (!br->is_cond_branch()) {
                    target = std::dynamic_pointer_cast<ir::BasicBlock>(inst->get_operands_ref()[0]);
                } else {
                    auto cond = readScalar(inst->get_operands_ref()[0]);
                    if (!cond) return FailReason::Unsupported;
                    auto ci = std::get_if<ir::ConstantInt>(&*cond);
                    if (!ci) return FailReason::Unsupported;
                    target = std::dynamic_pointer_cast<ir::BasicBlock>(inst->get_operands_ref()[ci->get_val() ? 1 : 2]);
                }
                stack.back().predBlock = stack.back().block;
                stack.back().block = target;
                stack.back().execIter = target->get_instructions_ref().begin();
                break;
            }
            default: {
                // fallback to normal execute handlers by reusing code path: copy-paste minimal set
                // handle other opcodes by delegating to execute() would require refactor; here duplicate minimal cases
                // integer and float arithmetics
                if (opcode == IT::ADD || opcode == IT::SUB || opcode == IT::MUL || opcode == IT::AND ||
                    opcode == IT::OR || opcode == IT::XOR) {
                    auto rv = (opcode == IT::ADD)   ? binInt([](int a, int b) { return a + b; })
                              : (opcode == IT::SUB) ? binInt([](int a, int b) { return a - b; })
                              : (opcode == IT::MUL) ? binInt([](int a, int b) { return a * b; })
                              : (opcode == IT::AND) ? binInt([](int a, int b) { return a & b; })
                              : (opcode == IT::OR)  ? binInt([](int a, int b) { return a | b; })
                                                    : binInt([](int a, int b) { return a ^ b; });
                    if (!rv) return FailReason::Unsupported;
                    cur.locals[inst] = *rv;
                    break;
                }
                if (opcode == IT::FADD || opcode == IT::FSUB || opcode == IT::FMUL || opcode == IT::FDIV ||
                    opcode == IT::FREM) {
                    auto rv = (opcode == IT::FADD)   ? binF([](float a, float b) { return a + b; })
                              : (opcode == IT::FSUB) ? binF([](float a, float b) { return a - b; })
                              : (opcode == IT::FMUL) ? binF([](float a, float b) { return a * b; })
                              : (opcode == IT::FDIV) ? binF([](float a, float b) { return a / b; })
                                                     : binF([](float a, float b) { return std::fmod(a, b); });
                    if (!rv) return FailReason::Unsupported;
                    cur.locals[inst] = *rv;
                    break;
                }
                if (opcode == IT::FNEG) {
                    auto v = readScalar(inst->get_operands_ref()[0]);
                    if (!v) return FailReason::Unsupported;
                    auto fv = std::get_if<ir::ConstantFloat>(&*v);
                    if (!fv) return FailReason::Unsupported;
                    cur.locals[inst] = RuntimeValue{ir::ConstantFloat(ir::FloatType::get(), -fv->get_val())};
                    break;
                }
                // load/gep handled as in execute()
                if (opcode == IT::LOAD || opcode == IT::GETELEMENTPTR || opcode == IT::MEMSET || opcode == IT::CALL ||
                    opcode == IT::RET || opcode == IT::BR || opcode == IT::ICMP || opcode == IT::FCMP ||
                    opcode == IT::FPTOSI || opcode == IT::SITOFP || opcode == IT::TRUNC || opcode == IT::ZEXT ||
                    opcode == IT::BITCAST || opcode == IT::SELECT || opcode == IT::PHICOPY || opcode == IT::MOVE) {
                    // fallback: temporarily call original execute by restarting interpreter is cumbersome; replicate
                    // minimal For brevity, delegate by returning Unsupported to skip transformation
                    return FailReason::Unsupported;
                }
                return FailReason::Unsupported;
            }
        }
    }
}

}  // namespace opt::exec
