/*
    SPDX-License-Identifier: Apache-2.0
    Copyright 2023 CMMC Authors
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

// reference: https://github.com/dtcxzyw/cmmc/blob/3888660547f82af579fb9013cdfefea52fd923f8/cmmc/Transforms/Util/PatternMatch.hpp#L4

#pragma once

#include "Value.h"
#include <Block.h>
#include <Instruction.h>

template <typename T>
static constexpr bool staticAssertionFail = false;

template <typename ValueType>
struct MatchContext final {
    ValueType* value;

    explicit MatchContext(ValueType* val) : value{ val } {};

    template <typename T = ValueType>
    [[nodiscard]] MatchContext<Value> getOperand(uint32_t idx) const {
        if constexpr(std::is_base_of_v<Instruction, T>) {
            const auto val = value->getOperand(idx);
            return MatchContext<Value>{ val };
        } else {
            static_assert(staticAssertionFail<T>, "Unsupported operation");
        }
    }
};

template <typename T, typename Derived>
class GenericMatcher {
public:
    bool operator()(const MatchContext<Value>& ctx) const noexcept {
        if(auto val = dynamic_cast<T*>(ctx.value)) {
            return (static_cast<const Derived*>(this))->handle(MatchContext<T>{ val });
        }
        return false;
    }
};

class AnyMatcher {
    Value*& mValue;

public:
    explicit AnyMatcher(Value*& value) noexcept : mValue{ value } {}
    bool operator()(const MatchContext<Value>& ctx) const noexcept {
        mValue = ctx.value;
        return true;
    }
};

inline auto any(Value*& val) {
    return AnyMatcher{ val };
}

//! NOTICE: in sysy doc, only has int, and is signed
class ConstantIntegerMatcher final : public GenericMatcher<Const, ConstantIntegerMatcher> {
    using Value = int;
    Value& mVal;

public:
    explicit ConstantIntegerMatcher(Value& val) noexcept : mVal{ val } {}
    [[nodiscard]] bool handle(const MatchContext<Const>& ctx) const noexcept {
        if(ctx.value->constId() != ConstID::Int)
            return false;
        mVal = ctx.value->intVal;
        return true;
    }
};

// NOLINTNEXTLINE
inline auto int_(int& val) noexcept {
    return ConstantIntegerMatcher{ val };
}

template <bool IsCommutative, typename LhsMatcher, typename RhsMatcher>
class BinaryOpMatcher final : public GenericMatcher<BinaryInstruction, BinaryOpMatcher<IsCommutative, LhsMatcher, RhsMatcher>> {
    BinaryInstructionOps mTarget;
    LhsMatcher mLhsMatcher;
    RhsMatcher mRhsMatcher;

public:
    explicit BinaryOpMatcher(BinaryInstructionOps target, LhsMatcher lhsMatcher, RhsMatcher rhsMatcher) noexcept
        : mTarget{ target }, mLhsMatcher{ lhsMatcher }, mRhsMatcher{ rhsMatcher } {}
    [[nodiscard]] bool handle(const MatchContext<BinaryInstruction>& ctx) const noexcept {
        if(/*mTarget != InstructionID::None &&*/ ctx.value->op != mTarget)
            return false;
        if(mLhsMatcher(ctx.getOperand(0)) && mRhsMatcher(ctx.getOperand(1)))
            return true;
        if constexpr(IsCommutative)
            return mLhsMatcher(ctx.getOperand(1)) && mRhsMatcher(ctx.getOperand(0));
        return false;
    }
};

template <typename Lhs, typename Rhs>
auto add(Lhs lhs, Rhs rhs) {
    return BinaryOpMatcher<true, Lhs, Rhs>{ BinaryInstructionOps::Add, lhs, rhs };
}

template <typename Lhs, typename Rhs>
auto sub(Lhs lhs, Rhs rhs) {
    return BinaryOpMatcher<false, Lhs, Rhs>{ BinaryInstructionOps::Sub, lhs, rhs };
}

class ExactlyMatcher final {
    Value*& mVal;

public:
    explicit ExactlyMatcher(Value*& val) noexcept : mVal{ val } {}
    bool operator()(const MatchContext<Value>& ctx) const noexcept {
        return ctx.value == mVal;
    }
};

inline auto exactly(Value*& val) noexcept {
    return ExactlyMatcher{ val };
}

class PhiMatcher final : public GenericMatcher<PhiInstruction, PhiMatcher> {
    PhiInstruction*& mPhi;

public:
    explicit PhiMatcher(PhiInstruction*& phi) noexcept : mPhi{ phi } {}
    [[nodiscard]] bool handle(const MatchContext<PhiInstruction>& ctx) const noexcept {
        mPhi = ctx.value;
        return true;
    }
};

inline auto phi(PhiInstruction*& phi) noexcept {
    return PhiMatcher{ phi };
}

template <typename LhsMatcher, typename RhsMatcher>
class ICompareMatcher final : public GenericMatcher<IcmpInstruction, ICompareMatcher<LhsMatcher, RhsMatcher>> {
    InsID mTarget;
    // IcmpKind mCompare;
    LhsMatcher mLhsMatcher;
    RhsMatcher mRhsMatcher;

public:
    explicit ICompareMatcher(InsID target /*, IcmpKind compare*/, LhsMatcher lhsMatcher, RhsMatcher rhsMatcher) noexcept
        : mTarget{ target } /*, mCompare{ compare }*/, mLhsMatcher{ lhsMatcher }, mRhsMatcher{ rhsMatcher } {}
    [[nodiscard]] bool handle(const MatchContext<IcmpInstruction>& ctx) const noexcept {
        if(ctx.value->insId != mTarget)
            return false;
        // mCompare = ctx.value->kind;
        if(mLhsMatcher(ctx.getOperand(0)) && mRhsMatcher(ctx.getOperand(1)))
            return true;
        if(mLhsMatcher(ctx.getOperand(1)) && mRhsMatcher(ctx.getOperand(0))) {
            // mCompare = ctx.value->getReverseKind();
            return true;
        }
        return false;
    }
};

template <typename Lhs, typename Rhs>
auto icmp(Lhs lhs, Rhs rhs) {
    return ICompareMatcher{ InsID::Icmp, lhs, rhs };
}